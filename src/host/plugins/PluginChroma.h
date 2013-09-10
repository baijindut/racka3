/*
  ZynAddSubFX - a software synthesizer

  Chorus.h - Chorus and Flange effects
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

#ifndef CHROMA_H
#define CHROMA_H

#include "global.h"
#include "../Plugin.h"
#include "../StereoBuffer.h"

#include "nnls-chroma-0.2.1/NNLSChroma.h"

class PluginChroma : public Plugin
{

public:
	PluginChroma ();
    ~PluginChroma ();

	int process(StereoBuffer* input);

	void panic();

private:
    void setParam (int npar, int value);
    int getParam (int npar);
    void cleanup ();

private:
    VampPlugin* _chroma;
    float _mono[PERIOD];
};

#endif
