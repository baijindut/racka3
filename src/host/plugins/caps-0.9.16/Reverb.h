/*
	Reverb.h
	
	Copyright 2002-13 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	two reverb units: JVRev and Plate.
	
	the former is a rewrite of STK's JVRev, a traditional design.
	
	original comment:
	
		This is based on some of the famous    
		Stanford CCRMA reverbs (NRev, KipRev)  
		all based on the Chowning/Moorer/      
		Schroeder reverberators, which use     
		networks of simple allpass and comb    
		delay filters.  

	(STK is an effort of Gary Scavone).
	
	the algorithm is mostly unchanged in this implementation; the delay
	line lengths have been fiddled with to make the stereo field more
	evenly weighted, and denormal protection has been added.

	the Plate reverb is based on the circuit discussed in Jon Dattorro's 
	september 1997 JAES paper on effect design (part 1: reverb & filters).
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

#ifndef _REVERB_H_
#define _REVERB_H_

#include <stdio.h>

#include "dsp/Delay.h"
#include "dsp/OnePole.h"
#include "dsp/Sine.h"
#include "dsp/util.h"

/* both reverbs use this */
class Lattice
: public DSP::Delay
{
	public:
		inline sample_t 
		process (sample_t x, double d)
			{
				sample_t y = get();
				x -= d * y;
				put (x);
				return d * x + y;
			}
};

/* helper for JVRev */
class JVComb
: public DSP::Delay
{
	public:
		float c;
		
		inline sample_t 
		process (sample_t x)
			{
				x += c * get();
				put (x);
				return x;
			}
};

class JVRev
: public LadspaPlugin
{
	public:
		DSP::OnePoleLP<sample_t> bandwidth;

		static int default_length[9];
		sample_t t60;

		Lattice allpass [3];
		JVComb comb[4];

		DSP::Delay left, right;
		
		double apc;
		
		template <yield_func_t F>
		void cycle (uint frames);

		int length [9];

		void set_t60 (sample_t t);

	public:
		static PortInfo port_info [];

		void init();
		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

/* /////////////////////////////////////////////////////////////////////// */

class ModLattice
{
	public:
		float n0, width;

		DSP::Delay delay;
		DSP::Sine lfo;
		
		void init (int n, int w)
			{
				n0 = n;
				width = w;
				delay.init (n + w);
			}

		void reset()
			{
				delay.reset();
			}

		inline sample_t
		process (sample_t x, double d)
			{
				sample_t y = delay.get_linear (n0 + width * lfo.get());
				x += d * y;
				delay.put (x);
				return y - d * x; /* note sign */
			}
};

class PlateStub
: public LadspaPlugin
{
	public:
		sample_t f_lfo;

		sample_t indiff1, indiff2, dediff1, dediff2;
		
		struct {
			DSP::OnePoleLP<sample_t> bandwidth;
			Lattice lattice[4];
		} input;

		struct {
			ModLattice mlattice[2];
			Lattice lattice[2];
			DSP::Delay delay[4];
			DSP::OnePoleLP<sample_t> damping[2];
			int taps[12];
		} tank;

	public:
		void init();
		void activate()
			{ 
				input.bandwidth.reset();

				for (int i = 0; i < 4; ++i)
				{
					input.lattice[i].reset();
					tank.delay[i].reset();
				}

				for (int i = 0; i < 2; ++i)
				{
					tank.mlattice[i].reset();
					tank.lattice[i].reset();
					tank.damping[i].reset();
				}
				
				tank.mlattice[0].lfo.set_f (1.2, fs, 0);
				tank.mlattice[1].lfo.set_f (1.2, fs, .5 * M_PI);
			}

		inline void process (sample_t x, sample_t decay, 
					sample_t * xl, sample_t * xr);
};

/* /////////////////////////////////////////////////////////////////////// */

class Plate
: public PlateStub
{
	public:
		template <yield_func_t F>
			void cycle (uint frames);

	public:
		static PortInfo port_info [];

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

/* /////////////////////////////////////////////////////////////////////// */

class PlateX2
: public PlateStub
{
	public:
		template <yield_func_t F>
			void cycle (uint frames);

	public:
		static PortInfo port_info [];

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

#endif /* _REVERB_H_ */
