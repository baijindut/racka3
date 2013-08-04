// Based in gate_1410.c LADSPA Swh-plugins


/*
  rakarrack - a guitar effects software

 Gate.C  -  Noise Gate Effect
 Based on Steve Harris LADSPA gate.

  Copyright (C) 2008 Josep Andreu
  Author: Josep Andreu

 This program is free software; you can redistribute it and/or modify
 it under the terms of version 2 of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License (version 2) for more details.

 You should have received a copy of the GNU General Public License
 (version2)  along with this program; if not, write to the Free Software
 Foundation,
 Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <math.h>
#include "PluginNoiseGate.h"


PluginNoiseGate::PluginNoiseGate ()
{
    lpfl = new AnalogFilter (2, 22000, 1, 0);
    lpfr = new AnalogFilter (2, 22000, 1, 0);
    hpfl = new AnalogFilter (3, 20, 1, 0);
    hpfr = new AnalogFilter (3, 20, 1, 0);

    env = 0.0;
    gate = 0.0;
    fs = fSAMPLE_RATE;
    state = CLOSED;
    hold_count = 0;

    registerPlugin(1,"Noise Gate",
     			   "Filter and gate",
     				1);
    //    {0, -10, 1, 2, 6703, 76, 2},

     registerParam(1,"Threshold","","","","",-70,20,1,0);
     registerParam(2,"Range","","","dB","",-90,0,1,-10);
     registerParam(3,"Attack","","","","",1,250,1,1);
     registerParam(4,"Release","","","","",2,250,1,2);
     registerParam(5,"Low Pass","","","","",20,26000,10,6703);
     registerParam(6,"High Pass","","","","",20,26000,10,76);
     registerParam(7,"Hold","","","","",2,500,2,2);
}

PluginNoiseGate::~PluginNoiseGate ()
{
	delete lpfl;
	delete lpfr;
	delete hpfl;
	delete hpfr;
}

void PluginNoiseGate::cleanup()
{
    lpfl->cleanup ();
    hpfl->cleanup ();
    lpfr->cleanup ();
    hpfr->cleanup ();
}

void PluginNoiseGate::panic()
{
	cleanup();
	Plugin::panic();
}

void PluginNoiseGate::setlpf (int value)
{
    Plpf = value;
    float fr = (float)Plpf;
    lpfl->setfreq (fr);
    lpfr->setfreq (fr);
};

void PluginNoiseGate::sethpf (int value)
{
    Phpf = value;
    float fr = (float)Phpf;
    hpfl->setfreq (fr);
    hpfr->setfreq (fr);
};

void PluginNoiseGate::setParam(int np, int value)
{

    switch (np) {

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
    case 5:
        setlpf(value);
        break;
    case 6:
        sethpf(value);
        break;
    case 7:
        Phold = value;
        hold = (float)Phold;
        break;

    }


}

int PluginNoiseGate::getParam(int np)
{

    switch (np)

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
    case 5:
        return (Plpf);
        break;
    case 6:
        return (Phpf);
        break;
    case 7:
        return (Phold);
        break;

    }

    return (0);

}

int PluginNoiseGate::process(StereoBuffer* input)
{
	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

    int i;
    float sum;

    for (i=0;i<PERIOD;i++)
    {
    	outLeft[i]=inLeft[i];
    	outRight[i]=inRight[i];
    }

    lpfl->filterout (outLeft);
    hpfl->filterout (outLeft);
    lpfr->filterout (outRight);
    hpfr->filterout (outRight);

    for (i = 0; i < PERIOD; i++) {

        sum = fabsf (inLeft[i]) + fabsf (inRight[i]);


        if (sum > env)
            env = sum;
        else
            env = sum * ENV_TR + env * (1.0f - ENV_TR);

        if (state == CLOSED) {
            if (env >= t_level)
                state = OPENING;
        } else if (state == OPENING) {
            gate += a_rate;
            if (gate >= 1.0) {
                gate = 1.0f;
                state = OPEN;
                hold_count = lrintf (hold * fs * 0.001f);
            }
        } else if (state == OPEN) {
            if (hold_count <= 0) {
                if (env < t_level) {
                    state = CLOSING;
                }
            } else
                hold_count--;

        } else if (state == CLOSING) {
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
};
