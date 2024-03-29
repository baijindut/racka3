/*
	dsp/v4f_BiQuad.h
	
	Copyright 2003-13 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Bi-quad IIR filters, SIMD 

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

#ifndef _DSP_V4F_BI_QUAD_H_
#define _DSP_V4F_BI_QUAD_H_

#include "v4f.h"

namespace DSP {

#ifdef __APPLE__
inline float pow10f(float f) {return pow(10,f);}
#endif

class RBJ4f
{
	public:
		v4f_t sin, cos, alpha;
		
		RBJ4f (v4f_t f, v4f_t Q)
			{
				v4f_t w = v4f_2pi * f;

				sin = v4f_map<__builtin_sinf> (w);
				cos = v4f_map<__builtin_cosf> (w);

				alpha = sin / (v4f_2 * Q);
			}
};

/* four parallel filters */
class BiQuad4f
{
	public:
		/* sufficient space to align a,b,x,y[] into */
		char __data [10 * sizeof (v4f_t)];
		v4f_t * _data;
		inline v4f_t * data() { return _data; }

		int h; /* history index */

		BiQuad4f()
			{
				h = 0;
				_data = (v4f_t *) (((uint64)__data + 16) & ~15ll);
				unity();
				reset();
			}
		
		/* needed to make sure copy has properly aligned storage */
		void operator = (BiQuad4f & b)
			{
				h = b.h;
				memcpy (data(), b.data(), 9 * sizeof (v4f_t));
			}

		void unity()
			{
				v4f_t *a = data(), *b = a+2;
				a[0] = v4f_1;
				a[1] = a[2] = b[1] = b[2] = v4f_0;
			}

#ifdef __CREAM__
		void dumpab()
			{
				v4f_t *a = data();
				
				msg(), "a0 = ", a[0];
				msg(), "a1 = ", a[1];
				msg(), "a2 = ", a[2];
				msg(), "b1 = ", a[3];
				msg(), "b2 = ", a[4];
			}
#endif

		void reset()
			{
				v4f_t *x = data() + 5, *y = x + 2;
				x[0] = x[1] = 
				y[0] = y[1] = (v4f_t) {0,0,0,0};
			}

		/* RBJ prototypes */
		void set_bp (v4f_t f, v4f_t Q)
			{
				RBJ4f p (f, Q);
				v4f_t a[3], b[3];

				b[0] = Q * p.alpha;
				b[1] = v4f_0;
				b[2] = -b[0];
				
				a[0] = v4f_1 + p.alpha;
				a[1] = -v4f_2 * p.cos;
				a[2] = v4f_1 - p.alpha;

				make_direct_I (a, b);
			}

		void set_lp (v4f_t f, v4f_t Q)
			{
				RBJ4f p (f, Q);
				v4f_t a[3], b[3];

				b[1] = v4f_1 - p.cos;
				b[0] = b[2] = b[1] * v4f_half;
				
				a[0] = v4f_1 + p.alpha;
				a[1] = -v4f_2 * p.cos;
				a[2] = v4f_1 - p.alpha;

				make_direct_I (a, b);
			}

		void set_eq (v4f_t f, v4f_t Q, v4f_t gain)
			{
				/* A = pow (10, gain / 40) */
				v4f_t A = (v4f_t) {.025,.025,.025,.025};
				A *= gain;
				A = v4f_map<pow10f> (A);

				RBJ4f p (f, Q);

				v4f_t atA = p.alpha * A;
				v4f_t aoA = p.alpha / A;

				v4f_t a[3], b[3];

				a[1] = b[1] = -v4f_2 * p.cos;

				b[0] = v4f_1 + atA;
				b[2] = v4f_1 - atA;

				a[0] = v4f_1 + aoA;
				a[2] = v4f_1 - aoA;
			
				make_direct_I (a, b);
			}

		void make_direct_I (v4f_t * ha, v4f_t * hb)
			{
				v4f_t *a = data(), *b = a + 2;

				v4f_t ha0i = v4f_1 / ha[0];
				
				a[0] = hb[0] * ha0i;
				a[1] = hb[1] * ha0i;
				a[2] = hb[2] * ha0i;

				ha0i = -ha0i;
				b[1] = ha[1] * ha0i;
				b[2] = ha[2] * ha0i;
			}

