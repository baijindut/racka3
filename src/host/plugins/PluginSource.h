/*
 * PluginMixSplitter.h
 *
 *  Created on: 6 Aug 2013
 *      Author: lenovo
 */

#ifndef PLUGINSOURCE_H_
#define PLUGINSOURCE_H_

#include "Plugin.h"

class PluginSource: public Plugin {
public:
	PluginSource();
	virtual ~PluginSource();

	int process(StereoBuffer* input);

	void panic();

private:
    void setParam (int npar, int value);
    int getParam (int npar);
};

#endif /* PLUGINMIXSPLITTER_H_ */
