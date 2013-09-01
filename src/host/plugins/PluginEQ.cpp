/*
  ZynAddSubFX - a software synthesizer

  EQ.C - EQ effect
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "PluginEQ.h"

PluginEQ::PluginEQ ()
{
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        filter[i].Ptype = 0;
        filter[i].Pfreq = 64;
        filter[i].Pgain = 64;
        filter[i].Pq = 64;
        filter[i].Pstages = 0;
        filter[i].l = new AnalogFilter (6, 1000.0f, 1.0f, 0);
        filter[i].r = new AnalogFilter (6, 1000.0f, 1.0f, 0);
    };
    //default values
    Pvolume = 50;

    cleanup ();

    registerPlugin(1,"EQ","Analogue EQ",1);
    registerParam(0,"Volume","","","Off","Loud",0,127,1,64);

    const char* filters[] = {"Off",
    					"LPF 1 pole",
    				    "HPF 1 pole",
    				    "LPF 2 poles",
    					"HPF 2 poles",
    					"BPF 2 poles",
    					"NOTCH 2 poles",
    					"PEAK (2 poles)",
    					"Low Shelf - 2 poles",
    					"High Shelf - 2 poles",
    					0};
    for (int i=0;i<3;i++)
    {
    	char name[64];

    	sprintf(name,"Band %d Filter",i);
    	registerParam(i*5 + 10,name,filters,0);

    	sprintf(name,"Band %d Freq",i);
    	registerParam(i*5 + 11,name,"","Hz","","",20,20000,1,440);

    	sprintf(name,"Band %d Gain",i);
    	registerParam(i*5 + 12,name,"","","Off","Loud",0,127,1,64);

    	sprintf(name,"Band %d Q",i);
    	registerParam(i*5 + 13,name,"","","","",0,127,1,64);

    	sprintf(name,"Band %d Stages",i);
    	registerParam(i*5 + 14,name,"","","","",1,4,1,1);
    }
};

PluginEQ::~PluginEQ ()
{
	for (int i = 0; i < MAX_EQ_BANDS; i++)
	{
		delete filter[i].l;
		delete filter[i].r;
	};
};

/*
 * Cleanup the effect
 */
void
PluginEQ::cleanup ()
{
    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        filter[i].l->cleanup ();
        filter[i].r->cleanup ();
    };
};

/*
 * Effect output
 */
int
PluginEQ::process(StereoBuffer* input)
{
    int i;
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

    for (i = 0; i < MAX_EQ_BANDS; i++) {
        if (filter[i].Ptype == 0)
            continue;
        filter[i].l->filterout (outLeft);
        filter[i].r->filterout (outRight);
    };

    for (i = 0; i < PERIOD; i++) {
    	outLeft[i] *= outvolume;
    	outRight[i] *= outvolume;
    };

    return	paContinue;
};


/*
 * Parameter control
 */
void
PluginEQ::setvolume (int Pvolume)
{
    this->Pvolume = Pvolume;

    outvolume = powf (0.005f, (1.0f - (float)Pvolume / 127.0f)) * 10.0f;


};


void
PluginEQ::setParam (int npar, int value)
{
    switch (npar) {
    case 0:
        setvolume (value);
        break;
    };
    if (npar < 10)
        return;

    // 0 to set volume
    // 1-9 invalid
    // then every subsequent 5 if a filter:
    // 10: filter type (0-9)
    // 11: frequency (20-20000)
    // 12: gain (0-127)
    // 13: Q (0-127)
    // 14: stage (0-MAX_EQ_BANDS)

    int nb = (npar - 10) / 5;	//number of the band (filter)
    if (nb >= MAX_EQ_BANDS)
        return;
    int bp = npar % 5;		//band paramenter

    float tmp;
    switch (bp) {
    case 0:
        if (value > 9)
            value = 0;		//has to be changed if more filters will be added
        filter[nb].Ptype = value;
        if (value != 0) {
            filter[nb].l->settype (value - 1);
            filter[nb].r->settype (value - 1);
        };
        break;
    case 1:
        filter[nb].Pfreq = value;
        tmp = (float)value;
        filter[nb].l->setfreq (tmp);
        filter[nb].r->setfreq (tmp);
        break;
    case 2:
        filter[nb].Pgain = value;
        tmp = 30.0f * ((float)value - 64.0f) / 64.0f;
        filter[nb].l->setgain (tmp);
        filter[nb].r->setgain (tmp);
        break;
    case 3:
        filter[nb].Pq = value;
        tmp = powf (30.0f, ((float)value - 64.0f) / 64.0f);
        filter[nb].l->setq (tmp);
        filter[nb].r->setq (tmp);
        break;
    case 4:
        if (value >= MAX_FILTER_STAGES)
            value = MAX_FILTER_STAGES - 1;
        filter[nb].Pstages = value;
        filter[nb].l->setstages (value);
        filter[nb].r->setstages (value);
        break;
    };
};

int
PluginEQ::getParam (int npar)
{
    switch (npar) {
    case 0:
        return (Pvolume);
        break;
    };

    if (npar < 10)
        return (0);

    int nb = (npar - 10) / 5;	//number of the band (filter)
    if (nb >= MAX_EQ_BANDS)
        return (0);
    int bp = npar % 5;		//band paramenter
    switch (bp) {
    case 0:
        return (filter[nb].Ptype);
        break;
    case 1:
        return (filter[nb].Pfreq);
        break;
    case 2:
        return (filter[nb].Pgain);
        break;
    case 3:
        return (filter[nb].Pq);
        break;
    case 4:
        return (filter[nb].Pstages);
        break;
    };

    return (0);			//in case of bogus parameter number
};

float PluginEQ::getfreqresponse (float freq)
{
    float
    resp = 1.0f;

    for (int i = 0; i < MAX_EQ_BANDS; i++) {
        if (filter[i].Ptype == 0)
            continue;
        resp *= filter[i].l->H (freq);
    };
    return (rap2dB (resp * outvolume));
};
