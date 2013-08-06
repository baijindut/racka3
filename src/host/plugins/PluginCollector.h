/*
 * PluginMixSplitter.h
 *
 *  Created on: 6 Aug 2013
 *      Author: lenovo
 */

#ifndef PLUGINCOLLECTOR_H_
#define PLUGINCOLLECTOR_H_

#include "Plugin.h"

class PluginCollector: public Plugin {
public:
	PluginCollector();
	virtual ~PluginCollector();

	int process(StereoBuffer* input);

	void panic();

private:
    void setParam (int npar, int value);
    int getParam (int npar);
};

#endif /* PLUGINMIXSPLITTER_H_ */
