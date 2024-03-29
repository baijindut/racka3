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
#include "EQ.h"

PluginEQ::PluginEQ (float * efxoutl_, float * efxoutr_)
{

    efxoutl = efxoutl_;
    efxoutr = efxoutr_;


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
    Ppreset = 0;
    Pvolume = 50;

    setpreset (Ppreset);
    cleanup ();
};

PluginEQ::~PluginEQ ()
{
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
void
PluginEQ::out (float * smpsl, float * smpsr)
{
    int i;
    for (i = 0; i < MAX_EQ_BANDS; i++) {
        if (filter[i].Ptype == 0)
            continue;
        filter[i].l->filterout (efxoutl);
        filter[i].r->filterout (efxoutr);
    };


    for (i = 0; i < PERIOD; i++) {
        efxoutl[i] = smpsl[i] * outvolume;
        efxoutr[i] = smpsr[i] * outvolume;
    };

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
PluginEQ::setpreset (int npreset)
{
    const int PRESET_SIZE = 1;
    const int NUM_PRESETS = 2;
    int presets[NUM_PRESETS][PRESET_SIZE] = {
        //EQ 1
        {67},
        //EQ 2
        {67}
    };

    for (int n = 0; n < PRESET_SIZE; n++)
        changepar (n, presets[npreset][n]);
    Ppreset = npreset;
};


void
PluginEQ::changepar (int npar, int value)
{
    switch (npar) {
    case 0:
        setvolume (value);
        break;
    };
    if (npar < 10)
        return;

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
PluginEQ::getpar (int npar)
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
