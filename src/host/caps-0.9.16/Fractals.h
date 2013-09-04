/*
	Fractals.h
	
	Copyright 2004-11 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Lorenz and Roessler attractors made audible.

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

#ifndef _FRACTALS_H_
#define _FRACTALS_H_

#include "dsp/Lorenz.h"
#include "dsp/Roessler.h"
#include "dsp/OnePole.h"

class Fractal
: public LadspaPlugin
{
	public:
		sample_t h, gain;

		DSP::Lorenz lorenz;
		DSP::Roessler roessler;
		DSP::OnePoleHP<sample_t> hp; /* dc removal */

		template <yield_func_t F>
				void cycle (uint frames);
		template <yield_func_t F, int Mode>
				void subcycle (uint frames);

	public:
		static PortInfo port_info [];

		void init();
		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

#endif /* _FRACTALS_H_ */
