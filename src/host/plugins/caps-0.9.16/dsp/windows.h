/*
	dsp/windows.h
	
	Copyright 2004-12 Tim Goetze <tim@quitte.de>
	
	http://quitte.de/dsp/

	Window functions.

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

#ifndef __DSP_WINDOWS_H__ 
#define __DSP_WINDOWS_H__

namespace DSP {
	
/* prototypes for window value application ... */
typedef void (*window_sample_func_t) (sample_t &, sample_t);

/* ... which go as template parameters for the window calculation below */
inline void
store_sample (sample_t & d, sample_t s)
{
	d = s;
}

inline void
apply_window (sample_t &d, sample_t s)
{
	d *= s;
}

template <window_sample_func_t F>
void
hann (sample_t * s, int n, double step = 1)
{
	step = step / n;
	
	for (int i = 0; i < n; ++i)
	{
		register double f = i * step;
		F (s[i], .5 - .5 * cos (2 * M_PI * f));
	}
}

template <window_sample_func_t F>
void
hamming (sample_t * s, int n)
{
	register float in = 1. / n;
	
	for (int i = 0; i < n; ++i)
	{
		register double f = i * in;
		F (s[i], .54 - .46 * cos (2 * M_PI * f));
	}
}

template <window_sample_func_t F>
void
blackman (sample_t * s, int n)
{
	register float in = 1. / n;

	for (int i = 0; i < n; ++i)
	{
		register float f = (float) i;

		register double b = .42f - 
						.5f * cos (2.f * f * M_PI * in) + 
						.08 * cos (4.f * f * M_PI * in);

		F (s[i], b);
	}
}

template <window_sample_func_t F>
void
blackman_harris (sample_t * s, int n)
{
	register double w1 = 2.f * M_PI / (n - 1);
	register double w2 = 2.f * w1;
	register double w3 = 3.f * w1;

	for (int i = 0; i < n; ++i)
	{
		register double f = (double) i;

		register double bh = .35875f - 
				.48829f * cos (w1 * f) + 
				.14128f * cos (w2 * f) - 
				.01168f * cos (w3 * f);

		F (s[i], bh);
	}
}

/* helper for the kaiser window, courtesy of R. Dobson, courtesy of csound */
inline double 
besseli (double x)
{
	double ax, ans;
	double y;

	if ((ax = fabs (x)) < 3.75)     
	{
		y = x / 3.75;
		y *= y;
		ans = (1.0 + y * (3.5156229 +
			y * (3.0899424 +
				y * (1.2067492 +
					y * (0.2659732 +
						y * (0.360768e-1 +
							y * 0.45813e-2))))));
	}
	else 
	{
		y = 3.75 / ax;
		ans = ((exp (ax) / sqrt (ax))
			* (0.39894228 +
				y * (0.1328592e-1 +
					y * (0.225319e-2 +
						y * (-0.157565e-2 +
							y * (0.916281e-2 +
								y * (-0.2057706e-1 +
									y * (0.2635537e-1 +
										y * (-0.1647633e-1 +
											y * 0.392377e-2)))))))));
	}

	return ans;
}

/* step = .5 : window [-n to 0] */
template <window_sample_func_t F>
void
kaiser (sample_t * s, int n, double beta, double step = 1)
{
	double bb = besseli (beta);
	int si = 0;

	for (double i = -n / 2 + .1; si < n; ++si, i += step)
	{
		double k = besseli ((beta * sqrt (1 - pow ((2 * i / (n - 1)), 2)))) / bb;

		/* can you spell hack */
		if (!finite (k) || isnan(k))
			k = 0;

		F (s[si], k);
	}
}

}; /* namespace DSP */

#endif /* __DSP_WINDOWS_H__ */
