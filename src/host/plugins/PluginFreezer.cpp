/*
  freezer plugin by chay strawbridge
  11-sep2013

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <math.h>
#include "PluginFreezer.h"
#include <stdio.h>
#include "portaudio.h"
#include "settings.h"


PluginFreezer::PluginFreezer ()
{
	_gateStruct.env = 0.0;
	_gateStruct.gate = 0.0;
	_gateStruct.fs = fSAMPLE_RATE;
	_gateStruct.state = CLOSED;
	_gateStruct.oldState = CLOSED;
	_gateStruct.holdCount = 0;
	_gateStruct.a_rate=1;
	_gateStruct.d_rate=1;
	_gateStruct.cooloff=0;
	_gateStruct.cold=0;

	_grainLength=0;
	_grainPos=0;
	_grainState=GRAINSTATE_NULL;
	_grain = new StereoBuffer(MAX_TOTALGRAIN);
	_decay=0;

    registerPlugin(1,"Freezer",
     			   "Gate triggered grain synth",
     				1);
    //    {0, -10, 1, 2, 6703, 76, 2},

     registerParam(1,"Threshold","level at which gate opens","dB","","",-70,20,1,0);
     registerParam(2,"Range","difference between open and closed state","","dB","",-90,0,1,-10);

     registerParam(7,"Hold","time gate stays open","ms","","",2,500,2,2);
     registerParam(8,"Cooloff","prevent gate reopening","ms","","",2,500,2,2);

     registerParam(9,"Decay","decay of grain pad","ms","","",2,10000,10,2000);
};

PluginFreezer::~PluginFreezer ()
{
	delete _grain;
};

int PluginFreezer::process(StereoBuffer* input)
{
	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

    int i;
    float sum;

    // lazy, but quick enuf blank the buffer
    for (i=0;i<input->length;i++)
    {
    	outLeft[i]=0;
    	outRight[i]=0;
    }

    for (i = 0; i < PERIOD; i++) {

        sum = fabsf (inLeft[i]) + fabsf (inRight[i]);

        if (sum > _gateStruct.env)
        	_gateStruct.env = sum;
        else
        	_gateStruct.env = sum * ENV_TR + _gateStruct.env * (1.0f - ENV_TR);

        if (_gateStruct.state == CLOSED)
        {
            if (_gateStruct.env >= _gateStruct.t_level)
            {
            	if (_gateStruct.cold <= 0)
            		_gateStruct.state = OPENING;
            }
            _gateStruct.cold --;
        }
        else if (_gateStruct.state == OPENING)
        {
        	_gateStruct.gate += _gateStruct.a_rate;
            if (_gateStruct.gate >= 1.0)
            {
            	_gateStruct.gate = 1.0f;
            	_gateStruct.state = OPEN;
            	_gateStruct.holdCount = lrintf (_gateStruct.hold * _gateStruct.fs * 0.001f);
            }
        }
        else if (_gateStruct.state == OPEN)
        {
            if (_gateStruct.holdCount <= 0)
            {
                if (_gateStruct.env < _gateStruct.t_level)
                {
                	_gateStruct.state = CLOSING;
                }
            } else
            	_gateStruct.holdCount--;

        }
        else if (_gateStruct.state == CLOSING)
        {
        	_gateStruct.gate -= _gateStruct.d_rate;
            if (_gateStruct.env >= _gateStruct.t_level)
            {
            	if (_gateStruct.cold <=0)
            		_gateStruct.state = OPENING;
            }
            else if (_gateStruct.gate <= 0.0) {
            	_gateStruct.gate = 0.0;
            	_gateStruct.state = CLOSED;
            	_gateStruct.cold = lrintf(_gateStruct.cooloff * _gateStruct.fs * 0.001f);
            }
        }

        // act on open/closed gate
        if (_gateStruct.oldState != _gateStruct.state)
        {
			if (_gateStruct.state==OPEN)
			{
				_grainState = GRAINSTATE_RECORDING;
				_grainLength =0;
				_grainPos=0;
			}
			else if (_gateStruct.state==CLOSED)
			{
				_grainState = GRAINSTATE_PLAYING;
				_grainPos=0;
				_decay = lrintf(Pdecay * _gateStruct.fs * 0.001f);

				// todo: process grain here
			}

			_gateStruct.oldState = _gateStruct.state;
        }

        // grain
        if (_grainState==GRAINSTATE_RECORDING && _grainLength < MAX_TOTALGRAIN)
        {
        	_grain->left[_grainLength] = inLeft[i];
        	_grain->right[_grainLength] = inRight[i];
        	_grainLength++;
        }
        else if (_grainState==GRAINSTATE_PLAYING && _grainLength > 0)
        {
        	if (_decay-- <=0) {
        		_grainState = GRAINSTATE_STOPPED;
        	}
        }


		float level = _decay / (Pdecay * _gateStruct.fs * 0.001f);;
		outLeft[i] = _grain->left[_grainPos] * level;
		outRight[i] = _grain->right[_grainPos] * level;

		if (_grainPos++ >= _grainLength)
			_grainPos=0;


        //outLeft[i] = inLeft[i] * (_gateStruct.cut * (1.0f - _gateStruct.gate) + _gateStruct.gate);
        //outRight[i] = inRight[i]* (_gateStruct.cut * (1.0f - _gateStruct.gate) + _gateStruct.gate);
    }

    return paContinue;
}

/*
 * Cleanup the effect
 */
void
PluginFreezer::cleanup ()
{

};

void PluginFreezer::panic()
{
	cleanup();
	Plugin::panic();
}


void
PluginFreezer::setParam (int npar, int value)
{
    switch (npar) {

    case 1:
        Pthreshold = value;
        _gateStruct.t_level = dB2rap ((float)Pthreshold);
        break;
    case 2:
        Prange = value;
        _gateStruct.cut = dB2rap ((float)Prange);
        break;

    case 7:
        Phold = value;
        _gateStruct.hold = (float)Phold;
        break;
    case 8:
        Pcooloff = value;
        _gateStruct.cooloff = (float)Pcooloff;
        break;
    case 9:
    {
        Pdecay = value;
        _decay=0;
        break;
    }
    }
};

int
PluginFreezer::getParam (int npar)
{

    switch (npar)

    {
    case 1:
        return (Pthreshold);
        break;
    case 2:
        return (Prange);
        break;

    case 7:
        return (Phold);
        break;
    case 8:
        return (Pcooloff);
        break;
    case 9:
    	return (Pdecay);
    	break;
    }

    return (0);
};

