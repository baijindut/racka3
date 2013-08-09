/*
 * PluginCollector.cpp
 *
 *  Created on: 6 Aug 2013
 *      Author: lenovo
 */

#include "PluginCollector.h"

PluginCollector::PluginCollector()
{
	_type = PLUGIN_COLLECTOR;

	registerPlugin(1,"Collector","Collects and mixes Channel A and B from associated Splitter",1);
}

PluginCollector::~PluginCollector()
{

}

int PluginCollector::process(StereoBuffer* input)
{
	// should never be called.)
	return paContinue;
}

void PluginCollector::panic()
{
}

void PluginCollector::setParam(int npar, int value)
{
}

int PluginCollector::getParam(int npar)
{
}
