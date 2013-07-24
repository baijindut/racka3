/*
 * Host.h
 *
 *  Created on: 20 Jul 2013
 *      Author: slippy
 */

#ifndef HOST_H_
#define HOST_H_

#include "portaudio.h"
#include <list>
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

	cJSON* getAvailablePlugins();

	bool addPlugin(char* name,int before);
	bool swapPlugin(int from,int to);

	bool setPluginParams(cJSON* json);
	bool setPluginParam(int index,char* param,int value);

private:

	// create a plugin from its name (unless there is one available in the pool, in which case you get it, and it is removed
	Plugin* createPluginIfNeeded(char* name,bool addToPoolImmediately=false);

	// definately create a new plugin instance
	Plugin* createNewPlugin(char* name);

	// pool of spare plugins
	list <Plugin*> _pool;

	// the plugins in the chain
	list <Plugin*> _plugins;

	// 2 buffers to swap, for input and output
	float _bufferLeft[2][FRAMES_PER_BUFFER*2];
	float _bufferRight[2][FRAMES_PER_BUFFER*2];

	cJSON* _jsonAllPlugins;
};

#endif /* HOST_H_ */
