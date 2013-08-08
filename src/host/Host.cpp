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
#include "plugins/PluginCompressor.h"
#include "plugins/PluginMixSplitter.h"
#include "plugins/PluginSource.h"
#include "plugins/PluginCollector.h"

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
	createPluginIfNeeded("Compressor",true);
	createPluginIfNeeded("Mix Splitter",true);
	// TODO: add other plugins

	// loop over pool and create all plugin json list
	_jsonAllPlugins = cJSON_CreateArray();
	for(vector<Plugin*>::iterator it=_pool.begin(); it!=_pool.end();++it)
	{
		cJSON* jsonObject = cJSON_CreateObject();
		(*it)->getPluginJson(jsonObject);
		cJSON_AddItemToArray(_jsonAllPlugins,jsonObject);
	}

	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&_chainSpinner, &Attr);
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

int Host::process(float* inLeft,float* inRight,float* outLeft,float* outRight,
             unsigned long framesPerBuffer,
             const PaStreamCallbackTimeInfo* timeInfo,
             PaStreamCallbackFlags statusFlags)
{
	int i;

	// if we cannot get the lock, someone is someone doing a chain-related action.
	if (pthread_mutex_trylock(&_chainSpinner) !=0 )
	{
		// no processing, no output.
		for (int i=0;i<framesPerBuffer;i++)
			outLeft[i]=outRight[i]=0.0;
	}
	else
	{
		// it is safe to do the actual processing

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
				scopedBuffer.initWrapper(previousPlugin->getOutputBuffer(0));
			}

			// make the plugin process whatever input
			plugin->master(&scopedBuffer);

			finalOutput.initWrapper(plugin->getOutputBuffer(0));

			// remember last plugin
			previousPlugin = plugin;
		}

		// copy to output buffer
		memcpy(outLeft,finalOutput.left,framesPerBuffer*sizeof(float));
		memcpy(outRight,finalOutput.right,framesPerBuffer*sizeof(float));

		pthread_mutex_unlock(&_chainSpinner);
	}

	return paContinue;
}

