/*
	Click.h
	
	Copyright 2004-11 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Units repeatedly playing back a wavetable.

*/
/*
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 3
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA or point your web browser to http://www.gnu.org.
*/

#ifndef _CLICK_H_
#define _CLICK_H_

#include "dsp/OnePole.h"
#include "dsp/BiQuad.h"
#include "dsp/util.h"

template <int Waves>
class ClickStub
: public LadspaPlugin
{
	public:
		sample_t bpm;

		struct {
			int16 * data;
			uint N; /* number of samples in wave */
		} wave[Waves];

		DSP::OnePoleLP<sample_t> lp;

		uint period; /* frames remaining in period */
		uint played; /* frames played from sample */

		template <yield_func_t F>
				void cycle (uint frames);

		ClickStub() 
			{
				for (int i=0; i < Waves; ++i)
					wave[i].data = 0;
			}
		~ClickStub() 
			{
				for (int i=0; i < Waves; ++i)
					if (wave[i].data) delete [] wave[i].data;
			}

	public:
		void initwave (int i, int16 * wave, uint N);

		void activate()
			{ 
				played = 0;
				period = 0;
				bpm = -1;
			}

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

class Click
: public ClickStub<4>
{
	public:
		void initsimple();
		void initparfilt();
		void initsine();
		void initdirac();

		void init()
			{ initsimple(); initparfilt(); initsine(); initdirac(); }

		static PortInfo port_info [];
};

class CEO
: public ClickStub<1>
{
	public:
		void init();

		static PortInfo port_info [];
};

#endif /* _CLICK_H_ */
