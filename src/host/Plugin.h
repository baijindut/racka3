/*
 * Plugin.h
 *
 *  Created on: 21 Jul 2013
 *      Author: slippy
 */

#ifndef PLUGIN_H_
#define PLUGIN_H_

#include "portaudio.h"
#include "cJSON.h"
#include "uthash.h"
#include <vector>
#include <string>
using namespace std;

class Plugin;
struct PluginParam
{
	int index;					// 1
	char name[64];				// "delay length"
	char description[256];		// "duration of simulated tape loop" (good for a tooltip perhaps)
	char units[64];				// "msec"
	char minName[64];			// "none"  (could be "off" or "no" if this represented a button)
	char maxName[64];			// "2000"  (could be "on" or "yes" if this represented a button)
	vector<string> labels;		// labels, eg "sine","squar","tri"
	int max;					// 2000.0
	int min;					// 0.0
	int step;					// 1.0
	int defaultValue;
	Plugin* plugin;				// pointer to the plugin that owns this parameter
	UT_hash_handle hh;
};

class Plugin {
public:
	Plugin();
	virtual ~Plugin();

	// process an input buffer into supplied output buffer
	virtual int process(float* inLeft,float* inRight,float* outLeft,float* outRight,
						  unsigned long framesPerBuffer) =0;

	char* getName();
	int getVersion();

	// called by host to set plugin parameters as per supplied json of format, eg:
	// [{param="volume",value=100},{param="pan",value=128}]
	// sets the volume to 100 and pan to 128
	int setParams(cJSON* jsonParamArray);

	// set a single parameter
	int setParam(cJSON* json);

	// called by host to get plugin parameters. supplied values (if any) are completely ignored.
	// [{param="volume",value=100},{param="pan",value=128}] or [{param="volume"},{param="pan",value=128}]
	// might be updated to:
	// [{param="volume",value=111},{param="pan",value=150}]
	int getParams(cJSON* jsonParamArray);

	// creates the JSON describing the plugin and all its details
	int getPluginJson(cJSON* jsonObject);

	void setInstance(int instance);
	int getInstance();

	void setPosition(int position);
	int getPosition();

protected:
	void registerPlugin(char* name,
								  char* description,
								  int version);

	// called by actual plugin to setup json mapping
	PluginParam* registerParam(int index,
								 char* name,
								 char* description,
						 char* units,
						 char* minName,
						 char* maxName,
						 int min,int max,int step,int value);

	// alternative version to register multiposition knobs like 'lfo type' (sine, saw, square)
	PluginParam* registerParam(int index,char* name,const char* labels[],int value);

	// set or get a param by its index, called by the setParams() or getParams() function
	virtual void setParam(int index,int value) =0;
	virtual int getParam(int index)=0;

private:
	char _name[64];
	char _description[256];
	int _version;

	int _instance;
	int _position;

	PluginParam* _paramList;

};

#endif /* PLUGIN_H_ */
