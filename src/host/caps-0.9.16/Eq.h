/*
	Eq.h
	
	Copyright 2004-13 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	IIR equalisation filters.

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

#ifndef _EQ_H_
#define _EQ_H_

#include "dsp/util.h"
#include "dsp/Eq.h"
#include "dsp/BiQuad.h"
#include "dsp/RBJ.h"
#include "dsp/v4f.h"
#include "dsp/v4f_BiQuad.h"

/* octave-band variants, mono and stereo */
class Eq10
: public LadspaPlugin
{
	public:
		sample_t gain[10];
		DSP::Eq<10> eq;

		int block;
			enum { BlockSize = 64 };

		template <yield_func_t F>
			void cycle (uint frames);

	public:
		static PortInfo port_info [];

		void init();
		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

class Eq10X2
: public LadspaPlugin
{
	public:
		sample_t gain[10];
		DSP::Eq<10> eq[2];

		template <yield_func_t F>
			void cycle (uint frames);

	public:
		static PortInfo port_info [];

		void init();
		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

/* 4-way parametric, parallel implementation */
class Eq4p
: public LadspaPlugin
{
	public:
		struct {sample_t mode,gain,f,Q;} state[4]; /* parameters */

		DSP::BiQuad4f filter[2];

		bool xfade;
		void updatestate();

		template <yield_func_t F>
			void cycle (uint frames);

	public:
		static PortInfo port_info [];

		void init();
		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

#endif /* _EQ_H_ */
