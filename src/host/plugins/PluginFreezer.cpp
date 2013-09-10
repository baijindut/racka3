/*
  ZynAddSubFX - a software synthesizer

  Chorus.C - Chorus and Flange effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu

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
    env = 0.0;
    gate = 0.0;
    fs = fSAMPLE_RATE;
    state = CLOSED;
    hold_count = 0;

    registerPlugin(1,"Freezer",
     			   "Gate triggered grain synth",
     				1);
    //    {0, -10, 1, 2, 6703, 76, 2},

     registerParam(1,"Threshold","","","","",-70,20,1,0);
     registerParam(2,"Range","","","dB","",-90,0,1,-10);
     registerParam(3,"Attack","","","","",1,250,1,1);
     registerParam(4,"Release","","","","",2,250,1,2);

     registerParam(7,"Hold","","","","",2,500,2,2);
};

PluginFreezer::~PluginFreezer ()
{

};

int PluginFreezer::process(StereoBuffer* input)
{
	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

    int i;
    float sum;

    for (i = 0; i < PERIOD; i++) {

        sum = fabsf (inLeft[i]) + fabsf (inRight[i]);

        if (sum > env)
            env = sum;
        else
            env = sum * ENV_TR + env * (1.0f - ENV_TR);

        if (state == CLOSED)
        {
            if (env >= t_level)
                state = OPENING;
        }
        else if (state == OPENING)
        {
            gate += a_rate;
            if (gate >= 1.0)
            {
                gate = 1.0f;
                state = OPEN;
                hold_count = lrintf (hold * fs * 0.001f);
            }
        }
        else if (state == OPEN)
        {
            if (hold_count <= 0)
            {
                if (env < t_level)
                {
                    state = CLOSING;
                }
            } else
                hold_count--;

        }
        else if (state == CLOSING)
        {
            gate -= d_rate;
            if (env >= t_level)
                state = OPENING;
            else if (gate <= 0.0) {
                gate = 0.0;
                state = CLOSED;
            }
        }

        outLeft[i] *= (cut * (1.0f - gate) + gate);
        outRight[i] *= (cut * (1.0f - gate) + gate);
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
        t_level = dB2rap ((float)Pthreshold);
        break;
    case 2:
        Prange = value;
        cut = dB2rap ((float)Prange);
        break;
    case 3:
        Pattack = value;
        a_rate = 1000.0f / ((float)Pattack * fs);
        break;
    case 4:
        Pdecay = value;
        d_rate = 1000.0f / ((float)Pdecay * fs);
        break;

    case 7:
        Phold = value;
        hold = (float)Phold;
        break;

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
    case 3:
        return (Pattack);
        break;
    case 4:
        return (Pdecay);
        break;

    case 7:
        return (Phold);
        break;

    }

    return (0);
};

