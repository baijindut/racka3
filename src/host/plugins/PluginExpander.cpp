
/*
  rakarrack - a guitar effects software

 Expander.C  -  Noise Gate Effect

  Copyright (C) 2010 Ryan Billing & Josep Andreu
  Author: Ryan Billing & Josep Andreu
  Adapted from swh-plugins Noise Gate by Steve Harris

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
#include "PluginExpander.h"


PluginExpander::PluginExpander ()
{
    lpfl = new AnalogFilter (2, 22000, 1, 0);
    lpfr = new AnalogFilter (2, 22000, 1, 0);
    hpfl = new AnalogFilter (3, 20, 1, 0);
    hpfr = new AnalogFilter (3, 20, 1, 0);

    env = 0.0;
    oldgain = 0.0;
    efollower = 0;
    fs = fSAMPLE_RATE;

    /*
    //Noise Gate
    {-50, 20, 50, 50, 3134, 76, 0},
    //Boost Gate
    {-55, 30, 50, 50, 1441, 157, 50},
    //Treble swell
    {-30, 9, 950, 25, 6703, 526, 90}
    */
    registerPlugin(1,"Expander","Expander",1);
    registerParam(1,"Threshold","","","","",-99,0,1,-30);
    registerParam(2,"Shape","","","","",0,64,1,9);
    registerParam(3,"Attack","","Ms","","",0,2000,1,950);
    registerParam(4,"Decay","","Ms","","",0,2000,1,25);
    registerParam(5,"LPF Freq","","Hz","","",20,20000,1,6703);
    registerParam(6,"HPF Freq","","Hz","","",20,20000,1,526);
    registerParam(7,"Level","","","","",0,127,1,90);
}

PluginExpander::~PluginExpander ()
{
	delete lpfl;
	delete lpfr;
	delete hpfl;
	delete hpfr;
}

void
PluginExpander::cleanup ()
{
    lpfl->cleanup ();
    hpfl->cleanup ();
    lpfr->cleanup ();
    hpfr->cleanup ();
    oldgain = 0.0f;

}

void
PluginExpander::setlpf (int value)
{
    Plpf = value;
    float fr = (float)Plpf;
    lpfl->setfreq (fr);
    lpfr->setfreq (fr);
};

void
PluginExpander::sethpf (int value)
{
    Phpf = value;
    float fr = (float)Phpf;
    hpfl->setfreq (fr);
    hpfr->setfreq (fr);
};

void
PluginExpander::setParam (int np, int value)
{

    switch (np) {

    case 1:
        Pthreshold = value;
        tfactor = dB2rap (-((float) Pthreshold));
        tlevel = 1.0f/tfactor;
        break;
    case 2:
        Pshape = value;
        sfactor = dB2rap ((float)Pshape/2);
        sgain = expf(-sfactor);
        break;
    case 3:
        Pattack = value;
        a_rate = 1000.0f/((float)Pattack * fs);
        break;
    case 4:
        Pdecay = value;
        d_rate = 1000.0f/((float)Pdecay * fs);
        break;
    case 5:
        setlpf(value);
        break;
    case 6:
        sethpf(value);
        break;
    case 7:
        Plevel = value;
        level = dB2rap((float) value/6.0f);
        break;

    }


}

int
PluginExpander::getParam (int np)
{

    switch (np)

    {
    case 1:
        return (Pthreshold);
        break;
    case 2:
        return (Pshape);
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
        return (Plevel);
        break;
    }

    return (0);

}

int
PluginExpander::process(StereoBuffer* input)
{
    int i;
    float delta = 0.0f;
    float expenv = 0.0f;

	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

	// copy to output buffer, in which the in-place processing will occur
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

        delta = 0.5f*(fabsf (outLeft[i]) + fabsf (outRight[i])) - env;    //envelope follower from Compressor.C
        if (delta > 0.0)
            env += a_rate * delta;
        else
            env += d_rate * delta;

        //End envelope power detection

        if (env > tlevel) env = tlevel;
        expenv = sgain * (expf(env*sfactor*tfactor) - 1.0f);		//Envelope waveshaping

        gain = (1.0f - d_rate) * oldgain + d_rate * expenv;
        oldgain = gain;				//smooth it out a little bit

        if(efollower) {
            outLeft[i] = gain;
            outRight[i] += gain;
        } else {
        	outLeft[i] *= gain*level;
        	outRight[i] *= gain*level;
        }

    }

    return paContinue;
};
