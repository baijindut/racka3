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
#include "../settings.h"
using namespace std;

class Host
{
public:
	Host();
	virtual ~Host();

	int process(const float *inputBuffer,
				 float *outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* timeInfo,
                 PaStreamCallbackFlags statusFlags);

	void getAvailablePlugins(cJSON* json);

	void getPluginChain(cJSON* json);

	void addPlugin(cJSON* json);			// name, position
	void removePlugin(cJSON* json);		// instance
	void movePlugin(cJSON* json);			// instance, position

	void setPluginParam(cJSON* json);

private:

	// create a plugin from its name (unless there is one available in the pool, in which case you get it, and it is removed
	Plugin* createPluginIfNeeded(char* name,bool addToPoolImmediately=false);

	// definately create a new plugin instance
	Plugin* createNewPlugin(char* name);

	// get plugin by instance number
	Plugin* getPluginFromInstance(int instance);

	Plugin* findPluginFromFriendAndType(int friendInstance,PluginType type);

	// give all plugins their correct position value
	void renumberPlugins();

	// pool of spare plugins
	vector <Plugin*> _pool;

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
