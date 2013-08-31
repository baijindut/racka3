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
#include "StereoBuffer.h"
#include "JsonFile.h"
using namespace std;

#define PARAM_MIX 900
#define PARAM_PRESET 901

class Host;
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

enum PluginType
{
	PLUGIN_PROCESSOR=0,
	PLUGIN_SPLITTER,
	PLUGIN_SOURCE,
	PLUGIN_COLLECTOR
};

class Plugin {
public:

	Plugin();
	virtual ~Plugin();

	// process an input buffer into internal output buffers(s)
	int master(StereoBuffer* input);
	int master(StereoBuffer* inputA,StereoBuffer* inputB);

	// waah! clean buffers in an attempt to prevent pops
	void panic();

	char* getName();
	PluginType getType();
	int getVersion();

	// called by host to set plugin parameters as per supplied json of format, eg:
	// [{param="volume",value=100},{param="pan",value=128}]
	// sets the volume to 100 and pan to 128
	int setParams(cJSON* jsonParamArray);

	// set a single parameter
	int setParam(cJSON* json);
	int setParam(char* name,int value);

	// called by host to get plugin parameters. supplied values (if any) are completely ignored.
	// [{param="volume",value=100},{param="pan",value=128}] or [{param="volume"},{param="pan",value=128}]
	// might be updated to:
	// [{param="volume",value=111},{param="pan",value=150}]
	int getParams(cJSON* jsonParamArray);

	void getAllParams(cJSON* jsonParams);

	// creates the JSON describing the plugin and all its details
	int getPluginJson(cJSON* jsonObject);

	void setInstance(int instance);
	int getInstance();

	void setPosition(int position);
	int getPosition();

	// mixing and routing
	int getMix();
	void setMix(int mix);

	// used for host to store the preset
	int getPresetNumber();
	void setPresetNumber(int presetNumber);

	int getOutputBufferCount();
	StereoBuffer* getOutputBuffer(int i);

	void setFriend(int mate);
	int getFriend();

	void registerPlugin(int outputCount,
						char* name,
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
	PluginParam* registerParam(int index,char* name,vector<string> labels,int value);

	PluginParam* getRegisteredParam(char* name);

	bool unRegisterParam(char* name);

protected:

	virtual int process(StereoBuffer* input) =0;

	// set or get a param by its index, called by the setParams() or getParams() function
	virtual void setParam(int index,int value) =0;
	virtual int getParam(int index)=0;

protected:
	PluginType _type;

	vector<StereoBuffer*> _outputBuffers;
	int _desiredSourceInstance;
	int _desiredSourceChannel;
	int _mix;

private:
	char _name[64];
	char _description[256];
	int _version;

	int _instance;
	int _position;

	int _friend;

	PluginParam* _paramList;

	int _presetNumber;
};

#endif /* PLUGIN_H_ */
