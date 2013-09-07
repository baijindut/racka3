/*
 * Plugin.cpp
 *
 *  Created on: 21 Jul 2013
 *      Author: slippy
 */

#include "Plugin.h"
#include "../settings.h"

Plugin::Plugin() {

	_name[0] = '\0';
	_description[0] = '\0';
	_version = 0;
	_paramList = 0;
	_instance = -1;
	_position = -1;

	_desiredSourceInstance = -1;
	_desiredSourceChannel = 0;

	_mix=127;
	_type=PLUGIN_PROCESSOR;
	_presetNumber=0;
}

Plugin::~Plugin() {


	// free all parameter structures
	PluginParam* param, *tmp;
	HASH_ITER(hh, _paramList, param, tmp)
	{
		HASH_DEL(_paramList,param);
		delete(param);
	}

	// free all output buffers
	for (vector<StereoBuffer*>::iterator it = _outputBuffers.begin();it != _outputBuffers.end(); ++it)
		delete (*it);
}

char* Plugin::getName() {
	return _name;
}

int Plugin::getVersion() {
	return _version;
}

int Plugin::setParam(char* name,int value)
{
	int ok=1;

	// get the PluginParam struct pointer for the named parameter
	PluginParam* pPluginParam = 0;
	HASH_FIND_STR(_paramList,name,pPluginParam);
	if (pPluginParam)
	{
		// actually set the value
		if (pPluginParam->index==PARAM_MIX)
			pPluginParam->plugin->setMix(value);
		if (pPluginParam->index==PARAM_PRESET)
			printf("setParam should never get called wih PARAM_PRESET!!\n");
		else
		{
			//printf("set param(%d,%d)\n",pPluginParam->index,value);
			pPluginParam->plugin->setParam(pPluginParam->index,value);
		}
	}
	else
	{
		ok = 0;
	}

	return ok;
}

int Plugin::setParam(cJSON* jsonItem)
{
	int ok =1;

	// get the parameter name
	cJSON* jsonParam  = cJSON_GetObjectItem(jsonItem,"param");

	// get the value
	cJSON* jsonValue = cJSON_GetObjectItem(jsonItem,"value");

	// make sure we got a string for the parameter name, and an int for the value
	if (jsonParam && jsonParam->type==cJSON_String && jsonParam->valuestring &&
		jsonValue && jsonValue->type==cJSON_Number)
	{
		ok = setParam(jsonParam->valuestring,jsonValue->valueint);
	}
	else
	{
		ok = 0;
	}

	return ok;
}

int Plugin::setParams(cJSON* jsonParamObject) {
	int ok = 1;

	// loop over array
	cJSON* jsonItem=jsonParamObject->child;
	while(jsonItem)
	{
		setParam(jsonItem->string,jsonItem->valueint);

		jsonItem=jsonItem->next;
	}

	return ok;
}

void Plugin::getAllParams(cJSON* jsonParams)
{
	PluginParam* param = _paramList;

	while (param)
	{
		cJSON* value = cJSON_CreateNumber(getParam(param->index));
		cJSON_AddItemToObject(jsonParams,param->name,value);
		param = (PluginParam*)param->hh.next;
	}
}

