/*
 * PluginFreqSplitter.h
 *
 *  Created on: 6 Aug 2013
 *      Author: lenovo
 */

#ifndef PLUGINFREQSPLITTER_H_
#define PLUGINFREQSPLITTER_H_

#include "Plugin.h"
#include "AnalogFilter.h"

class PluginXOver: public Plugin {
public:
	PluginXOver();
	virtual ~PluginXOver();

	int process(StereoBuffer* input);

	void panic();

private:
    void setParam (int npar, int value);
    int getParam (int npar);

private:
    int _nSplitFreq;

    AnalogFilter *lpfl, *lpfr, *hpfl, *hpfr;
};

#endif /* PLUGINMIXSPLITTER_H_ */
