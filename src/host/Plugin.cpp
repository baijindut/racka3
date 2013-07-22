/*
 * Plugin.cpp
 *
 *  Created on: 21 Jul 2013
 *      Author: slippy
 */

#include "Plugin.h"

Plugin::Plugin() {

	_name[0] = '\0';
	_description[0] = '\0';
	_version = 0;

	_paramList = 0;
}

Plugin::~Plugin() {



}

char* Plugin::getName() {
	return _name;
}

int Plugin::getVersion() {
	return _version;
}

int Plugin::setParams(cJSON* jsonParamArray) {
	int ok = 1;

	// loop over array
	cJSON* jsonItem=jsonParamArray->child;
	while(jsonItem)
	{
		// get the parameter name
		cJSON* jsonParam  = cJSON_GetObjectItem(jsonItem,"param");

		// get the value
		cJSON* jsonValue = cJSON_GetObjectItem(jsonItem,"value");

		// make sure we got a string for the parameter name, and an int for the value
		if (!jsonParam || jsonParam->type!=cJSON_String || !jsonParam->valuestring ||
			!jsonValue || jsonValue->type!=cJSON_Number)
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
				// actually set the value
				pPluginParam->plugin->setParam(pPluginParam->index,jsonValue->valueint);
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
				int value = pPluginParam->plugin->getParam(pPluginParam->index);

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
int Plugin::getPluginJson(cJSON* jsonObject) {

	cJSON_AddItemToObject(jsonObject,"name",cJSON_CreateString(_name));
	cJSON_AddItemToObject(jsonObject,"description",cJSON_CreateString(_description));
	cJSON_AddItemToObject(jsonObject,"version",cJSON_CreateNumber(_version));

	cJSON* paramArray = cJSON_CreateArray();
	PluginParam* pParam = _paramList;
	while (pParam)
	{
		cJSON* paramObject = cJSON_CreateObject();

		cJSON_AddItemToObject(paramObject,"name",cJSON_CreateString(pParam->name));
		cJSON_AddItemToObject(paramObject,"description",cJSON_CreateString(pParam->description));
		cJSON_AddItemToObject(paramObject,"units",cJSON_CreateString(pParam->units));
		cJSON_AddItemToObject(paramObject,"minName",cJSON_CreateString(pParam->minName));
		cJSON_AddItemToObject(paramObject,"maxName",cJSON_CreateString(pParam->maxName));
		cJSON_AddItemToObject(paramObject,"max",cJSON_CreateNumber((int)pParam->max));
		cJSON_AddItemToObject(paramObject,"min",cJSON_CreateNumber((int)pParam->min));
		cJSON_AddItemToObject(paramObject,"step",cJSON_CreateNumber((int)pParam->step));

		cJSON_AddItemToArray(paramArray,paramObject);
		pParam = (PluginParam*)pParam->hh.next;
	}

	cJSON_AddItemToObject(jsonObject,"params",paramArray);

	return 1;
}

void Plugin::registerPlugin(char* name, char* description, int version) {
	strcpy(_name,name);
	strcpy(_description,description);
	_version = version;
}

void Plugin::registerParam(int index,char* name, char* description, char* units,
		char* minName, char* maxName, int min, int max, int step,
		int value) {

	PluginParam* param = (PluginParam*)malloc(sizeof(PluginParam));

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

	// add to the hashmap
	HASH_ADD_STR(_paramList,name,param);

	// actually set the parameter
	param->plugin->setParam(index,value);
}