		/* readable prototype */
		inline v4f_t _process (v4f_t s)
			{
				v4f_t *a = data(), *b = a + 2, *x = a + 5, *y = a + 7;

				register v4f_t r = s * a[0];
				
				r += a[1] * x[h];
				r += b[1] * y[h];

				h ^= 1;
				r += a[2] * x[h];
				r += b[2] * y[h];

				y[h] = r;
				x[h] = s;
				
				return r;
			}

		/* the production version with less pointer arithmetic in the prologue */
		inline v4f_t process (register v4f_t s)
			{
				v4f_t *a = data();

				register v4f_t r = s * a[0];
				
				r += a[1] * a[5+h]; /* a[1] * x[h] */
				r += a[2+1] * a[7+h]; /* b[1] * y[h] */

				h ^= 1;
				r += a[2] * a[5+h]; /* a[2] * x[h] */
				r += a[2+2] * a[7+h]; /* b[2] * y[h] */

				a[5+h] = s; /* x[h] = s */
				a[7+h] = r; /* y[h] = r */
				
				return r;
			}

		/* resonators / bandpass filters have a zero a[1] coefficient, allowing for
		 * optimisation: */
		inline v4f_t process_no_a1 (v4f_t s)
			{
				v4f_t *a = data();

				register v4f_t r = s * a[0];
				
				r += a[2+1] * a[7+h]; /* b[1] * y[h] */

				h ^= 1;
				r += a[2] * a[5+h]; /* a[2] * x[h] */
				r += a[2+2] * a[7+h]; /* b[2] * y[h] */

				a[5+h] = s; /* x[h] = s */
				a[7+h] = r; /* y[h] = r */
				
				return r;
			}

		/* using the parallel structure as four filters in series by 
		 * sequential rotation */
		inline float seriesprocess (float x)
			{
				v4f_t *a = data();
				v4f_t s = a[7+h]; /* y[-1] = last output */
				s = v4f_shuffle (s, 3,0,1,2); 
				v4fa(s)[0] = x;
				s = process(s);
				return v4fa(s)[3];
			}

		/* set the coefficients of one filter only */
		void set_ab (int i, float * ca, float * cb)
			{
				/* map to v4f array address */
				v4f_t *a = data(), *b = a+2;
				/* assign */
				for (int j=0; j<3; ++j)
					((float *) &(a[j]))[i] = ca[j];
				for (int j=1; j<3; ++j)
					((float *) &(b[j]))[i] = cb[j];
			}
}; /* class BiQuad4f */

/* N*4 parallel filters */
template <uint N>
class BiQuad4fBank
{
	public:
		enum { DataSize = (2 + 7*N) * sizeof (v4f_t) };
		/* data layout: x[2] first, then N * (a[3], b[2], y[2]) 
		 * plus 16 extra bytes to ensure sufficient room for alignment */
		char __data [DataSize + sizeof (v4f_t)];
		v4f_t * _data;
		inline v4f_t * data() {return _data;}
		int h1; /* history index */

		BiQuad4fBank()
			{
				_data = (v4f_t*) (((uint64) __data + 16) & ~15ll);
				memset (data(), 0, DataSize);
				h1 = 0;
			}

		void reset()
			{
				v4f_t *x = data();
				x[0] = x[1] = v4f_0;

				x += 2 + 5; /* point to y[] */
				for (uint i = 0; i < N; ++i, x += 7)
					x[0] = x[1] = v4f_0;
			}

		void unity()
			{
				v4f_t *a = data();
				memset (a, 0, DataSize);
				a += 2; /* point to a[] */
				for (uint i = 0; i < N; ++i, a += 7)
					a[0] = v4f_1;
			}

#ifdef __CREAM__
		void dumpab()
			{
				v4f_t *a = data() + 2;
				
				for (uint i = 0; i < N; ++i, a += 7)
				{
					msg(), "a0[", i, "] = ", a[0];
					msg(), "a1[", i, "] = ", a[1];
					msg(), "a2[", i, "] = ", a[2];
					msg(), "b1[", i, "] = ", a[3];
					msg(), "b2[", i, "] = ", a[4];
				}
			}
#endif

		inline v4f_t process (v4f_t s, uint n = N)
			{
				v4f_t *x = data(), *a = x + 2; 

				v4f_t acc = v4f_0;

				register uint h2 = h1 ^ 1;
				for (uint i = 0; i < n; ++i, a += 7)
				{
					register v4f_t r = s * a[0];
					
					r += a[1] * x[h1];
					r += a[2+1] * a[5+h1]; /* b[1] * y[h1] */

					r += a[2] * x[h2];
					r += a[2+2] * a[5+h2]; /* b[2] * y[h2] */

					a[5+h2] = r; /* y[h2] */
					acc += r;
				}

				x[h2] = s;
				h1 = h2;

				return acc;
			}

