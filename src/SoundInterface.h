/*
 * SoundInterface.h
 *
 *  Created on: Sep 14, 2013
 *      Author: slippy
 */

#ifndef SOUNDINTERFACE_H_
#define SOUNDINTERFACE_H_

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "portaudio.h"
#include "Host.h"
#include "HttpServer.h"
#include <unistd.h>
#include "settings.h"

#define TABLE_SIZE   (200)

class SoundInterface
{
public:
	SoundInterface();
	virtual ~SoundInterface();

	bool init(int period,int rate,string devicename);

	int getDeviceCount();
	string getDeviceName(int n);

	bool isGood();

	string getLastError();

private:
	bool _legit;
	string _error;

private:
    PaStreamParameters inputParameters, outputParameters;

	paTestData;
};

#endif /* SOUNDINTERFACE_H_ */
