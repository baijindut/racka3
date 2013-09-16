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
#include "portaudio.h"
#include "settings.h"
#include "cJSON.h"
#include "Processor.h"
#include <iostream>
#include <string>
#include "JsonFile.h"
#include "portaudiocpp/PortAudioCpp.hxx"
#ifdef WIN32
#include "portaudiocpp/AsioDeviceAdapter.hxx"
#endif

#define TABLE_SIZE   (200)

class SoundInterface
{
public:
	SoundInterface();
	virtual ~SoundInterface();

	// needs 'period' int, 'device' string
	bool init(cJSON* json);
	bool close();

	// adds json array 'devices' to json
	bool listDevices(cJSON* json);

	// adds 'error' string to json
	bool getLastError(cJSON* json);

	bool getCurrent(cJSON* json);

	bool isGood();

	void setProcessor(Processor* p);

	// called by portaudio
	int process(const void *inputBuffer,
				 void *outputBuffer,
				 unsigned long framesPerBuffer,
				 const PaStreamCallbackTimeInfo *timeInfo,
				 PaStreamCallbackFlags statusFlags);

private:
	void printSupportedStandardSampleRates(cJSON* json,
			const portaudio::DirectionSpecificStreamParameters &inputParameters,
			const portaudio::DirectionSpecificStreamParameters &outputParameters);

	portaudio::Device* deviceFromJson(cJSON* json);
	int bufferSizeFromJson(cJSON* json);

	std::string _error;
	portaudio::MemFunCallbackStream<SoundInterface>* _stream;
	Processor* _processor;

	portaudio::AutoSystem* _autoSys;

	portaudio::System* _sys;

	JsonFile* _persist;
};

#endif /* SOUNDINTERFACE_H_ */
