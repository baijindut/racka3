/*
	ToneStack.h
	
	Copyright 2006
		David Yeh <dtyeh@ccrma.stanford.edu> (implementation)

	Copyright 2006-13
		Tim Goetze <tim@quitte.de> (cosmetics)

	Tone Stack emulation.

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

#ifndef _DSP_TONESTACK_H_
#define _DSP_TONESTACK_H_

#include "util.h"
#include "windows.h"
#include "TDFII.h"
#include "LatFilt.h"
#include "tonestack/tables.h"

namespace DSP {

typedef struct {
	double R1, R2, R3, R4;
	double C1, C2, C3;
} TSParameters;

#define TS_N_PRESETS (sizeof (DSP::ToneStack::presets) / \
				sizeof (DSP::TSParameters))

class ToneStack 
{
  private:
		enum { Order = 3 };

		double c;   // BT coef

		double b1t, b1m, b1l, b1d,
				b2t, b2m2, b2m, b2l, b2lm, b2d,
				b3lm, b3m2, b3m, b3t, b3tm, b3tl,
				a0, a1d, a1m, a1l, a2m, a2lm, a2m2, a2l, a2d,
				a3lm, a3m2, a3m, a3l, a3d;      // intermediate calculations

		double fs;
		TDFII<Order> filter;
		
	public:
		/* in ../ToneStack.cc */
		static const char * presetdict; 
		static TSParameters presets[]; 
		static int n_presets;

		ToneStack() 
			{
				setmodel(0);
			}
		
		void init (double _fs)
			{
				c = 2 * _fs;
			}

		void reset() { filter.reset(); }
		void setmodel (int model) 
			{ 
				setparams (presets[model]); 
				filter.reset();
			}

		void setparams (TSParameters & p)
			{
				double R1 = p.R1, R2 = p.R2, R3 = p.R3, R4 = p.R4;
				double C1 = p.C1, C2 = p.C2, C3 = p.C3;

				b1t  = C1*R1;
				b1m  = C3*R3;
				b1l  = C1*R2 + C2*R2;
				b1d  = C1*R3 + C2*R3;
				b2t  = C1*C2*R1*R4 + C1*C3*R1*R4;
				b2m2 = -(C1*C3*R3*R3 + C2*C3*R3*R3);
				b2m  = C1*C3*R1*R3 + C1*C3*R3*R3 + C2*C3*R3*R3;
				b2l  = C1*C2*R1*R2 + C1*C2*R2*R4 + C1*C3*R2*R4;
				b2lm = C1*C3*R2*R3 + C2*C3*R2*R3;
				b2d  = C1*C2*R1*R3 + C1*C2*R3*R4 + C1*C3*R3*R4;
				b3lm = C1*C2*C3*R1*R2*R3 + C1*C2*C3*R2*R3*R4;
				b3m2 = -(C1*C2*C3*R1*R3*R3 + C1*C2*C3*R3*R3*R4);
				b3m  = C1*C2*C3*R1*R3*R3 + C1*C2*C3*R3*R3*R4;
				b3t  = C1*C2*C3*R1*R3*R4;
				b3tm = -b3t;
				b3tl = C1*C2*C3*R1*R2*R4;
				a0   = 1;
				a1d  = C1*R1 + C1*R3 + C2*R3 + C2*R4 + C3*R4;
				a1m  = C3*R3;
				a1l  = C1*R2 + C2*R2;
				a2m  = C1*C3*R1*R3 - C2*C3*R3*R4 + C1*C3*R3*R3 + C2*C3*R3*R3;
				a2lm = C1*C3*R2*R3 + C2*C3*R2*R3;
				a2m2 = -(C1*C3*R3*R3 + C2*C3*R3*R3);
				a2l  = C1*C2*R2*R4 + C1*C2*R1*R2 + C1*C3*R2*R4 + C2*C3*R2*R4;
				a2d  = C1*C2*R1*R4 + C1*C3*R1*R4 + C1*C2*R3*R4
						+ C1*C2*R1*R3 + C1*C3*R3*R4 + C2*C3*R3*R4;
				a3lm = C1*C2*C3*R1*R2*R3 + C1*C2*C3*R2*R3*R4;
				a3m2 = -(C1*C2*C3*R1*R3*R3 + C1*C2*C3*R3*R3*R4);
				a3m  = C1*C2*C3*R3*R3*R4 + C1*C2*C3*R1*R3*R3 - C1*C2*C3*R1*R3*R4;
				a3l  = C1*C2*C3*R1*R2*R4;
				a3d  = C1*C2*C3*R1*R3*R4;

				filter.reset();
			}

		/* extensively reworked TG 5/2011 */
		inline void updatecoefs (double b, double m, double t)
			{
				struct {
						double a1, a2, a3;
						double b1, b2, b3;
				} acoef; // analog coefficients

				// digital coefficients
				double dcoef_a[Order + 1];
				double dcoef_b[Order + 1];

				m = (m - 1) * 3.5;
				m = pow (10, m);
		
				acoef.a1 = a1d + m*a1m + b*a1l;
				acoef.a2 = m*a2m + b*m*a2lm + m*m*a2m2 + b*a2l + a2d;
				acoef.a3 = b*m*a3lm + m*m*a3m2 + m*a3m + b*a3l + a3d;

				double c2 = c*c, c3 = c2*c;
				
				acoef.a1 *= c, acoef.a2 *= c2, acoef.a3 *= c3;

				dcoef_a[0] = -1 - acoef.a1 - acoef.a2 - acoef.a3; // sets scale
				dcoef_a[1] = -3 - acoef.a1 + acoef.a2 + 3*acoef.a3;
				dcoef_a[2] = -3 + acoef.a1 + acoef.a2 - 3*acoef.a3;
				dcoef_a[3] = -1 + acoef.a1 - acoef.a2 + acoef.a3;

				acoef.b1 = t*b1t + m*b1m + b*b1l + b1d;
				acoef.b2 = t*b2t + m*m*b2m2 + m*b2m + b*b2l + b*m*b2lm + b2d;
				acoef.b3 = b*m*b3lm + m*m*b3m2 + m*b3m + t*b3t + t*m*b3tm + t*b*b3tl;
				
				acoef.b1 *= c, acoef.b2 *= c2, acoef.b3 *= c3;

				dcoef_b[0] = - acoef.b1 - acoef.b2 - acoef.b3;
				dcoef_b[1] = - acoef.b1 + acoef.b2 + 3*acoef.b3;
				dcoef_b[2] = acoef.b1 + acoef.b2 - 3*acoef.b3;
				dcoef_b[3] = acoef.b1 - acoef.b2 + acoef.b3;

				double a0i = 1. / dcoef_a[0];
				for (int i=1; i<=3; ++i)
					filter.a[i] = dcoef_a[i] * a0i;
				
				for (int i=0; i<=3; ++i)
					filter.b[i] = dcoef_b[i] * a0i;
			} 

		/* actually do the DFII filtering, one sample at a time */
		inline sample_t process (sample_t x)
			{
				return filter.process (x);
			}
};

