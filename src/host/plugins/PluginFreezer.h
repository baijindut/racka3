/*
  freezer plugin by chay strawbridge

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

#ifndef FREEZER_H
#define FREEZER_H

#include "global.h"
#include "../Plugin.h"
#include "../StereoBuffer.h"
#include "Freeverb/revmodel.h"

class PluginFreezer : public Plugin
{

public:
	PluginFreezer ();
    ~PluginFreezer ();

	int process(StereoBuffer* input);

	void panic();

private:
    void setParam (int npar, int value);
    int getParam (int npar);
    void cleanup ();

    int Pthreshold;
    int Pattack;			// release time (ms)
    int Prange;
    int Phold;
    int Pdecay;

private:

    struct gateStruct
    {
		int holdCount;
		int state;
		float cut;
		float t_level;
		float a_rate;
		float d_rate;
		float env;
		float gate;
		float fs;
		float hold;
	} _gateStruct;

    vector <revmodel*> _models;
    vector <float> _modelDelay;
    int _nextModel;
};

#endif
