/*
  PluginArpie.C - Arpeggiated Echo effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu
  Arpeggio Echo effect by Transmogrifox

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
#include "PluginArpie.h"
#include "portaudio.h"

PluginArpie::PluginArpie ()
{
    //default values
    Pvolume = 50;
    Ppanning = 64;
    Pdelay = 60;
    Plrdelay = 100;
    Plrcross = 100;
    Pfb = 40;
    Phidamp = 60;
    Pharms = 3;
    Psubdiv = 1;

    ldelay = NULL;
    rdelay = NULL;
    lrdelay = 0;
    harmonic = 1;
    Srate_Attack_Coeff = 1.0f / (fSAMPLE_RATE * ATTACK);
    invattack = SAMPLE_RATE/15;
    envattack = 1.0f/(float)invattack;
    maxx_delay = SAMPLE_RATE * MAX_DELAY;
    fade = SAMPLE_RATE / 10;    //200ms fade time available

    ldelay = new float[maxx_delay];
    rdelay = new float[maxx_delay];
    pattern = new int[MAXHARMS];

 //    {67, 64, 35, 64, 30, 59, 0, 127, 4},
    registerPlugin(1,"Arpie",
       				   "Arpeggiated Echo",
       				   1);
   //{, 35, 64, 30, 59, 0, 127, 4},
   registerParam(1,"Panning","","","","",0,127,1,64);
   registerParam(2,"Tempo","","","","",2,600,1,120);
    registerParam(3,"LR Delay","","","","",0,127,1,0);
    registerParam(4,"LR Cross","","","","",0,127,1,0);
    registerParam(5,"Feedback","","","","",0,127,1,0);
    registerParam(6,"HF Damping","","","","",0,127,1,0);
   registerParam(7,"Arpies","","","","",0,127,1,0);
    registerParam(8,"Harmonics","","","","",2,8,1,2);
    registerParam(9,"Pattern","","","","",0,4,1,0);
    registerParam(10,"Subdiv","","","","",1,6,1,1);

    setpattern (0);
    cleanup ();
};

PluginArpie::~PluginArpie ()
{
    delete[] ldelay;
    delete[] rdelay;
    delete[] pattern;
};

void PluginArpie::panic()
{
	cleanup();
	Plugin::panic();
}

/*
 * Cleanup the effect
 */
void
PluginArpie::cleanup ()
{
    memset(ldelay, 0 ,sizeof(float)* maxx_delay);
    memset(rdelay, 0 ,sizeof(float)* maxx_delay);
    oldl = 0.0;
    oldr = 0.0;
    rvkl = 0;
    rvkr = 0;
    kl = 0;
    kr = 0;
    harmonic = 1;
};


/*
 * Initialize the delays
 */
void
PluginArpie::initdelays ()
{
    int i;

    kl = 0;
    kr = 0;

    dl = delay - lrdelay;
    if (dl < 1)
        dl = 1;
    dr = delay + lrdelay;
    if (dr < 1)
        dr = 1;

    rvkl = 0;
    rvkr = 0;
    Srate_Attack_Coeff = 15.0f / (dl + dr);   // Set swell time to 1/10th of average delay time
    fade = (dl+dr)/5;

    for (i = dl; i < maxx_delay; i++)
        ldelay[i] = 0.0;
    for (i = dr; i < maxx_delay; i++)
        rdelay[i] = 0.0;
    oldl = 0.0;
    oldr = 0.0;

};

/*
 * Effect output
 */
