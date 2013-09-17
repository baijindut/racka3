/*
 * Host.cpp
 *
 *  Created on: 20 Jul 2013
 *      Author: slippy
 */

#include "Host.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>


#include "plugins/PluginRBEcho.h"
#include "plugins/PluginChorus.h"
#include "plugins/PluginNoiseGate.h"
#include "plugins/PluginBackingTrack.h"
#include "plugins/PluginCompressor.h"
#include "plugins/PluginMixSplitter.h"
#include "plugins/PluginSource.h"
#include "plugins/PluginCollector.h"
#include "plugins/PluginXOverSplitter.h"
#include "plugins/PluginArpie.h"
#include "plugins/PluginGrain.h"
#include "plugins/PluginReverb.h"
#include "plugins/PluginEQ.h"
#include "plugins/PluginExpander.h"
#include "CAPSPluginWrapper.h"
#include "plugins/PluginFreezer.h"
#include "plugins/PluginStompBox.h"
#include "plugins/PluginRing.h"
#include "plugins/PluginVibe.h"
#include "plugins/PluginOpticalTrem.h"

#include <stdio.h>
#include <algorithm>
using namespace std;

#define JSONERROR(JSON,ERROR) do {cJSON_AddItemToObject(JSON,"error",cJSON_CreateString(ERROR));} while(0)

Host::Host()
{
	Plugin* plugin;
	_nextInstance=0;

	// add names of all plugins to list
	_pluginNames.push_back("Echoverse");
	_pluginNames.push_back("Chorus");
	_pluginNames.push_back("Noise Gate");
	_pluginNames.push_back("Backing Track");
	_pluginNames.push_back("Compressor");
	_pluginNames.push_back("Mix Splitter");
	_pluginNames.push_back("Crossover Splitter");
	_pluginNames.push_back("Arpie");
	_pluginNames.push_back("Grain");
	_pluginNames.push_back("Reverb");
	_pluginNames.push_back("EQ");
	_pluginNames.push_back("Expander");

	_pluginNames.push_back(CAPS"AmpVTS");
	_pluginNames.push_back(CAPS"AutoFilter");
	_pluginNames.push_back(CAPS"CabinetIV");
	_pluginNames.push_back(CAPS"ChorusI");
	_pluginNames.push_back(CAPS"CompressX2");
	_pluginNames.push_back(CAPS"Eq10X2");
	_pluginNames.push_back(CAPS"NoiseGate");
	_pluginNames.push_back(CAPS"Narrower");
	_pluginNames.push_back(CAPS"Wider");
	_pluginNames.push_back(CAPS"PhaserII");
	_pluginNames.push_back(CAPS"PlateX2");
	_pluginNames.push_back(CAPS"SpiceX2");
	_pluginNames.push_back(CAPS"Scape");

	_pluginNames.push_back("Freezer");
	_pluginNames.push_back("StompBox");
	_pluginNames.push_back("RingMod");
	_pluginNames.push_back("Vibe");
	_pluginNames.push_back("Optical Trem");

	// loop over all names and create all plugin json list
	_jsonAllPlugins = cJSON_CreateArray();
	for(vector<string>::iterator it=_pluginNames.begin(); it!=_pluginNames.end();++it)
	{
		cJSON* jsonObject = cJSON_CreateObject();
		Plugin* plugin = createNewPlugin(*it);
		if (plugin)
		{
			plugin->getPluginJson(jsonObject);
			delete plugin;
			cJSON_AddItemToArray(_jsonAllPlugins,jsonObject);
		}
	}

//	pthread_mutexattr_t Attr;
//	pthread_mutexattr_init(&Attr);
//	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
//	pthread_mutex_init(&_chainSpinner, &Attr);
	pthread_mutex_init(&_chainSpinner,0);
}

