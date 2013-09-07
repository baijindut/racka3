/*
	Saturate.h
	
	Copyright 2004-13 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Oversampled waveshaper and an exciter-like circuit.

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

#ifndef _SATURATE_H_
#define _SATURATE_H_

#include "dsp/util.h"
#include "dsp/Oversampler.h"
#include "dsp/RMS.h"
#include "dsp/Compress.h"
#include "dsp/OnePole.h"
#include "dsp/BiQuad.h"
#include "dsp/Butterworth.h"
#include "dsp/Delay.h"
#include "dsp/ChebyshevPoly.h"

class Saturate
: public LadspaPlugin
{
	public:
		struct {
			float linear, delta;
		} gain;
		float bias;

		DSP::OnePoleHP<sample_t> hp;

		DSP::Oversampler<8,64> over;

		template <yield_func_t F>
				void cycle (uint frames);
		template <clip_func_t C, yield_func_t F>
				void subcycle (uint frames);

	public:
		static PortInfo port_info[];

		void init();

		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

/* stacked Butterworth crossover (Linkwitz-Riley) */
struct Splitter
{
	DSP::BiQuad<sample_t> lp[2], hp[2];
	float f;

	void reset()
		{ f=0; lp[0].reset(); lp[1].reset(); hp[0].reset(); hp[1].reset(); }
	void set_f (float _f)
		{
			DSP::Butterworth::LP (_f,lp[0]);
			DSP::Butterworth::LP (_f,lp[1]);
			f = _f;
			DSP::Butterworth::HP (f,hp[0]);
			DSP::Butterworth::HP (f,hp[1]);
		}
	sample_t low (sample_t x) 
		{ return lp[1].process(lp[0].process(x)); }
	sample_t high (sample_t x) 
		{ return hp[1].process(hp[0].process(x)); }
};

class Spice
: public LadspaPlugin
{
	public:
		Splitter split[2];
		DSP::BiQuad<sample_t> shape[2];
		DSP::ChebPoly<5> cheby; 

		uint remain;
		DSP::CompressPeak compress;

		template <yield_func_t F>
				void cycle (uint frames);

	public:
		static PortInfo port_info[];

		void init();
		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

class SpiceX2
: public LadspaPlugin
{
	public:
		struct {
			Splitter split[2];
			DSP::BiQuad<sample_t> shape[2];
		} chan[2];
		DSP::ChebPoly<5> cheby; 

		uint remain;
		DSP::CompressPeak compress;

		template <yield_func_t F>
				void cycle (uint frames);

	public:
		static PortInfo port_info[];

		void init();
		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

#endif /* _SATURATE_H_ */
