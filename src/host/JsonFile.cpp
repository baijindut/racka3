/*
 * JsonFile.cpp
 *
 *  Created on: Aug 27, 2013
 *      Author: slippy
 */

#include "JsonFile.h"
#include <stdio.h>
#include <stdlib.h>
#include "settings.h"
#include <string.h>

using namespace std;

JsonFile::JsonFile(string fname)
{
	_name = string(STORAGE_DIR);
	for (int i=0;i<fname.length();i++)
	{
		char c = tolower(fname[i]);
		if ( (c>='a' && c<='z') || c=='/')
			_name+=c;
	}
	_name+=".json";

	_json = cJSON_CreateObject();

	if (!reload())
		persist();
}

JsonFile::~JsonFile()
{
	persist();
	if (_json)
		cJSON_Delete(_json);
}

static void _mkdir(const char *dir)
{
	char tmp[256];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if (tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for (p = tmp + 1; *p; p++)
	{
		if (*p == '/')
		{
			*p = 0;
			mkdir(tmp, S_IRWXU);
			*p = '/';
		}
	}
	mkdir(tmp, S_IRWXU);
}

cJSON* JsonFile::json()
{
	return _json;
}

void JsonFile::ensureDirectories()
{
	// get dir for file
	string dir = string(_name);

	unsigned int pos = dir.find_last_of('/');
	dir = dir.substr(0, pos);

	_mkdir(dir.c_str());
}

bool JsonFile::persist()
{
	bool ret = false;

	if (_json)
	{
		ensureDirectories();

		FILE* h = fopen(_name.c_str(), "w");
		if (h)
		{
			char * buffer = cJSON_Print(_json);

			ret = 1 == fwrite(buffer, strlen(buffer), 1, h);

			free(buffer);

			fclose(h);
		}
	}

	return ret;
}

bool JsonFile::reload()
{
	bool ret = false;

	FILE* h = fopen(_name.c_str(),"r");
	if (h)
	{
		fseek(h,0,SEEK_END);
		unsigned int size = ftell(h);
		rewind(h);

		char* buffer = (char*)malloc(size+1);
		buffer[size]=0;

		ret = 1==fread(buffer,size,1,h);

		if (ret)
		{
			if (_json)
				cJSON_Delete(_json);

			_json = cJSON_Parse(buffer);
			if (_json)
				ret = true;
		}

		free(buffer);

		fclose(h);
	}

	return ret;
}
