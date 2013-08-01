/*
 * Host.cpp
 *
 *  Created on: 20 Jul 2013
 *      Author: slippy
 */

#include "Host.h"
#include <string.h>


#include "plugins/PluginRBEcho.h"
#include "plugins/PluginChorus.h"
#include "plugins/PluginNoiseGate.h"
#include "plugins/PluginBackingTrack.h"

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
	createPluginIfNeeded("Noise Gate",true);
	createPluginIfNeeded("Backing Track",true);
	// TODO: add other plugins

	// loop over pool and create all plugin json list
	_jsonAllPlugins = cJSON_CreateArray();
	for(vector<Plugin*>::iterator it=_pool.begin(); it!=_pool.end();++it)
	{
		cJSON* jsonObject = cJSON_CreateObject();
		(*it)->getPluginJson(jsonObject);
		cJSON_AddItemToArray(_jsonAllPlugins,jsonObject);
	}

	_chainAction = NULL;
	pthread_mutex_init(&_chainActionMutex,0);
	pthread_mutex_init(&_chainActionCompleted,0);
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

	// any delegated chain action to do? (trylock rather than lock so we never block here)
	if (pthread_mutex_trylock(&_chainActionMutex) == 0)
	{
		// if there is an action
		if (_chainAction)
		{
			delegatedChainAction(_chainAction);
			_chainAction = 0;
		}

		// silence all buffers
		for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
			for (int b=0;b<(*it)->getOutputBufferCount();b++)
				(*it)->getOutputBuffer(b)->silence();

		pthread_mutex_unlock(&_chainActionMutex);
	}

	// keep a note of the final output bufer
	StereoBuffer finalOutput = StereoBuffer(inLeft,inRight,framesPerBuffer);

	// process all plugins
	Plugin* plugin = 0;
	Plugin* previousPlugin = 0;
	bool bFirst = true;
	for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
	{
		StereoBuffer scopedBuffer;

		// get pointer to this plugin
		plugin = *it;

		// determine where it wants to get its source audio from
		if (bFirst)
		{
			// if its the first plugin, it has to get its audio from the input
			scopedBuffer.initWrapper(inLeft,inRight,framesPerBuffer);
			bFirst = false;
		}
		else
		{
			// the plugin has a preference as to where it wants to get its input from
			int sourceInstance = plugin->getDesiredSourceInstance();
			int sourceChannel = plugin->getDesiredSourceChannel();

			if (sourceInstance == -1)
				scopedBuffer.initWrapper(previousPlugin->getOutputBuffer(0));
			else
				scopedBuffer.initWrapper(getPluginFromInstance(sourceInstance)->getOutputBuffer(sourceChannel));
		}

		// process
		plugin->process(&scopedBuffer);

		finalOutput.initWrapper(plugin->getOutputBuffer(0));

		// remember last plugin
		previousPlugin = plugin;
	}

	// copy to output buffer
	memcpy(outLeft,finalOutput.left,framesPerBuffer*sizeof(float));
	memcpy(outRight,finalOutput.right,framesPerBuffer*sizeof(float));

	return paContinue;
}

// these 2 are easy. they are fast, and all the work can happen in the audio thread
void Host::removePlugin(cJSON* json) {invokeChainAction(json,"removeplugin");}
void Host::movePlugin(cJSON* json) {invokeChainAction(json,"moveplugin");}

// add is trickier: we may need to create a new plugin instance if there isnt one in the pool,
// so that we dont do the possibly lengthy plugin construction in the sudio thread
void Host::addPlugin(cJSON* json)
{
	// parse json, and if its all good, ensure there is a plugin instance in the pool
	// so the audio thread never has to allocate one.

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
			if (createPluginIfNeeded(name,true))
			{
				// cause it to be done
				invokeChainAction(json,"addplugin");
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

// blocks until complete
void Host::invokeChainAction(cJSON* json,char* action)
{
	// add action to json, we know what to do later
	cJSON_AddItemToObject(json,"action",cJSON_CreateString(action));

	// this tells the audio thread to do the action
	pthread_mutex_lock(&_chainActionMutex);
	pthread_mutex_lock(&_chainActionCompleted);
	_chainAction = json;
	pthread_mutex_unlock(&_chainActionMutex);

	// sometime now, the audio thread will unlock the completion mutex

	// wait until action is done
	pthread_mutex_lock(&_chainActionCompleted);
	pthread_mutex_unlock(&_chainActionCompleted);
}

void Host::delegatedChainAction(cJSON* json)
{
	cJSON* jsonAction = cJSON_GetObjectItem(json,"action");
	char* action;

	if (jsonAction && jsonAction->type==cJSON_String && jsonAction->string)
	{
		action = jsonAction->valuestring;

		if (strcmp(action,"addplugin")==0)
			delegatedAddPlugin(json);
		if (strcmp(action,"removeplugin")==0)
			delegatedRemovePlugin(json);
		if (strcmp(action,"moveplugin")==0)
			delegatedMovePlugin(json);
	}

	// now the action has been done, unlock the completed mutex to allow waiting thread to fall through
	pthread_mutex_unlock(&_chainActionCompleted);
}

void Host::delegatedAddPlugin(cJSON* json)
{
	// no need to check json is ok - we have already done this
	cJSON* jsonName = cJSON_GetObjectItem(json,"name");
	cJSON* jsonPosition = cJSON_GetObjectItem(json,"position");
	char* name = jsonName->valuestring;
	int before = jsonPosition->valueint;

	Plugin* plugin = createPluginIfNeeded(name);
	plugin->setInstance(_nextInstance++);
	// todo: init plugin to defaults

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

void Host::delegatedRemovePlugin(cJSON* json)
{
	cJSON* jsonInstance = cJSON_GetObjectItem(json,"instance");

	if (jsonInstance && jsonInstance->type==cJSON_Number)
	{
		Plugin* plugin = getPluginFromInstance(jsonInstance->valueint);
		if (plugin)
		{
			_plugins.erase(std::remove(_plugins.begin(), _plugins.end(), plugin),
						   _plugins.end());

			// put the plugin back into the pool
			_pool.push_back(plugin);

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

void Host::delegatedMovePlugin(cJSON* json)
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

void Host::renumberPlugins()
{
	int position =0;
	vector<Plugin*>::iterator it;

	for(it=_plugins.begin(); it!=_plugins.end();++it)
	{
		(*it)->setPosition(position++);
	}

	/// debug
	/*
	printf("------------\n");
	printf("%d in pool, %d in chain\n",_pool.size(),_plugins.size());
	position=0;
	for(it=_plugins.begin(); it!=_plugins.end();++it)
		printf("chain %d: %s\n",position++,(*it)->getName());

	printf("\n");
	position=0;
	for(it=_pool.begin(); it!=_pool.end();++it)
		printf("pool %d: %s\n",position++,(*it)->getName());
	*/

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
		return new PluginRBEcho();
	} else if (0==strcmp(name,"Chorus")) {
		return new PluginChorus();
	} else if (0==strcmp(name,"Noise Gate")) {
		return new PluginNoiseGate();
	} else if (0==strcmp(name,"Backing Track")) {
		return new PluginBackingTrack();
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
			plugin = *it;
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

