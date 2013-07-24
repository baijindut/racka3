/*
 * Host.cpp
 *
 *  Created on: 20 Jul 2013
 *      Author: slippy
 */

#include "Host.h"
#include <string.h>


#include "plugins/RBEcho.h"
#include "plugins/FuzzTest.h"
#include <stdio.h>

Host::Host()
{
	Plugin* plugin;

	// add all the plugins to the pool
	createPluginIfNeeded("Echoverse",true);
	// TODO: add other plugins

	// loop over pool and create all plugin json list
	_jsonAllPlugins = cJSON_CreateArray();
	for(list<Plugin*>::iterator it=_pool.begin(); it!=_pool.end();++it)
	{
		cJSON* jsonObject = cJSON_CreateObject();
		(*it)->getPluginJson(jsonObject);
		cJSON_AddItemToArray(_jsonAllPlugins,jsonObject);
	}

	// debug
	char* c = cJSON_Print(_jsonAllPlugins);
	printf("%s\n",c);
	free(c);
}


Host::~Host()
{
	list<Plugin*>::iterator it;

	for(it=_plugins.begin(); it!=_plugins.end();++it)
		delete *it;

	for(it=_pool.begin(); it!=_pool.end();++it)
		delete *it;

	cJSON_Delete(_jsonAllPlugins);
}

int Host::process(const float *inputBuffer,
			 float *outputBuffer,
             unsigned long framesPerBuffer,
             const PaStreamCallbackTimeInfo* timeInfo,
             PaStreamCallbackFlags statusFlags)
{
	int i;

	float *inLeft = ((float **) inputBuffer)[0];
	float *inRight = ((float **) inputBuffer)[1];
	float *outLeft = ((float **) outputBuffer)[0];
	float *outRight = ((float **) outputBuffer)[1];

	// copy to input buffer
	memcpy(_bufferLeft[0],inLeft,framesPerBuffer*sizeof(float));
	memcpy(_bufferRight[0],inRight,framesPerBuffer*sizeof(float));

	float* bufLeft1 = _bufferLeft[0];
	float* bufRight1 = _bufferRight[0];
	float* bufLeft2 = _bufferLeft[1];
	float* bufRight2 = _bufferLeft[1];
	float* latestLeftBuf = (float*)inLeft;
	float* latestRightBuf = (float*)inRight;

	if (_plugins.size())
	{
		for(list<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
		{
			// process
			(*it)->process(bufLeft1,bufRight1,bufLeft2,bufRight2,framesPerBuffer);

			// buffer swap
			latestLeftBuf = bufLeft2;
			latestRightBuf = bufRight2;
			bufLeft2 = bufLeft1;
			bufRight2 = bufLeft1;
			bufLeft1 = latestLeftBuf;
			bufRight1 = latestRightBuf;
		}
	}

	// copy to output buffer
	memcpy(outLeft,latestLeftBuf,framesPerBuffer*sizeof(float));
	memcpy(outRight,latestRightBuf,framesPerBuffer*sizeof(float));

	return paContinue;
}

cJSON* Host::getAvailablePlugins()
{
	return _jsonAllPlugins;
}

bool Host::addPlugin(char* name, int before)
{
	bool bOk=false;

	if (_plugins.size()==0 && before < _plugins.size() && before >= 0)
	{
		Plugin* plugin = createPluginIfNeeded(name);
		if (plugin)
		{
			if (_plugins.size())
			{
				list<Plugin*>::iterator it=_plugins.begin();
				for (int i=0;i<before;i++)
					it++;
				_plugins.insert(it,plugin);
			}
			else
			{
				_plugins.push_back(plugin);
			}

			bOk = true;
		}
	}

	return bOk;
}

bool Host::swapPlugin(int from, int to)
{
	bool bOk = false;

	if (from >=0 && from < _plugins.size() &&
		to >=0 && to < _plugins.size() &&
		from!=to && _plugins.size()>=2)
	{
		Plugin* a = _plugins[from];
		Plugin* b = _plugins[to];

		_plugins[to] = a;
		_plugins[from] = b;
	}

	return bOk;
}

bool Host::setPluginParams(cJSON* json)
{
	//TODO: set plugin parameters
}

bool Host::setPluginParam(int index, char* param, int value)
{
	//TODO: set plugin parameter
}

Plugin* Host::createNewPlugin(char* name)
{
	Plugin* plugin =0;

	if (0==strcmp(name,"Echoverse"))
	{
		return new RBEcho();
	}

	return plugin;
}

Plugin* Host::createPluginIfNeeded(char* name,bool addToPoolImmediately)
{
	Plugin* plugin=0;
	list<Plugin*>::iterator it;

	// look in pool
	for(it=_pool.begin(); it!=_pool.end();++it)
	{
		if (0==strcmp(name,(*it)->getName()))
		{
			plugin = *it;
			_pool.remove(plugin);
			break;
		}
	}

	// if not found in pool
	if (!plugin)
	{
		plugin = createNewPlugin(name);
		if (addToPoolImmediately && plugin)
		{
			_pool.push_back(plugin);
		}
	}

	return plugin;
}
