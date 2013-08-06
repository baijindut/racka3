/*
 * PluginMixSplitter.h
 *
 *  Created on: 6 Aug 2013
 *      Author: lenovo
 */

#ifndef PLUGINMIXSPLITTER_H_
#define PLUGINMIXSPLITTER_H_

#include "Plugin.h"

class PluginMixSplitter: public Plugin {
public:
	PluginMixSplitter();
	virtual ~PluginMixSplitter();

	int process(StereoBuffer* input);

	void panic();

private:
    void setParam (int npar, int value);
    int getParam (int npar);

private:
    int _nSplit;
    float _fA,_fB;
};

#endif /* PLUGINMIXSPLITTER_H_ */