Host::~Host()
{
	vector<Plugin*>::iterator it;

	for(it=_plugins.begin(); it!=_plugins.end();++it)
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

		// loop over plugins
		for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
		{
			// this will hold the buffer to input to the current plugin
			StereoBuffer inputBuffer;

			// get pointer to this plugin
			plugin = *it;

			// 1. determine where it wants to get its source audio from
			if (plugin->getPosition()==0)
			{
				// if its the first plugin, it has to get its audio from the input
				inputBuffer.initWrapper(inLeft,inRight,framesPerBuffer);
			}
			else
			{
				// most plugins just get audio from the previous plugin...
				if (previousPlugin->getType() != PLUGIN_SOURCE)
				{
					// .. get output buffer of previous plugin
					inputBuffer.initWrapper(previousPlugin->getOutputBuffer(0));
				}
				else // EXCEPT if the previous plugin is a CHANNEL B (source) plugin.
				{
					// get channel B buffer from associated splitter
					Plugin* splitter = getPluginFromInstance(previousPlugin->getFriend());
					inputBuffer.initWrapper(splitter->getOutputBuffer(1));
				}
			}

			// 2. processing.
			// Most plugins operate on one stereo buffer
			if (plugin->getType()!=PLUGIN_COLLECTOR)
			{
				// process the input buffer
				plugin->master(&inputBuffer);
			}
			else
			{
				// collector must get the channel B and A and do a mix.
				// channel B is easy; it is just the output of the previous plugin.
				// channel A is the output of the plugin just before the Source plugin
				// (which is effectively the channel A collector.)
				StereoBuffer* channelABuffer;
				Plugin* source = findPluginFromFriendAndType(plugin->getFriend(),PLUGIN_SOURCE);
				Plugin* before = _plugins[source->getPosition()-1];

				plugin->master(before->getOutputBuffer(0),&inputBuffer);
			}

			// store the output of the last plugin we process and remember last plugin
			finalOutput.initWrapper(plugin->getOutputBuffer(0));
			previousPlugin = plugin;
		}

		// copy to output buffer with emergency limit
		for (i=0;i<framesPerBuffer;i++)
		{
			outLeft[i] = finalOutput.left[i] > 0.9999999 ? 0.9999999 : finalOutput.left[i];
			outRight[i] = finalOutput.right[i] > 0.9999999 ? 0.9999999 : finalOutput.right[i];
		}

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
			Plugin* plugin = createNewPlugin(string(name));
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

					delete plugin;
				}
				else if (plugin->getType()==PLUGIN_SPLITTER)
				{
					int startPos = plugin->getPosition();
					int endPos = getPluginFromInstance(plugin->getFriend())->getPosition();

					// delete / move plugins to pool
					for (int i=startPos;i<=endPos;i++)
					{
						Plugin* remove = _plugins[i];
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

int Host::getPluginChainSize()
{
	return _plugins.size();
}

void Host::getPluginChain(cJSON* json,bool bVerbose)
{
	cJSON* pluginArray = cJSON_CreateArray();

	for(vector<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
	{
		cJSON* pluginObject = cJSON_CreateObject();
		(*it)->getPluginJson(pluginObject,bVerbose);
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

bool Host::setPluginPreset(Plugin* plugin,int nPreset)
{
	bool ret = false;

	PluginParam* presetParam = plugin->getRegisteredParam("preset");

	if (presetParam && nPreset>=0 && nPreset<presetParam->labels.size())
	{
		// get preset file
		JsonFile* jsfile = new JsonFile( string("presets/")+string(plugin->getName()) );
		cJSON* presets = cJSON_GetObjectItem(jsfile->json(),"presets");
		if (presets)
		{
			// get this particular preset
			const char* presetName = presetParam->labels[nPreset].c_str();
			cJSON* preset = cJSON_GetObjectItem(presets,presetName);
			if (preset)
			{
				// set parameters
				ret = plugin->setParams(preset);

				// remember what preset we just selected, to make the interface make more sense
				plugin->setPresetNumber(nPreset);
			}
		}

		delete jsfile;
	}

	return ret;
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
			// if this is the special 'preset' parameter, special handling
			if (strcmp(jsonParam->valuestring,"preset")==0)
			{
				if (!setPluginPreset(plugin,jsonValue->valueint)) {
					JSONERROR(json,"bad preset index");
				}
				else {
					// tell client to refresh (get values of all parameters for this plugin)
					cJSON_AddItemToObject(json,"refresh",cJSON_CreateBool(1));
				}
			}
			else if (!plugin->setParam(json))
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

Plugin* Host::createNewPlugin(string name)
{
	Plugin* plugin =0;

	if (name == "Echoverse") {
		plugin = new PluginRBEcho();
	} else if (name=="Chorus") {
		plugin = new PluginChorus();
	} else if (name=="Noise Gate") {
		plugin = new PluginNoiseGate();
	} else if (name=="Backing Track") {
		plugin = new PluginBackingTrack();
	} else if (name=="Compressor") {
		plugin = new PluginCompressor();
	} else if (name=="Mix Splitter") {
		plugin = new PluginMixSplitter();
	} else if (name=="Channel B") {
		plugin = new PluginSource();
	} else if (name=="Collector") {
		plugin = new PluginCollector();
	} else if (name=="Crossover Splitter") {
		plugin = new PluginXOver();
	} else if (name=="Arpie") {
		plugin = new PluginArpie();
	} else if (name=="Grain") {
		plugin = new PluginGrain();
	} else if (name=="Reverb") {
		plugin = new PluginReverb();
	} else if (name=="EQ") {
		plugin = new PluginEQ();
	} else if (name=="Expander") {
		plugin = new PluginExpander();
	}
	else if (name.substr(0,strlen(CAPS))==CAPS) {
		plugin = new CAPSPluginWrapper();
		if (!((CAPSPluginWrapper*)plugin)->loadCapsPlugin(name) ) {
			delete plugin; plugin=0;
		}
	} else if (name=="Freezer") {
		plugin = new PluginFreezer();
	} else if (name=="StompBox") {
		plugin = new PluginStompBox();
	} else if (name=="RingMod") {
		plugin = new PluginRing();
	} else if (name=="Vibe") {
		plugin = new PluginVibe();
	} else if (name=="Optical Trem") {
		plugin = new PluginOpticalTrem();
	}

	// do extra stuff for plugin
	if (plugin)
	{
		// processing plugins must have a dry/wet mix and presets
		if (plugin->getType()==PLUGIN_PROCESSOR)
		{
			plugin->registerParam(PARAM_MIX,"Mix","Dry/Wet Mix","","Dry","Wet",0,127,1,127);

			updatePluginPresets(plugin);
		}
	}

	return plugin;
}

void Host::chainLock()
{
	pthread_mutex_lock(&_chainSpinner);
}

void Host::deletePluginPreset(cJSON* json)
{
	cJSON* jsonInstance = cJSON_GetObjectItem(json,"instance");
	cJSON* jsonName = cJSON_GetObjectItem(json,"presetName");

	if (jsonInstance && jsonInstance->type==cJSON_Number &&
		jsonName && jsonName->type==cJSON_String)
	{
		char* presetName = jsonName->valuestring;
		Plugin* plugin = getPluginFromInstance(jsonInstance->valueint);
		if (!plugin)
		{
			JSONERROR(json,"no such plugin instance");
		}
		else
		{
			JsonFile* jsfile = new JsonFile( string("presets/")+string(plugin->getName()) );
			if (jsfile)
			{
				// get preset object, create if needed
				cJSON* presets = cJSON_GetObjectItem(jsfile->json(),"presets");
				if (presets)
				{
					// delete the old preset with this name (if existant)
					cJSON_DeleteItemFromObject(presets,presetName);
				}

				// flush and dispose
				delete jsfile;

				// update plugin to reflect new preset
				updatePluginPresets(plugin);
			}
		}
	}
	else
	{
		JSONERROR(json,"instance and/or presetName not given");
	}
}

void Host::storePluginPreset(cJSON* json)
{
	cJSON* jsonInstance = cJSON_GetObjectItem(json,"instance");
	cJSON* jsonName = cJSON_GetObjectItem(json,"presetName");

	if (jsonInstance && jsonInstance->type==cJSON_Number &&
		jsonName && jsonName->type==cJSON_String)
	{
		char* presetName = jsonName->valuestring;
		Plugin* plugin = getPluginFromInstance(jsonInstance->valueint);
		if (!plugin)
		{
			JSONERROR(json,"no such plugin instance");
		}
		else
		{
			JsonFile* jsfile = new JsonFile( string("presets/")+string(plugin->getName()) );
			if (jsfile)
			{
				// get preset object, create if needed
				cJSON* presets = cJSON_GetObjectItem(jsfile->json(),"presets");
				if (presets)
				{
					// delete the old preset with this name (if existant)
					cJSON_DeleteItemFromObject(presets,presetName);

					// make new one
					cJSON* preset = cJSON_CreateObject();
					plugin->getAllParams(preset);

					// add it
					cJSON_AddItemToObject(presets,presetName,preset);
				}

				// flush and dispose
				delete jsfile;

				// update plugin to reflect new preset
				updatePluginPresets(plugin);
			}
		}
	}
	else
	{
		JSONERROR(json,"instance and/or presetName not given");
	}
}

void Host::deleteRackPreset(cJSON* json)
{
	cJSON* jsonName = cJSON_GetObjectItem(json,"presetName");

	if (jsonName && jsonName->type==cJSON_String)
	{
		char* presetName = jsonName->valuestring;
		JsonFile* file = new JsonFile(string("racks/")+string(presetName));
		file->remove();
	}
	else
	{
		JSONERROR(json,"must give presetName");
	}
}

void Host::storeRackPreset(cJSON* json)
{
	cJSON* jsonName = cJSON_GetObjectItem(json,"presetName");

	if (jsonName && jsonName->type==cJSON_String)
	{
		char* presetName = jsonName->valuestring;

		// clear old preset (or make new one if none before
		JsonFile* file = new JsonFile(string("racks/")+string(presetName));
		file->clear();

		// add plugin chain (brief version)
		getPluginChain(file->json(),false);

		delete file;
	}
	else
	{
		JSONERROR(json,"must give presetName");
	}
}

void Host::loadRackPreset(cJSON* json)
{
	cJSON* presetName = cJSON_GetObjectItem(json,"presetName");

	if (presetName && presetName->type==cJSON_String)
	{
		JsonFile* file = new JsonFile(string("racks/")+string(presetName->valuestring));

		chainLock();

		// delete all plugins
		vector<Plugin*>::iterator it;
		for(it=_plugins.begin(); it!=_plugins.end();++it)
			delete *it;
		_plugins.clear();

		// create the new chain
		cJSON* pluginArray = cJSON_GetObjectItem(file->json(),"plugins");
		if(pluginArray)
		{
			for (int i=0;i<cJSON_GetArraySize(pluginArray);i++)
			{
				cJSON* pluginJson = cJSON_GetArrayItem(pluginArray,i);
				Plugin* plugin;

				// create the plugin
				plugin = createNewPlugin(cJSON_GetObjectItem(pluginJson,"name")->valuestring);
				_plugins.push_back(plugin);

				// initialize plugin
				cJSON* paramArray = cJSON_GetObjectItem(pluginJson,"params");
				for (int p=0;p<cJSON_GetArraySize(paramArray);p++)
				{
					cJSON* paramObject = cJSON_GetArrayItem(paramArray,p);
					char* paramName = cJSON_GetObjectItem(paramObject,"name")->valuestring;
					int paramValue = cJSON_GetObjectItem(paramObject,"value")->valueint;

					if (strcmp(paramName,"preset")!=0)
						plugin->setParam(paramName,paramValue);
				}

				plugin->setInstance(cJSON_GetObjectItem(pluginJson,"instance")->valueint);
				plugin->setFriend(cJSON_GetObjectItem(pluginJson,"friend")->valueint);
			}
		}


		chainUnlock();
	}
	else
	{
		JSONERROR(json,"must give presetName");
	}
}

void Host::listRackPresets(cJSON* json)
{
	// dir data/racks/ for .json files
	DIR *dp;
	struct dirent *ep;
	dp = opendir (STORAGE_DIR"racks");

	if (dp != NULL)
	{
		cJSON* racks = cJSON_CreateArray();

		while ( (ep = readdir (dp)))
		{
			if (strstr(ep->d_name,".json"))
			{
				cJSON_AddItemToArray(racks,cJSON_CreateString(string(ep->d_name).substr(0,strlen(ep->d_name)-5).c_str()));
			}
		}

		(void) closedir (dp);

		cJSON_AddItemToObject(json,"racks",racks);
	}
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

void Host::updatePluginPresets(Plugin* plugin)
{
	JsonFile* jsfile = new JsonFile( string("presets/")+string(plugin->getName()) );

	// delete old parameter
	plugin->unRegisterParam("preset");

	if (jsfile)
	{
		// get preset object, create if needed
		cJSON* presets = cJSON_GetObjectItem(jsfile->json(),"presets");
		if (!presets)
		{
			// create presets object
			presets = cJSON_CreateObject();
			cJSON_AddItemToObject(jsfile->json(),"presets",presets);
		}

		// create default preset. do this every time, in case we change the hardcoded values
		// in the registerParam() calls, or add or change the parameters themserves.
		cJSON* dfault = cJSON_GetObjectItem(presets,"default");
		if (dfault) {
			while (dfault->child)
				cJSON_DeleteItemFromObject(dfault,dfault->child->string);
		} else {
			dfault = cJSON_CreateObject();
			cJSON_AddItemToObject(presets,"default",dfault);
		}
		plugin->getAllParams(dfault);

		// get preset count
		int presetCount = 0;
		cJSON* preset = presets->child;
		vector<string> presetNames;
		while (preset)
		{
			presetNames.push_back(string(preset->string));
			presetCount++;
			preset = preset->next;
		}

		// register new parameter
		plugin->registerParam(PARAM_PRESET,"preset",presetNames,0);


		// flush and dispose
		delete jsfile;
	}
}