		inline v4f_t process_no_a0 (v4f_t s)
			{
				v4f_t *x = data(), *a = x + 2; 

				v4f_t acc = v4f_0;

				register uint h2 = h1 ^ 1;
				for (uint i = 0; i < N; ++i, a += 7)
				{
					register v4f_t r;
					
					r = a[1] * x[h1];
					r += a[2+1] * a[5+h1]; /* b[1] * y[h1] */

					r += a[2] * x[h2];
					r += a[2+2] * a[5+h2]; /* b[2] * y[h2] */

					a[5+h2] = r; /* y[h2] */
					acc += r;
				}

				x[h2] = s;
				h1 = h2;

				return acc;
			}

		inline v4f_t process_no_a1 (v4f_t s, uint n = N)
			{
				v4f_t *x = data(), *a = x + 2; 

				v4f_t acc = v4f_0;

				register uint h2 = h1 ^ 1;
				for (uint i = 0; i < n; ++i, a += 7)
				{
					register v4f_t r = s * a[0];
					
					r += a[2+1] * a[5+h1]; /* b[1] * y[h1] */

					r += a[2] * x[h2];
					r += a[2+2] * a[5+h2]; /* b[2] * y[h2] */

					a[5+h2] = r; /* y[h2] */
					acc += r;
				}

				x[h2] = s;
				h1 = h2;

				return acc;
			}


		/* RBJ prototypes */
		void set_bp (v4f_t * f, v4f_t * Q, v4f_t * gain = 0)
			{
				v4f_t * a = data() + 2; 
				for (uint i = 0; i < N; ++i, a += 7)
				{
					RBJ4f p (f[i], Q[i]);
					v4f_t ha[3], hb[3];

					hb[0] = Q[i] * p.alpha;
					hb[1] = v4f_0;
					hb[2] = -hb[0];
					
					ha[0] = v4f_1 + p.alpha;
					ha[1] = -v4f_2 * p.cos;
					ha[2] = v4f_1 - p.alpha;

					make_direct_I (a, ha, hb);

					if (gain) a[0] *= gain[i];
				}
			}

		void set_eq (v4f_t *f, v4f_t *Q, v4f_t *gain)
			{
				v4f_t * a = data() + 2; 
				for (uint i = 0; i < N; ++i, a += 7)
				{
					/* A = pow (10, gain / 40) */
					v4f_t A = (v4f_t) {.025,.025,.025,.025};
					A *= gain[i];
					A = v4f_map<pow10f> (A);

					RBJ4f p (f[i], Q[i]);

					v4f_t atA = p.alpha * A;
					v4f_t aoA = p.alpha / A;

					v4f_t ha[3], hb[3];

					ha[1] = hb[1] = -v4f_2 * p.cos;

					hb[0] = v4f_1 + atA;
					hb[2] = v4f_1 - atA;

					ha[0] = v4f_1 + aoA;
					ha[2] = v4f_1 - aoA;
				
					make_direct_I (a, ha, hb);
				}
			}


		void make_direct_I (v4f_t * a, v4f_t * ha, v4f_t * hb)
			{
				v4f_t *b = a + 2;

				v4f_t ha0i = v4f_1 / ha[0];
				
				a[0] = hb[0] * ha0i;
				a[1] = hb[1] * ha0i;
				a[2] = hb[2] * ha0i;

				ha0i = -ha0i;
				b[1] = ha[1] * ha0i;
				b[2] = ha[2] * ha0i;
			}

		void set_a (uint k, float *c, uint n = N)
			{
				//msg(), k, " = ", (v4f_t) {c[0],c[1],c[2],c[3]}, (v4f_t) {c[4],c[5],c[6],c[7]};
				v4f_t * a = data() + 2 + k; 
				for (uint i = 0; i < n; ++i, a += 7, c += 4)
					*a = (v4f_t) {c[0],c[1],c[2],c[3]};
			}

		void set_b (uint k, float *c, uint n = N) { set_a (k+2, c, n); }

