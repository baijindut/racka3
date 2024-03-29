/*
	dsp/Compress.h
	
	Copyright 2011-13 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Simplistic dynamic range processor.

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

#ifndef _DSP_COMPRESS_H_
#define _DSP_COMPRESS_H_

#include "RMS.h"
#include "OnePole.h"
#include "util.h"

namespace DSP {

class Compress
{
	public:
		uint blocksize;
		float over_block; /* per-sample delta = 1/blocksize */
		float threshold, attack, release;

		struct {
			sample_t current, target, direct;
			sample_t step;
			DSP::OnePoleLP<sample_t> lp;
		} gain;

		void init (float fs)
			{
				if (fs > 120000) blocksize = 4; 
				else if (fs > 60000) blocksize = 2; 
				else blocksize = 1;

				blocksize *= 4;
				over_block = 1./blocksize;

				set_threshold (0);
				set_attack (0);
				set_release (1);
				set_directgain (1);

				gain.current = gain.target = gain.direct;
				gain.step = 0;

				gain.lp.set (.05);
				/* prevent immediate drop from 0 sample in filter history */
				gain.lp.y1 = gain.current; 
			}

		void set_threshold (float t) { threshold = pow2 (t); }
		void set_directgain (float d) { gain.direct = 4 * d; }

		void set_attack (float a) 
			{ 
				attack = pow2 (2 * a);
				attack = (.001 + attack) * over_block;
			}
		void set_release (float r) 
			{ 
				release = pow2 (2 * r);
				release = (.001 + release) * over_block;
			}

		void start_block (sample_t powa, float strength)
			{
				if (powa < threshold)
					gain.target = gain.direct;
				else
				{
					sample_t t = pow5 (1 - (powa - threshold));
					t = max (.00001, t);
					gain.target = pow (4, (1 - strength) + strength * t);
				}

				if (gain.target < gain.current) 
					gain.step = -min (attack, (gain.current - gain.target) * over_block);
				else if (gain.target > gain.current) 
					gain.step = min (release, (gain.target - gain.current) * over_block);
				else
					gain.step = 0;
			}

		inline sample_t get()
			{
				gain.current += gain.step;
				gain.current = gain.lp.process (gain.current - 1e-20);
				return .0625 * pow2 (gain.current); /* 4*4 * 0.0625 = 1 */
			}
};

class CompressRMS
: public Compress
{
	public:
		struct {
			DSP::RMS<32> rms;
			DSP::OnePoleLP<sample_t> lp;
			sample_t current;
		} power;

		void init (float fs)
			{
				Compress::init (fs);

				power.current = 0;
				power.lp.set (.96);
				power.rms.reset();
			}

		inline void start_block (float strength)
			{
				power.current = power.rms.get();
				/* add a small constant to prevent denormals in recursion tail */
				sample_t powa = power.current = 
						power.lp.process (power.current + 1e-24);

				Compress::start_block (powa, strength);
			}

		void store (sample_t x)
			{
				power.rms.store (x*x);
			}

		void store (sample_t xl, sample_t xr)
			{
				power.rms.store (.5*(xl*xl + xr*xr));
			}
};

class CompressPeak
: public Compress
{
	public:
		struct {
			DSP::OnePoleLP<sample_t> lp;
			sample_t current;
		} peak;

		void init (float fs)
			{
				Compress::init (fs);

				peak.current = 0;
				peak.lp.set (.1);
			}

		inline void start_block (float strength)
			{
				/* add a small constant to prevent denormals in recursion tail */
				peak.current = peak.current * .9 + 1e-24;
				sample_t p = peak.lp.process (peak.current);

				Compress::start_block (p, strength);
			}

		void store (sample_t x)
			{
				x = fabs (x);
				if (x > peak.current)
					peak.current = x;
			}

		void store (sample_t xl, sample_t xr)
			{
				xl = fabs (xl);
				xr = fabs (xr);
				if (xl > peak.current)
					peak.current = xl;
				if (xr > peak.current)
					peak.current = xr;
			}
};

}; /* namespace DSP */

#endif /* _DSP_COMPRESS_H_ */
