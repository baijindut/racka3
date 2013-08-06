/*
 * PluginSource.cpp
 *
 *  Created on: 6 Aug 2013
 *      Author: lenovo
 */

#include "PluginSource.h"

PluginSource::PluginSource()
{
	_type = PLUGIN_SOURCE;

	registerPlugin(2,"Channel B","Emits Channel B signal from associated splitter",1);
}

PluginSource::~PluginSource()
{

}

int PluginSource::process(StereoBuffer* input)
{
	// should never be called
	return paContinue;
}

void PluginSource::panic()
{
}

void PluginSource::setParam(int npar, int value)
{
}

int PluginSource::getParam(int npar)
{
}