int
PluginArpie::process (StereoBuffer* input)
{
    int i;
    float l, r, ldl, rdl, rswell, lswell;

	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

    for (i = 0; i < PERIOD; i++) {
        ldl = ldelay[kl];
        rdl = rdelay[kr];
        l = ldl * (1.0f - lrcross) + rdl * lrcross;
        r = rdl * (1.0f - lrcross) + ldl * lrcross;
        ldl = l;
        rdl = r;

        ldl = inLeft[i] * panning - ldl * fb;
        rdl = inRight[i] * (1.0f - panning) - rdl * fb;

        if(reverse > 0.0) {

            lswell =	(float)(abs(kl - rvkl)) * Srate_Attack_Coeff;
            envswell = (1.0f - cosf(PI * envcnt*envattack));
            if (envswell > 1.0f) envswell = 1.0f;
            if (lswell <= PI) {
                lswell = 0.5f * (1.0f - cosf(lswell));  //Clickless transition
                outLeft[i] = envswell * (reverse * (ldelay[rvkl] * lswell + ldelay[rvfl] * (1.0f - lswell))  + (ldl * (1-reverse)));   //Volume ducking near zero crossing.
            } else {
                outLeft[i] = ((ldelay[rvkl] * reverse)  + (ldl * (1-reverse))) * envswell;
            }

            rswell = 	(float)(abs(kr - rvkr)) * Srate_Attack_Coeff;
            if (rswell <= PI) {
                rswell = 0.5f * (1.0f - cosf(rswell));   //Clickless transition
                outRight[i] = envswell * (reverse * (rdelay[rvkr] * rswell + rdelay[rvfr] * (1.0f - rswell))  + (rdl * (1-reverse)));  //Volume ducking near zero crossing.
            } else {
                outRight[i] = envswell * ((rdelay[rvkr] * reverse)  + (rdl * (1-reverse)));
            }


        } else {
            outLeft[i]= ldl;
            outRight[i]= rdl;
        }


        //LowPass Filter
        ldelay[kl] = ldl = ldl * hidamp + oldl * (1.0f - hidamp);
        rdelay[kr] = rdl = rdl * hidamp + oldr * (1.0f - hidamp);
        oldl = ldl + DENORMAL_GUARD;
        oldr = rdl + DENORMAL_GUARD;

        if (++envcnt >= invattack) envcnt = invattack;
        if (kl > (dl - invattack)) envcnt -=2;
        if (envcnt < 0) envcnt = 0;

        if (++kl >= dl) {
            kl = 0;
            envcnt = 0;
            if (++harmonic >= Pharms) harmonic = 0;
        }
        if (++kr >= dr)
            kr = 0;
        rvkl += pattern[harmonic];
        if (rvkl >= (dl )) rvkl = rvkl%(dl);
        rvkr += pattern[harmonic];
        if (rvkr >= (dr )) rvkr = rvkr%(dr);

        rvfl = rvkl + fade;
        if (rvfl >= (dl ))  rvfl = rvfl%(dl);
        rvfr = rvkr + fade;
        if (rvfr >= (dr )) rvfr = rvfr%(dr);
    };

    return paContinue;
};


void
PluginArpie::setpanning (int Ppanning)
{
    this->Ppanning = Ppanning;
    panning = ((float)Ppanning + 0.5f) / 127.0f;
};

void
PluginArpie::setreverse (int Preverse)
{
    this->Preverse = Preverse;
    reverse = (float) Preverse / 127.0f;
};

void
PluginArpie::setdelay (int Pdelay)
{
    this->Pdelay = Pdelay;
    if (Pdelay < 2) Pdelay = 2;
    if (Pdelay > 600) Pdelay = 600;	//100ms .. 2 sec constraint
    delay = 1 + lrintf ( (60.0f/((float)(subdiv*Pdelay))) * fSAMPLE_RATE );	//quarter notes
    initdelays ();

};

void
PluginArpie::setlrdelay (int Plrdelay)
{
    float tmp;
    this->Plrdelay = Plrdelay;
    tmp =
        (powf (2.0, fabsf ((float)Plrdelay - 64.0f) / 64.0f * 9.0f) -
         1.0f) / 1000.0f * fSAMPLE_RATE;
    if (Plrdelay < 64.0)
        tmp = -tmp;
    lrdelay = lrintf(tmp);
    initdelays ();
};

void
PluginArpie::setlrcross (int Plrcross)
{
    this->Plrcross = Plrcross;
    lrcross = (float)Plrcross / 127.0f * 1.0f;
};

void
PluginArpie::setfb (int Pfb)
{
    this->Pfb = Pfb;
    fb = (float)Pfb / 128.0f;
};

void
PluginArpie::sethidamp (int Phidamp)
{
    this->Phidamp = Phidamp;
    hidamp = 0.5f - (float)Phidamp / 254.0f;
};


void
PluginArpie::setpattern (int Ppattern)
{
    this->Ppattern = Ppattern;

    const int PATTERN_SIZE = MAXHARMS;
    const int NUM_PATTERNS = 7;
    int setpatterns[NUM_PATTERNS][PATTERN_SIZE] = {
        {2, 3, 4, 5, 6, 7, 8, 9},
        {9, 8, 7, 6, 5, 4, 3, 2},
        {2, 4, 3, 5, 4, 6, 5, 7},
        {2, 2, 4, 3, 6, 2, 5, 3},
        {3, 2, 4, 3, 5, 4, 6, 5},
        {4, 3, 2, 7, 5, 3, 4, 2},
        {2, 3, 4, 5, 6, 7, 8, 9}
    };


    if (Ppattern >= PATTERN_SIZE)
        Ppattern = PATTERN_SIZE - 1;
    for (int ii = 0; ii < PATTERN_SIZE; ii++)
        pattern[ii] = setpatterns[Ppattern][ii];

};

void
PluginArpie::setParam (int npar, int value)
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
        Pharms = value;
        if ( (Pharms < 2) && (Pharms >= MAXHARMS)) {
            Pharms = 2;
        }
        break;
    case 9:
        setpattern(value);
        break;
    case 10:
        Psubdiv = value;
        subdiv = Psubdiv+1;
        setdelay (Pdelay);
        break;

    };
};

int
PluginArpie::getParam (int npar)
{
    switch (npar) {
    case 0:
        return (Pvolume);
        break;
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
        return (Pharms);
        break;
    case 9:
        return(Ppattern);
        break;
    case 10:
        return (Psubdiv);
        break;
    };
    return (0);			//in case of bogus parameter number
};
