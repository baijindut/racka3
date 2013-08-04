/*
  ZynAddSubFX - a software synthesizer

  RBEcho.C - Echo effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu
  Reverse Echo effect by Transmogrifox

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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "PluginRBEcho.h"

PluginRBEcho::PluginRBEcho ()
{
	//default values
    Ppanning = 64;
    Pdelay = 60;
    Plrdelay = 100;
    Plrcross = 100;
    Pfb = 40;
    Phidamp = 10;
    Psubdiv = 1;
    subdiv = 1.0f;
    reverse = ireverse = 0.0f;
    pingpong = 0.0f;
    ipingpong = 1.0f;

    lrdelay = 0;
    Srate_Attack_Coeff = 1.0f / (fSAMPLE_RATE * ATTACK);
    maxx_delay = 1 + SAMPLE_RATE * MAX_DELAY;

    ldelay = new delayline(2.0f, 3);
    rdelay = new delayline(2.0f, 3);

	registerPlugin(1,"Echoverse",
				   "A flexible temp-syncing reversible echo",
				   1);

    registerParam(1,"Pan","","","far left","far right",0,127,1,64);
    registerParam(2,"Tempo","","BPM","","",0,600,1,120);
    registerParam(3,"LR Delay","","","","",0,127,1,80);
    registerParam(4,"Angle","","far left","far right","",-64,64,1,0);
    registerParam(5,"Feedback","","","","",0,127,1,80);
    registerParam(6,"Damping","","","","",0,127,1,10);
    registerParam(7,"Reverse","Reverse the echo","","forward","backward",0,127,127,0);
    registerParam(8,"Tempo Subdivision","fraction of beat","","","",0,5,1,0);
    registerParam(9,"E.S.","","","","",0,127,1,100);

    cleanup ();
};

PluginRBEcho::~PluginRBEcho ()
{
	delete ldelay;
	delete rdelay;
};

/*
 * Cleanup the effect
 */
void
PluginRBEcho::cleanup ()
{
    ldelay->cleanup();
    rdelay->cleanup();
    ldelay->set_averaging(0.25f);
    rdelay->set_averaging(0.25f);
    oldl = 0.0;
    oldr = 0.0;
};

void PluginRBEcho::panic()
{
	Plugin::panic();
	initdelays();
	cleanup();
}

/*
 * Initialize the delays
 */
void
PluginRBEcho::initdelays ()
{
    oldl = 0.0;
    oldr = 0.0;

    if(Plrdelay>0) {
        ltime = delay + lrdelay;
        rtime = delay - lrdelay;
    } else {
        ltime = delay - lrdelay;
        rtime = delay + lrdelay;
    }

    if(ltime > 2.0f) ltime = 2.0f;
    if(ltime<0.01f) ltime = 0.01f;

    if(rtime > 2.0f) rtime = 2.0f;
    if(rtime<0.01f) rtime = 0.01f;


};

int PluginRBEcho::process(StereoBuffer* input)
{
	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

    int i;
    float ldl, rdl;
    float avg, ldiff, rdiff, tmp;

   for (i = 0; i < PERIOD; i++) {

		//LowPass Filter
		ldl = lfeedback * hidamp + oldl * (1.0f - hidamp);
		rdl = rfeedback * hidamp + oldr * (1.0f - hidamp);
		oldl = ldl + DENORMAL_GUARD;
		oldr = rdl + DENORMAL_GUARD;

		ldl = ldelay->delay_simple((ldl + inLeft[i]), delay, 0, 1, 0);
		rdl = rdelay->delay_simple((rdl + inRight[i]), delay, 0, 1, 0);


		if(Preverse) {
			rvl = ldelay->delay_simple(oldl, delay, 1, 0, 1)*ldelay->envelope();
			rvr = rdelay->delay_simple(oldr, delay, 1, 0, 1)*rdelay->envelope();
			ldl = ireverse*ldl + reverse*rvl;
			rdl = ireverse*rdl + reverse*rvr;

		}


		lfeedback = lpanning * fb * ldl;
		rfeedback = rpanning * fb * rdl;

		if(Pes) {
			ldl *= cosf(lrcross);
			rdl *= sinf(lrcross);

			avg = (ldl + rdl) * 0.5f;
			ldiff = ldl - avg;
			rdiff = rdl - avg;

			tmp = avg + ldiff * pes;
			ldl = 0.5 * tmp;

			tmp = avg + rdiff * pes;
			rdl = 0.5f * tmp;


		}
		outLeft[i] = (ipingpong*ldl + pingpong *ldelay->delay_simple(0.0f, ltime, 2, 0, 0)) * lpanning;
		outRight[i] = (ipingpong*rdl + pingpong *rdelay->delay_simple(0.0f, rtime, 2, 0, 0)) * rpanning;
		//outLeft[i] = (ipingpong*ldl + pingpong *ldelay->delay_simple(0.0f, ltime, 2, 0, 0)) * lpanning;
		//outRight[i] = (ipingpong*rdl + pingpong *rdelay->delay_simple(0.0f, rtime, 2, 0, 0)) * rpanning;

	};

	return paContinue;
}

