/*
 * Host.h
 *
 *  Created on: 20 Jul 2013
 *      Author: slippy
 */

#ifndef HOST_H_
#define HOST_H_

#include "portaudio.h"
#include <vector>
#include "Plugin.h"
#include "Processor.h"
#include "../settings.h"
using namespace std;

class Host : public Processor
{
public:
	Host();
	virtual ~Host();

	int process(float* inLeft,float* inRight,float* outLeft,float* outRight,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* timeInfo,
                 PaStreamCallbackFlags statusFlags);
	void panic(){};

	void getAvailablePlugins(cJSON* json);

	void getPluginChain(cJSON* json,bool bVerbose = true);
	int getPluginChainSize();

	void addPlugin(cJSON* json);			// name, position
	void removePlugin(cJSON* json);		// instance
	void movePlugin(cJSON* json);			// instance, position

	void setPluginParam(cJSON* json);

	bool setPluginPreset(Plugin* plugin,int nPreset);

	void storePluginPreset(cJSON* json);
	void deletePluginPreset(cJSON* json);

	void storeRackPreset(cJSON* json);
	void deleteRackPreset(cJSON* json);
	void loadRackPreset(cJSON* json);
	void listRackPresets(cJSON* json);


private:

	// create a new plugin instance
	Plugin* createNewPlugin(string name);

	// get plugin by instance number
	Plugin* getPluginFromInstance(int instance);

	Plugin* findPluginFromFriendAndType(int friendInstance,PluginType type);

	// give all plugins their correct position value
	void renumberPlugins();

	// update (or create if necesary) the plugins preset parameter
	void updatePluginPresets(Plugin* plugin);

	// pool of spare plugins
	vector <string> _pluginNames;

	// the plugins in the chain
	vector <Plugin*> _plugins;

	cJSON* _jsonAllPlugins;

	// chain action lock
	pthread_mutex_t _chainSpinner;
	void chainLock();
	void chainUnlock();

	int _nextInstance;
};

#endif /* HOST_H_ */