void Host::addPlugin(cJSON* json)
{
	// parse json, and if its all good, ensure there is a plugin instance in the pool
	// so the audio thread never has to allocate one.

	char* name;
	int before;
	cJSON* jsonName = cJSON_GetObjectItem(json,"name");
	cJSON* jsonPosition = cJSON_GetObjectItem(json,"position");

	// get values from JSON
	if (jsonName && jsonName->type==cJSON_String && jsonName->valuestring &&
		jsonPosition && jsonPosition->type==cJSON_Number)
	{
		name = jsonName->valuestring;
		before = jsonPosition->valueint;

		// OOB check
		if ( (_plugins.size()==0 && before == 0) || (before < _plugins.size() && before >= 0))
		{
			// create the instance
			Plugin* plugin = createPluginIfNeeded(name);
			if (plugin)
			{
				plugin->setInstance(_nextInstance++);

				chainLock();

				// add to the chain in the appropriate way
				if (_plugins.size())
				{
					// move to the position in the chain
					vector<Plugin*>::iterator it=_plugins.begin();
					for (int i=0;i<before;i++)
						it++;

					// is this a splitter?
					if (plugin->getType()==PLUGIN_SPLITTER)
					{
						// add collector and source
						Plugin* collector = new PluginCollector();
						collector->setFriend(plugin->getInstance());
						collector->setInstance(_nextInstance++);
						Plugin* source = new PluginSource();
						source->setFriend(plugin->getInstance());
						source->setInstance(_nextInstance++);
						plugin->setFriend(collector->getInstance());

						Plugin* arr[3] = {plugin,source,collector};
						_plugins.insert(it,arr,arr+3 );
					}
					else
					{
						_plugins.insert(it,plugin);
					}
				}
				else
				{
					_plugins.push_back(plugin);

					// is this a splitter?
					if (plugin->getType()==PLUGIN_SPLITTER)
					{
						// add collector and source
						Plugin* collector = new PluginCollector();
						collector->setFriend(plugin->getInstance());
						collector->setInstance(_nextInstance++);
						Plugin* source = new PluginSource();
						source->setFriend(plugin->getInstance());
						source->setInstance(_nextInstance++);
						plugin->setFriend(collector->getInstance());

						_plugins.push_back(source);
						_plugins.push_back(collector);
					}
				}

				chainUnlock();

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

void Host::removePlugin(cJSON* json)
{
	cJSON* jsonInstance = cJSON_GetObjectItem(json,"instance");

	if (jsonInstance && jsonInstance->type==cJSON_Number)
	{
		Plugin* plugin = getPluginFromInstance(jsonInstance->valueint);
		if (plugin)
		{
			if (plugin->getType()==PLUGIN_PROCESSOR ||
				plugin->getType()==PLUGIN_SPLITTER)
			{

				chainLock();

				if (plugin->getType()==PLUGIN_PROCESSOR)
				{
					_plugins.erase(std::remove(_plugins.begin(), _plugins.end(), plugin),
								   _plugins.end());

					// put the plugin back into the pool
					_pool.push_back(plugin);
				}
				else if (plugin->getType()==PLUGIN_SPLITTER)
				{
					int startPos = plugin->getPosition();
					int endPos = getPluginFromInstance(plugin->getFriend())->getPosition();

					// delete / move plugins to pool
					for (int i=startPos;i<=endPos;i++)
					{
						Plugin* remove = _plugins[i];
						if (remove->getType()==PLUGIN_PROCESSOR ||
							remove->getType()==PLUGIN_SPLITTER)
							_pool.push_back(remove);
						else
							delete remove;
					}

					// erase entries from chain
					_plugins.erase(_plugins.begin()+startPos,_plugins.begin()+endPos+1);
				}

				chainUnlock();
			}
			else
			{
				JSONERROR(json,"must delete parent splitter");
			}
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
			int minPos=0;
			int maxPos=0;

			// enforce bounds
			switch (plugin->getType())
			{
			case PLUGIN_PROCESSOR:	// can move anywhere
				minPos = 0;
				maxPos = _plugins.size();
				break;
			case PLUGIN_SPLITTER:	// cannot move below its  associated channel b
			{
				Plugin* source = findPluginFromFriendAndType(plugin->getInstance(),PLUGIN_SOURCE);
				maxPos = source->getPosition()-1;
				break;
			}
			case PLUGIN_SOURCE:	// cannot move above splitter or below collector
			{
				Plugin* splitter = getPluginFromInstance(plugin->getFriend());
				Plugin* collector = getPluginFromInstance(splitter->getFriend());
				minPos = splitter->getPosition()+1;
				maxPos = collector->getPosition()-1;
				break;
			}
			case PLUGIN_COLLECTOR: // cannot move above channelB
			{
				Plugin* source = findPluginFromFriendAndType(plugin->getFriend(),PLUGIN_SOURCE);
				minPos= source->getPosition()+1;
				maxPos = _plugins.size();
				break;
			}
			default:
				break;
			}

			if (to >= minPos && to <= maxPos)
			{
				chainLock();

				if (from >=0 && from < _plugins.size() &&
					to >=0 && to < _plugins.size() &&
					from!=to && _plugins.size()>=2)
				{
					Plugin* a = _plugins[from];
					Plugin* b = _plugins[to];

					_plugins[to] = a;
					_plugins[from] = b;
				}

				chainUnlock();
			}
			else
			{
				JSONERROR(json,"attempting to move OOB");
			}
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

	printf("------------\n");
	printf("%d in pool, %d in chain\n",_pool.size(),_plugins.size());
	position=0;
	for(it=_plugins.begin(); it!=_plugins.end();++it)
		printf("chain %d: %s\n",position++,(*it)->getName());

	printf("\n");
	position=0;
	for(it=_pool.begin(); it!=_pool.end();++it)
		printf("pool %d: %s\n",position++,(*it)->getName());


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
		plugin = new PluginRBEcho();
	} else if (0==strcmp(name,"Chorus")) {
		plugin = new PluginChorus();
	} else if (0==strcmp(name,"Noise Gate")) {
		plugin = new PluginNoiseGate();
	} else if (0==strcmp(name,"Backing Track")) {
		plugin = new PluginBackingTrack();
	} else if (0==strcmp(name,"Compressor")) {
		plugin = new PluginCompressor();
	} else if (0==strcmp(name,"Mix Splitter")) {
		plugin = new PluginMixSplitter();
	} else if (0==strcmp(name,"Channel B")) {
		plugin = new PluginSource();
	} else if (0==strcmp(name,"Collector")) {
		plugin = new PluginCollector();
	}

	if (plugin)
	{
		// processing plugins must have a dry/wet mix
		if (plugin->getType()==PLUGIN_PROCESSOR)
		{
			plugin->registerParam(PARAM_MIX,"Mix","Dry/Wet Mix","","Dry","Wet",0,127,1,127);
		}

		//
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
		plugin->panic();
		if (addToPoolImmediately && plugin)
		{
			_pool.push_back(plugin);
		}
	}

	return plugin;
}

void Host::chainLock()
{
	pthread_mutex_lock(&_chainSpinner);
}

void Host::chainUnlock()
{
	for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
		(*it)->panic();

	renumberPlugins();
	pthread_mutex_unlock(&_chainSpinner);
}

Plugin* Host::findPluginFromFriendAndType(int friendInstance,PluginType type)
{
	for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
	{
		Plugin* plugin = *it;
		if (plugin->getFriend() == friendInstance &&
			plugin->getType() == type)
		{
			return plugin;
		}
	}
	return 0;
}
