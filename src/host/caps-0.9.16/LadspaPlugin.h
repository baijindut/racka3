/*
 * LadspaPlugin.h
 *
 *  Created on: 4 Sep 2013
 *      Author: lenovo
 */

#ifndef LADSPAPLUGIN_H_
#define LADSPAPLUGIN_H_

class LadspaPlugin
{
	public:
		float fs, over_fs; /* sample rate and 1/fs */
		float adding_gain; /* for run_adding() */

		int first_run; /* 1st block after activate(), do no parameter smoothing */
		sample_t normal; /* renormal constant */

		sample_t ** ports;
		LADSPA_PortRangeHint * ranges; /* for getport() below */

	public:
		/* get port value, mapping inf or nan to 0 */
		inline sample_t getport_unclamped (int i)
			{
				sample_t v = *ports[i];
				return (isinf (v) || isnan(v)) ? 0 : v;
			}

		/* get port value and clamp to port range */
		inline sample_t getport (int i)
			{
				LADSPA_PortRangeHint & r = ranges[i];
				sample_t v = getport_unclamped (i);
				return clamp (v, r.LowerBound, r.UpperBound);
			}
};



#endif /* LADSPAPLUGIN_H_ */
