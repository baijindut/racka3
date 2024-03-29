/*
  Rakarrack   Audio FX software
  Stompbox.h - stompbox modeler
  Using Steve Harris LADSPA Plugin harmonic_gen
  Modified for rakarrack by Ryan Billing & Josep Andreu

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

#ifndef STOMPBOX_H
#define STOMPBOX_H

#include "global.h"
#include "AnalogFilter.h"
#include "Waveshaper.h"
#include "../StereoBuffer.h"
#include "Plugin.h"

class PluginStompBox : public Plugin
{
public:
    PluginStompBox ();
    ~PluginStompBox ();

    int process (StereoBuffer* input);

    void setParam (int npar, int value);
    int getParam (int npar);
    void cleanup ();

private:

    void setvolume (int value);
    void init_mode (int value);
    void init_tone ();

    int Pgain;
    int Phigh;
    int Pmid;
    int Plow;
    int Pmode;

    float gain, pre1gain, pre2gain, lowb, midb, highb, volume;
    float LG, MG, HG, RGP2, RGPST, pgain;

    AnalogFilter *linput, *lpre1, *lpre2, *lpost, *ltonehg, *ltonemd, *ltonelw;
    AnalogFilter *rinput, *rpre1, *rpre2, *rpost, *rtonehg, *rtonemd, *rtonelw;
    AnalogFilter *ranti, *lanti;
    Waveshaper *lwshape, *rwshape, *lwshape2, *rwshape2;

    StereoBuffer* _intBuffer;
};


#endif
