/*
  ZynAddSubFX - a software synthesizer

  EQ.h - EQ Effect
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

#ifndef EQ_H
#define EQ_H

#include "global.h"
#include "AnalogFilter.h"
#include "Plugin.h"

#define MAX_EQ_BANDS 16

class PluginEQ : public Plugin
{
public:
    PluginEQ ();
    ~PluginEQ ();

	int process(StereoBuffer* input);

    void setParam (int npar, int value);
    int getParam (int npar);

private:

    void cleanup ();
    void setvolume (int Pvolume);

    int Pvolume;	//Volumul
    float outvolume;		//this is the volume of effect and is public because need it in system effect. The out volume of

    struct {
        //parameters
        int Ptype, Pfreq, Pgain, Pq, Pstages;
        //internal values
        AnalogFilter *l, *r;
    } filter[MAX_EQ_BANDS];

    // useful but not useful yet!
    float getfreqresponse (float freq);
};


#endif