int Plugin::getParams(cJSON* jsonParamArray) {

	int ok = 1;

	// loop over array
	cJSON* jsonItem=jsonParamArray->child;
	while(jsonItem)
	{
		// get the parameter name
		cJSON* jsonParam  = cJSON_GetObjectItem(jsonItem,"param");

		// make sure we got a string for the parameter name
		if (!jsonParam || jsonParam->type!=cJSON_String || !jsonParam->valuestring)
		{
			ok = 0;
		}
		else
		{
			// get the PluginParam struct pointer for the named parameter
			PluginParam* pPluginParam = 0;
			HASH_FIND_STR(_paramList,jsonParam->valuestring,pPluginParam);
			if (pPluginParam)
			{
				// delete the parameter value (if one is supplied)
				cJSON_DeleteItemFromObject(jsonItem,"value");

				// actually get the value from the plugin
				int value;

				if (pPluginParam->index==PARAM_MIX)
					value = pPluginParam->plugin->getMix();
				else if (pPluginParam->index==PARAM_PRESET)
					value = pPluginParam->plugin->getPresetNumber();
				else
					value = pPluginParam->plugin->getParam(pPluginParam->index);

				// create a new value
				cJSON* jsonValue = cJSON_CreateNumber((double)value);

				// add it
				cJSON_AddItemToObject(jsonItem,"value",jsonItem);
			}
			else
			{
				ok = 0;
			}
		}

		jsonItem=jsonItem->next;
	}

	return ok;
}
int Plugin::getPluginJson(cJSON* jsonObject,bool bVerbose) {

	cJSON_AddItemToObject(jsonObject,"name",cJSON_CreateString(_name));
	cJSON_AddItemToObject(jsonObject,"version",cJSON_CreateNumber(_version));
	cJSON_AddItemToObject(jsonObject,"instance",cJSON_CreateNumber(_instance));
	cJSON_AddItemToObject(jsonObject,"position",cJSON_CreateNumber(_position));
	cJSON_AddItemToObject(jsonObject,"friend",cJSON_CreateNumber(_friend));

	if (bVerbose)
		cJSON_AddItemToObject(jsonObject,"description",cJSON_CreateString(_description));

	cJSON* paramArray = cJSON_CreateArray();
	PluginParam* pParam = _paramList;
	while (pParam)
	{
		cJSON* paramObject = cJSON_CreateObject();

		int value;
		if (pParam->index == PARAM_MIX)
			value = getMix();
		else if (pParam->index == PARAM_PRESET)
			value = getPresetNumber();
		else
			value = getParam(pParam->index);

		cJSON_AddItemToObject(paramObject,"name",cJSON_CreateString(pParam->name));
		cJSON_AddItemToObject(paramObject,"value",cJSON_CreateNumber(value));

		if (bVerbose)
		{
			cJSON_AddItemToObject(paramObject,"description",cJSON_CreateString(pParam->description));
			cJSON_AddItemToObject(paramObject,"units",cJSON_CreateString(pParam->units));
			cJSON_AddItemToObject(paramObject,"minName",cJSON_CreateString(pParam->minName));
			cJSON_AddItemToObject(paramObject,"maxName",cJSON_CreateString(pParam->maxName));
			cJSON_AddItemToObject(paramObject,"max",cJSON_CreateNumber((int)pParam->max));
			cJSON_AddItemToObject(paramObject,"min",cJSON_CreateNumber((int)pParam->min));
			cJSON_AddItemToObject(paramObject,"step",cJSON_CreateNumber((int)pParam->step));

			cJSON* labelArray = cJSON_CreateArray();
			for (vector<string>::iterator it = pParam->labels.begin();it!=pParam->labels.end();++it)
			{
				string s = *it;
				const char* c = s.c_str();
				cJSON_AddItemToArray(labelArray,cJSON_CreateString(c));
			}
			cJSON_AddItemToObject(paramObject,"labels",labelArray);
		}

		cJSON_AddItemToArray(paramArray,paramObject);
		pParam = (PluginParam*)pParam->hh.next;
	}

	cJSON_AddItemToObject(jsonObject,"params",paramArray);

	return 1;
}

void Plugin::registerPlugin(int outputCount,char* name, char* description, int version) {
	strcpy(_name,name);
	strcpy(_description,description);
	_version = version;

	// create output buffers, and add to vector
	for (int i=0;i<outputCount;i++) {
		StereoBuffer* buffer = new StereoBuffer(FRAMES_PER_BUFFER); // <-- this isnt the best
		_outputBuffers.push_back(buffer);
	}

}

int Plugin::getOutputBufferCount()
{
	return _outputBuffers.size();
}
StereoBuffer* Plugin::getOutputBuffer(int i)
{
	return _outputBuffers[i];
}

void Plugin::setInstance(int instance)
{
	_instance = instance;
}

int Plugin::getInstance()
{
	return _instance;
}

void Plugin::setPosition(int position)
{
	_position = position;
}

int Plugin::getPosition()
{
	return _position;
}

int Plugin::getMix()
{
	return _mix;
}

void Plugin::setMix(int mix)
{
	_mix = mix < 0 ? 0 : mix > 127 ? 127 : mix;
}

void Plugin::setFriend(int mate)
{
	_friend = mate;
}

int Plugin::getFriend()
{
	return _friend;
}

