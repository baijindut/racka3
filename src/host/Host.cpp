/*
 * Host.cpp
 *
 *  Created on: 20 Jul 2013
 *      Author: slippy
 */

#include "Host.h"
#include <string.h>


#include "plugins/RBEcho.h"
#include "plugins/Chorus.h"

#include <stdio.h>
#include <algorithm>
using namespace std;

#define JSONERROR(JSON,ERROR) do {cJSON_AddItemToObject(JSON,"error",cJSON_CreateString(ERROR));} while(0)

Host::Host()
{
	Plugin* plugin;
	_nextInstance=0;

	// add all the plugins to the pool
	createPluginIfNeeded("Echoverse",true);
	createPluginIfNeeded("Chorus",true);
	// TODO: add other plugins

	// loop over pool and create all plugin json list
	_jsonAllPlugins = cJSON_CreateArray();
	for(vector<Plugin*>::iterator it=_pool.begin(); it!=_pool.end();++it)
	{
		cJSON* jsonObject = cJSON_CreateObject();
		(*it)->getPluginJson(jsonObject);
		cJSON_AddItemToArray(_jsonAllPlugins,jsonObject);
	}

	_bBypassAll = false;
}


Host::~Host()
{
	vector<Plugin*>::iterator it;

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

	if (!_bBypassAll && _plugins.size() )
	{
		for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
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

void Host::getAvailablePlugins(cJSON* json)
{
	cJSON_AddItemReferenceToObject(json,"plugins",_jsonAllPlugins);
}

void Host::getPluginChain(cJSON* json)
{
	cJSON* pluginArray = cJSON_CreateArray();

	for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
	{
		cJSON* pluginObject = cJSON_CreateObject();
		(*it)->getPluginJson(pluginObject);
		cJSON_AddItemToArray(pluginArray,pluginObject);
	}

	cJSON_AddItemToObject(json,"plugins",pluginArray);
}

void Host::removePlugin(cJSON* json)
{
	cJSON* jsonInstance = cJSON_GetObjectItem(json,"instance");

	if (jsonInstance && jsonInstance->type==cJSON_Number)
	{
		Plugin* plugin = getPluginFromInstance(jsonInstance->valueint);
		if (plugin)
		{
			_plugins.erase(std::remove(_plugins.begin(), _plugins.end(), plugin),
						   _plugins.end());

			// renumber plugins since we could very well have removed one in the middle
			renumberPlugins();
		}
		else
		{
			JSONERROR(json,"invalid instance");
		}
	}
	else
	{
		JSONERROR(json,"instance not given");
	}
}

void Host::addPlugin(cJSON* json)
{
	char* name;
	int before;

	cJSON* jsonName = cJSON_GetObjectItem(json,"name");
	cJSON* jsonPosition = cJSON_GetObjectItem(json,"position");

	if (jsonName && jsonName->type==cJSON_String && jsonName->valuestring &&
		jsonPosition && jsonPosition->type==cJSON_Number)
	{
		name = jsonName->valuestring;
		before = jsonPosition->valueint;

		if ( (_plugins.size()==0 && before == 0) || (before < _plugins.size() && before >= 0))
		{
			Plugin* plugin = createPluginIfNeeded(name);
			if (plugin)
			{
				plugin->setInstance(_nextInstance++);
				if (_plugins.size())
				{
					vector<Plugin*>::iterator it=_plugins.begin();
					for (int i=0;i<before;i++)
						it++;
					_plugins.insert(it,plugin);
				}
				else
				{
					_plugins.push_back(plugin);
				}

				// renumber plugins before we return potentially incorrect json
				renumberPlugins();

				// add all plugin details to response - remove name.
				cJSON_DeleteItemFromObject(json,"name");
				plugin->getPluginJson(json);
			}
			else
			{
				JSONERROR(json,"no such plugin name");
			}
		}
		else
		{
			JSONERROR(json,"position is OOB");
		}
	}
	else
	{
		JSONERROR(json,"name or position incorrectly or not given");
	}
}

void Host::movePlugin(cJSON* json)
{
	int from;
	int to;
	int instance;

	cJSON* jsonInstance = cJSON_GetObjectItem(json,"instance");
	cJSON* jsonPosition = cJSON_GetObjectItem(json,"position");

	if (jsonInstance && jsonInstance->type==cJSON_Number &&
		jsonPosition && jsonPosition->type==cJSON_Number)
	{
		Plugin* plugin = getPluginFromInstance(jsonInstance->valueint);

		from = plugin->getPosition();
		to = jsonPosition->valueint;

		if (plugin)
		{
			if (from >=0 && from < _plugins.size() &&
				to >=0 && to < _plugins.size() &&
				from!=to && _plugins.size()>=2)
			{
				Plugin* a = _plugins[from];
				Plugin* b = _plugins[to];

				_plugins[to] = a;
				_plugins[from] = b;
			}

			renumberPlugins();
		}
		else
		{
			JSONERROR(json,"no such plugin instance");
		}

	}
	else
	{
		JSONERROR(json,"instance and or position not specified");
	}
}

void Host::renumberPlugins()
{
	int position =0;
	for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
	{
		(*it)->setPosition(position++);
	}
}

Plugin* Host::getPluginFromInstance(int instance)
{
	// TODO: use a hashmap of instances for faster lookup

	for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
	{
		if ((*it)->getInstance()==instance)
			return *it;
	}
	return 0;
}

void Host::setPluginParam(cJSON* json)
{
	cJSON* jsonInstance = cJSON_GetObjectItem(json,"instance");
	cJSON* jsonParam = cJSON_GetObjectItem(json,"param");
	cJSON* jsonValue = cJSON_GetObjectItem(json,"value");

	if (jsonInstance && jsonInstance->type==cJSON_Number &&
		jsonParam && jsonParam->type==cJSON_String && jsonParam->valuestring &&
		jsonValue && jsonValue->type==cJSON_Number)
	{
		Plugin* plugin = getPluginFromInstance(jsonInstance->valueint);
		if (plugin)
		{
			if (!plugin->setParam(json))
			{
				JSONERROR(json,"bad parameter name or value");
			}
		}
		else
		{
			JSONERROR(json,"bad instance value");
		}
	}
	else
	{
		JSONERROR(json,"incorrect parameters specified");
	}
}

Plugin* Host::createNewPlugin(char* name)
{
	Plugin* plugin =0;

	if (0==strcmp(name,"Echoverse")) {
		return new RBEcho();
	} else if (0==strcmp(name,"Chorus")) {
		return new Chorus();
	}

	return plugin;
}

Plugin* Host::createPluginIfNeeded(char* name,bool addToPoolImmediately)
{
	Plugin* plugin=0;
	vector<Plugin*>::iterator it;

	// look in pool
	for(it=_pool.begin(); it!=_pool.end();++it)
	{
		if (0==strcmp(name,(*it)->getName()))
		{
			_pool.erase(it);
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

