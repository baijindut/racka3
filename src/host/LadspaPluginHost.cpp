/*
 * LadspaPluginHost.cpp
 *
 *  Created on: 4 Sep 2013
 *      Author: lenovo
 */

#include "LadspaPluginHost.h"


LadspaPluginWrapper::LadspaPluginWrapper(DescriptorLadspaPlugin* ladspaPlugin)
{
	_p = ladspaPlugin;

	int nParamCount = 0;
	while (_p->ranges[nParamCount]) nParamCount++;

	
	// get parameters from the ladspa plugin and register them
	registerPlugin(1,)

}

LadspaPluginWrapper::~LadspaPluginWrapper()
{
	delete _p;
}

int LadspaPluginWrapper::process(StereoBuffer* input)
{

}

void LadspaPluginWrapper::setParam(int npar, int value)
{

}

int LadspaPluginWrapper::getParam(int npar)
{

}