		/* initialise coefficients wholesale */
		void set_ab (uint n, v4f_t *a0, v4f_t *a1, v4f_t *a2, v4f_t *b1, v4f_t *b2)
			{
				v4f_t * a = data() + 2; 
				for (uint i = 0; i < n; ++i, a += 7)
				{
					a[0] = a0[i];
					a[1] = a1[i];
					a[2] = a2[i];
					/* b[1,2] */
					a[2+1] = b1[i];
					a[2+2] = b2[i];
				}
			}

		/* set the coefficients of one filter only */
		void set_ab (int i, float * ca, float * cb)
			{
				/* map to v4f array address */
				int base = i >> 2; i &= 3;
				v4f_t *a = data()+2+base, *b = a+2;
				/* assign */
				for (int j=0; j<3; ++j)
					((float *) &(a[j]))[i] = ca[j];
				for (int j=1; j<3; ++j)
					((float *) &(b[j]))[i] = cb[j];
			}

};

/* 16-byte aligned v4f array */
template <int ItemSize>
class v4fState
{
	public:
		char * _state;
		v4f_t * state;
		v4fState() {_state = 0;}
		~v4fState() {if (_state) free (_state);}

		void init (int n)
			{
				size_t size = (n*ItemSize + 1) * sizeof(v4f_t);
				_state = (char *) malloc (size);
				memset (_state,0,size);
				state = (v4f_t*) (((uint64) _state + 16) & ~15ll);
			}

		inline operator v4f_t * () { return state; }
		inline v4f_t & operator [] (int i) { return state[i]; }
};

/* N*4 parallel resonators */
class Resonator4fBank
{
	public:
		int N;
		/* data layout: N * (Item = (a[1], b[2], y[2], cos))
		cos is stored separately so we can modify the pole radius without
		having to make that costly computation again */
		enum {Item = 6};
		v4fState<Item> state;
		int h1; /* history index */

		void init (int n)
			{
				N = n;
				state.init (n);
				h1 = 0;
			}

		void init (int i, v4f_t f, v4f_t r, v4f_t gain)
			{
				v4f_t * a = state + i*Item;
				f *= v4f_2pi;
				a[0] = v4f_map<__builtin_sinf> (f);
				a[0] *= gain;
				a[5] = v4f_map<__builtin_cosf> (f);
				set_r (i, r);
			}
		void set_r (int i, v4f_t r)
			{
				v4f_t * a = state + i*Item;
				v4f_t b = r;
				b *= v4f_2;
				b *= a[5]; /* cos(2*pi*f) */
				a[1] = b;
				r *= r;
				a[2] = -r;
			}
	
		void reset()
			{
				v4f_t *y = state + 3;

				for (int i = 0; i < N; ++i, y += Item)
					y[0] = y[1] = v4f_0;
			}

	public:
		inline v4f_t process (int i, sample_t x)
			{
				return process (i, (v4f_t) {x,x,x,x});
			}

		inline v4f_t process (int i, v4f_t x)
			{
				v4f_t *a = state + i*Item; 

				register uint h2 = h1 ^ 1;
				x = x * a[0]; /* x * a[0] */
				
				x += a[1] * a[3+h1]; /* b[1] * y[h1] */
				x += a[2] * a[3+h2]; /* b[2] * y[h2] */

				a[3+h2] = x; /* y[h2] */
				return x;
			}

		inline v4f_t y1(int i)
			{
				return *((v4f_t*) state + i*Item + 3 + h1); 
			}


		inline v4f_t processplus (int i, sample_t x)
			{
				v4f_t *a = state + i*Item; 

				v4f_t s = (v4f_t) {x,x,x,x};

				register uint h2 = h1 ^ 1;
				register v4f_t r = s * a[0]; /* x * a[0] */
				
				r += a[1] * a[3+h1]; /* b[1] * y[h1] */
				r += a[2] * a[3+h2]; /* b[2] * y[h2] */

				a[3+h2] = r; /* y[h2] */

				static v4f_t scale2nd = mk_v4f(.1);
				s = r;
				r *= scale2nd;
				r *= r;
				s += r;

				return s;
			}

		inline void step() { h1 ^= 1; }

		#ifdef __CREAM__
		void dumpab()
			{
				v4f_t *a = state;
				
				for (int i = 0; i < N; ++i, a += Item)
				{
					msg(), "a0[", i, "] = ", a[0];
					msg(), "b1[", i, "] = ", a[1];
					msg(), "b2[", i, "] = ", a[2];
				}
			}
		#endif
};

} /* namespace DSP */

#endif /* _DSP_V4F_BI_QUAD_H_ */
