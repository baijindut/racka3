/*
  ZynAddSubFX - a software synthesizer

  Chorus.C - Chorus and Flange effects
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

#include <math.h>
#include "PluginChroma.h"
#include <stdio.h>
#include "portaudio.h"
#include "settings.h"
#include "nnls-chroma-0.2.1/PluginInputDomainAdapter.h"
#include "nnls-chroma-0.2.1/PluginBufferingAdapter.h"
#include <sys/time.h>

PluginChroma::PluginChroma ()
{
	_chroma=0;

	_chroma = new NNLSChroma(fSAMPLE_RATE);
	if (_chroma->getInputDomain() == VampPlugin::FrequencyDomain) {
		_chroma = new PluginInputDomainAdapter(_chroma);
	}
	_chroma = new PluginBufferingAdapter(_chroma);

	// need to initialise plugin

	/*
	if (adapterFlags & ADAPT_BUFFER_SIZE) {
	_chroma = new PluginBufferingAdapter(_chroma);
	}
	if (adapterFlags & ADAPT_CHANNEL_COUNT) {
	_chroma = new PluginChannelAdapter(_chroma);
	}
	*/

	registerPlugin(1,"Chroma","Experimental chord detection",1);
};

PluginChroma::~PluginChroma ()
{
	if (_chroma)
		delete _chroma;
};

int PluginChroma::process(StereoBuffer* input)
{
	if (_chroma)
	{
		for (int i=0;i<input->length;i++)
		{
			_mono[i]= (input->left[i]*0.5 ) + ( input->right[i]*0.5);
		}

		timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		RealTime rt;
		rt.nsec = ts.tv_sec;
		rt.nsec = ts.tv_nsec;
		_chroma->process((const float**)&_mono,rt);
	}

    return paContinue;
}

/*
 * Cleanup the effect
 */
void
PluginChroma::cleanup ()
{

};

void PluginChroma::panic()
{
	cleanup();
	Plugin::panic();
}


void
PluginChroma::setParam (int npar, int value)
{

};

int
PluginChroma::getParam (int npar)
{

    return (0);
};