/* /////////////////////////////////////////////////////////////////////// */

/* 
	hardcode this, known size memory blocks
	extern double* KS;    // 25 x 25 x 3
	extern double* VS;    // 25 x 25 x 25 x 4
	extern double KS[NSTEPS][NSTEPS][TSORDER]; //[bass][mid][coefs]
	extern double VS[NSTEPS][NSTEPS][NSTEPS][TSORDER+1]; //[bass][mid][treb][coefs]
 */

class ToneStackLT 
{
  private:
		enum { Order = 3, Steps = 25 };

		// digital coefficients
		double *kcoef;
		double *vcoef;
		double af [Order + 1];
		double bf [Order + 1];

		double fs;
		LatFilt<Order> filter;

	public:
		ToneStackLT() 
			{ }

		void init (double _fs)
			{ }

		void reset()
			{
				filter.reset();
			}
	
		void updatecoefs (double b, double m, double t)
			{
				b = min (Steps-1, max (b * (Steps-1), 0));
				m = min (Steps-1, max (m * (Steps-1), 0));
				t = min (Steps-1, max (t * (Steps-1), 0));

				int bi = (int) b;
				int mi = (int) m;
				int ti = (int) t;

				kcoef = DSP::ToneStackKS +  (mi * Steps + bi) * Order;
				vcoef = DSP::ToneStackVS + ((mi * Steps + bi) * Steps + ti) * (Order + 1);
				
				for (int i = 0; i < Order; ++i) 
						filter.set_ki (kcoef[i], i);
						
				for (int i = 0; i < Order + 1; ++i) 
						filter.set_vi (vcoef[i], i);
			}

		/* actually do the DFII filtering, one sample at a time */
		inline sample_t process (sample_t x)
			{
				return filter.process (x);
			}
};

} /* namespace DSP */

#endif /* _DSP_TONESTACK_H_ */
