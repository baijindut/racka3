/*
	White.h
	
	Copyright 2004-12 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Noise generator.

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

#ifndef _WHITE_H_
#define _WHITE_H_

#include "dsp/White.h"
#include "dsp/OnePole.h"

class White
: public LadspaPlugin
{
	public:
		sample_t gain;

		DSP::White white, cream;
		DSP::OnePoleHP<sample_t> hp;

		template <yield_func_t F>
			void cycle (uint frames);

	public:
		static PortInfo port_info [];

		void init() {}
		void activate();

		void run (uint n) { cycle<store_func> (n); }
		void run_adding (uint n) { cycle<adding_func> (n); }
};

#endif /* _WHITE_H_ */