void
PluginRBEcho::setpanning (int Ppanning)
{
    this->Ppanning = Ppanning ;
    lpanning = ((float)Ppanning) / 64.0f;
    rpanning = 2.0f - lpanning;
    lpanning = 10.0f * powf(lpanning, 4);
    rpanning = 10.0f * powf(rpanning, 4);
    lpanning = 1.0f - 1.0f/(lpanning + 1.0f);
    rpanning = 1.0f - 1.0f/(rpanning + 1.0f);
    lpanning *= 1.1f;
    rpanning *= 1.1f;
};

void
PluginRBEcho::setreverse (int Preverse)
{
    this->Preverse = Preverse;
    reverse = (float) Preverse / 127.0f;
    ireverse = 1.0f - reverse;
};

void
PluginRBEcho::setdelay (int Pdelay)
{
    this->Pdelay = Pdelay;
    fdelay= 60.0f/((float) Pdelay);
    if (fdelay < 0.01f) fdelay = 0.01f;
    if (fdelay > (float) MAX_DELAY) fdelay = (float) MAX_DELAY;  //Constrains 10ms ... MAX_DELAY
    delay = subdiv * fdelay;
    initdelays ();
};

void
PluginRBEcho::setlrdelay (int Plrdelay)
{
    float tmp;
    this->Plrdelay = Plrdelay;
    lrdelay = delay * fabs(((float)Plrdelay - 64.0f) / 65.0f);

    tmp = fabs( ((float) Plrdelay - 64.0f)/32.0f);
    pingpong = 1.0f - 1.0f/(5.0f*tmp*tmp + 1.0f);
    pingpong *= 1.05159f;
    ipingpong = 1.0f - pingpong;
    initdelays ();
};

void
PluginRBEcho::setlrcross (int Plrcross)
{
    this->Plrcross = Plrcross;
    lrcross = D_PI * (float)Plrcross / 128.0f;

};

void
PluginRBEcho::setfb (int Pfb)
{
    this->Pfb = Pfb;
    fb = (float)Pfb / 128.0f;
};

void
PluginRBEcho::sethidamp (int Phidamp)
{
    this->Phidamp = Phidamp;
    hidamp = f_exp(-D_PI * 500.0f * ((float) Phidamp)/fSAMPLE_RATE);
};

void
PluginRBEcho::setParam (int npar, int value)
{
    switch (npar) {
    case 1:
        setpanning (value);
        break;
    case 2:
        setdelay (value);
        break;
    case 3:
        setlrdelay (value);
        break;
    case 4:
        setlrcross (value);
        break;
    case 5:
        setfb (value);
        break;
    case 6:
        sethidamp (value);
        break;
    case 7:
        setreverse (value);
        break;
    case 8:
        Psubdiv = value;
        subdiv = 1.0f/((float)(value + 1));
        delay = subdiv * fdelay;
        initdelays ();
        break;
    case 9:
        Pes = value;
        pes = 8.0f * (float)Pes / 127.0f;
        break;
    };
};

int
PluginRBEcho::getParam (int npar)
{
    switch (npar) {
    case 1:
        return (Ppanning);
        break;
    case 2:
        return (Pdelay);
        break;
    case 3:
        return (Plrdelay);
        break;
    case 4:
        return (Plrcross);
        break;
    case 5:
        return (Pfb);
        break;
    case 6:
        return (Phidamp);
        break;
    case 7:
        return (Preverse);
        break;
    case 8:
        return (Psubdiv);
        break;
    case 9:
        return (Pes);
        break;

    };
    return (0);			//in case of bogus parameter number
};