PluginType Plugin::getType()
{
	return _type;
}

PluginParam* Plugin::registerParam(int index,char* name,vector<string> labels,int value)
{
	// count the labels
	int labelCount=labels.size();

	// create a parameter struct as normal
	PluginParam* param = registerParam(index,name,"","","","",0,labelCount-1,1,value);

	// now add the labels to the list
	for (int i=0;i<labelCount;i++)
	{
		param->labels.push_back(labels[i]);
	}

	return param;
}

PluginParam* Plugin::registerParam(int index,char* name,const char* labels[],int value)
{
	// count the labels
	int labelCount=0;
	const char* label=labels[0];
	while (label)
	{
		label = labels[++labelCount];
	}

	// create a parameter struct as normal
	PluginParam* param = registerParam(index,name,"","","","",0,labelCount-1,1,value);

	// now add the labels to the list
	for (int i=0;i<labelCount;i++)
	{
		param->labels.push_back(labels[i]);
	}

	return param;
}

PluginParam* Plugin::registerParam(int index,char* name, char* description, char* units,
		char* minName, char* maxName, int min, int max, int step,
		int value) {

	PluginParam* param = new PluginParam();

	param->index=index;
	strcpy(param->name,name);
	strcpy(param->description,description);
	strcpy(param->units,units);
	strcpy(param->minName,minName);
	strcpy(param->maxName,maxName);
	param->max=max;
	param->min=min;
	param->step=step;
	param->plugin=this;
	param->defaultValue=value;

	// add to the hashmap
	HASH_ADD_STR(_paramList,name,param);

	// actually set the parameter
	param->plugin->setParam(index,value);

	return param;
}

int Plugin::master(StereoBuffer* input)
{
	if (_mix == 127)
	{
		// wet. use only the output of the effect
		return process(input);
	}
	else if (_mix==0)
	{
		// dry. use only the input - dont process effect at all
		for (int c=0;c<_outputBuffers.size();c++)
		{
			float* outLeft = _outputBuffers[c]->left;
			float* outRight = _outputBuffers[c]->right;

			for (int i =0;i<input->length;i++)
			{
				outLeft[i]=input->left[i];
				outRight[i]=input->right[i];
			}
		}
	}
	else
	{
		// some blend between wet and dry.
		float wet = _mix / 127.0;
		float dry = 1.0 - wet;

		// process the input into the output
		process(input);

		for (int c=0;c<_outputBuffers.size();c++)
		{
			float* outLeft = _outputBuffers[c]->left;
			float* outRight = _outputBuffers[c]->right;
			for (int i =0;i<input->length;i++)
			{
				// reduce volume of output
				outLeft[i] *= wet;
				outRight[i] *= wet;

				// add dry signal
				outLeft[i] += dry * input->left[i];
				outRight[i] += dry * input->right[i];
			}
		}
	}

	return paContinue;
}

int Plugin::master(StereoBuffer* inputA,StereoBuffer* inputB)
{
	// perform simple mix
	float* inLeftA = inputA->left;
	float* inRightA = inputA->right;
	float* inLeftB = inputB->left;
	float* inRightB = inputB->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

	int len = _outputBuffers[0]->length;

	for (int i=0;i<len;i++)
	{
		outLeft[i] = inLeftA[i] + inLeftB[i];
		outRight[i] = inRightA[i] + inRightB[i];
	}

	return paContinue;
}

void Plugin::panic()
{
	for (int b=0;b<_outputBuffers.size();b++)
	{
		_outputBuffers[b]->silence();
	}
}

PluginParam* Plugin::getRegisteredParam(char* name)
{
	PluginParam* pPluginParam = 0;
	HASH_FIND_STR(_paramList,name,pPluginParam);
	return pPluginParam;
}

int Plugin::getPresetNumber()
{
	return _presetNumber;
}

void Plugin::setPresetNumber(int presetNumber)
{
	_presetNumber = presetNumber;
}

bool Plugin::unRegisterParam(char* name)
{
	PluginParam* pPluginParam = 0;
	HASH_FIND_STR(_paramList,name,pPluginParam);
	if (pPluginParam)
	{
		HASH_DEL(_paramList,pPluginParam);
		delete(pPluginParam);

		return true;
	}
	return false;
}
