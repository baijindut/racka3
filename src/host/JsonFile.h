/*
 * JsonFile.h
 *
 *  Created on: Aug 27, 2013
 *      Author: slippy
 */

#ifndef JSONFILE_H_
#define JSONFILE_H_

#include <string>

#include "cJSON.h"
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

class JsonFile
{
public:
	JsonFile(const char* fname);
	virtual ~JsonFile();

	// get pointer to json object
	cJSON* json();

	// persist json
	bool persist();

	// reload json
	bool reload();

private:
	cJSON* _json;
	FILE* _hFile;
	std::string _name;

	void ensureDirectories();
};

#endif /* JSONFILE_H_ */
