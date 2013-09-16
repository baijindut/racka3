/*
 * SoundInterface.cpp
 *
 *  Created on: Sep 14, 2013
 *      Author: slippy
 */

#include <string.h>
#include "SoundInterface.h"
#include "cJSON.h"
using namespace std;

#define MEGACATCH \
catch (const portaudio::PaException &e) \
{\
	_error=string("A PortAudio error occured: ")+e.paErrorText();\
}\
catch (const portaudio::PaCppException &e)\
{\
	_error=string("A PortAudioCpp error occured: ")+e.what();\
}\
catch (const std::exception &e)\
{\
	_error=string("A generic exception occured: ")+e.what();\
}\
catch (...)\
{\
	_error=string("An unknown exception occured.");\
}

SoundInterface::SoundInterface()
{
	_processor=0;
	_stream=0;
	_autoSys = new portaudio::AutoSystem();
	_sys = &portaudio::System::instance();

	_persist = new JsonFile("soundinterface");
}

SoundInterface::~SoundInterface()
{
	close();
	delete _autoSys;
	delete _persist;
}

bool SoundInterface::close()
{
	_error.clear();

	try
	{
		_stream->stop();
		_stream->close();
		_stream=0;
	}
	MEGACATCH

	return _error.size();
}

portaudio::Device* SoundInterface::deviceFromJson(cJSON* json)
{
	portaudio::Device* device = &_sys->defaultOutputDevice();

	if (json &&														// have json?
		cJSON_GetObjectItem(json,"device") &&							// contains 'name' key
		cJSON_GetObjectItem(json,"device")->type==cJSON_String &&		// which has string type value
		cJSON_GetObjectItem(json,"device")->string &&					// and has a not null string
		cJSON_GetObjectItem(json,"device")->string[0]!='\0')			// which is not 0-length
	{
		char* name = cJSON_GetObjectItem(json,"device")->valuestring;

		// loop over devices and select one that matches
		for (portaudio::System::DeviceIterator i = _sys->devicesBegin(); i != _sys->devicesEnd(); ++i)
		{
			if (0==strcmp((*i).name(),name))
			{
				device = &(*i);
				break;
			}
		}
	}

	return device;
}

bool SoundInterface::getCurrent(cJSON* json)
{
	cJSON* c=cJSON_GetObjectItem(_persist->json(),"current");
	if (c)
	{
		cJSON_AddStringToObject(json,"device",cJSON_GetObjectItem(c,"device")->valuestring);
		cJSON_AddNumberToObject(json,"period",cJSON_GetObjectItem(c,"period")->valueint);
		cJSON_AddBoolToObject(json,"knownGood",cJSON_GetObjectItem(c,"knownGood")->valueint);
	}
	return c!=0;
}

int SoundInterface::bufferSizeFromJson(cJSON* json)
{
	int bs = 1024;

	if (json &&														// have json?
		cJSON_GetObjectItem(json,"period") &&							// contains 'name' key
		cJSON_GetObjectItem(json,"period")->type==cJSON_Number)		// which has int type value
	{
		bs = cJSON_GetObjectItem(json,"period")->valueint;
	}

	return bs;
}

void cJSON_ReplaceItemInObjectOrAdd(cJSON* object,char* name,cJSON* item)
{
	if (cJSON_GetObjectItem(object,name))
		cJSON_ReplaceItemInObject(object,name,item);
	else
		cJSON_AddItemToObject(object,name,item);
}

bool SoundInterface::init(cJSON* json)
{
	_error.clear();
	try
	{
		if (_stream)
			close();

		// if no json supplied, get the current settings from file
		if (!json)
			json = cJSON_GetObjectItem(_persist->json(),"current");

		// extract the settings
		portaudio::Device* device = deviceFromJson(json);
		int period = bufferSizeFromJson(json);

		// create the config file
		cJSON* current = cJSON_CreateObject();
		cJSON_ReplaceItemInObjectOrAdd(_persist->json(),"current",current);
		cJSON_ReplaceItemInObjectOrAdd(current,"device",cJSON_CreateString(device->name()));
		cJSON_ReplaceItemInObjectOrAdd(current,"period",cJSON_CreateNumber(period));
		cJSON* list = cJSON_CreateObject();
		cJSON_ReplaceItemInObjectOrAdd(_persist->json(),"interface",list);
		listDevices(list);
		cJSON_ReplaceItemInObjectOrAdd(current,"knownGood",cJSON_CreateBool(false));
		_persist->persist();

		// Set up the parameters required to open a (Callback)Stream:
		portaudio::DirectionSpecificStreamParameters outParams(*device,
															   2,
															   portaudio::FLOAT32,
															   false,
															   device->defaultLowOutputLatency(),
															   0);
		portaudio::DirectionSpecificStreamParameters inParams (*device,
															   2,
															   portaudio::FLOAT32,
															   false,
															   device->defaultLowOutputLatency(),
															   0);

		portaudio::StreamParameters params(inParams, outParams, SAMPLE_RATE, period, paClipOff);

		_stream = new portaudio::MemFunCallbackStream<SoundInterface>(params, *this, &SoundInterface::process);

		_stream->start();

		// if we got here, this is a known good configuration
		cJSON_ReplaceItemInObjectOrAdd(current,"knownGood",cJSON_CreateBool(true));
		_persist->persist();
	}
	MEGACATCH

	if (_error.size())
		cJSON_ReplaceItemInObjectOrAdd(json,"error",cJSON_CreateString(_error.c_str()));

	return _error.size()==0;
}

int SoundInterface::process(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags)
{
	if (_processor)
	{
		if (inputBuffer && outputBuffer)
		{
			float *inLeft = ((float **) inputBuffer)[0];
			float *inRight = ((float **) inputBuffer)[1];
			float *outLeft = ((float **) outputBuffer)[0];
			float *outRight = ((float **) outputBuffer)[1];

			_processor->process(inLeft,inRight,outLeft,outRight,framesPerBuffer,timeInfo,statusFlags);
		}
	}

	return paContinue;
}

bool SoundInterface::isGood()
{
	return _error.size()==0;
}

bool SoundInterface::getLastError(cJSON* json)
{
	if (_error.size())
	{
		cJSON_AddStringToObject(json,"error",_error.c_str());
		return true;
	}
	return false;
}

bool SoundInterface::listDevices(cJSON* json)
{
	_error.clear();

	try
	{
		portaudio::AutoSystem autoSys;
		portaudio::System &sys = portaudio::System::instance();

		cJSON* jPa = cJSON_CreateObject();
		cJSON_AddItemToObject(json,"portaudio",jPa);
		cJSON_AddNumberToObject(jPa,"versionNumber",sys.version());
		cJSON_AddStringToObject(jPa,"versionText",sys.versionText());

		//int numDevices = sys.deviceCount();

		cJSON* jDevices = cJSON_CreateArray();
		cJSON_AddItemToObject(json,"devices",jDevices);

		for (portaudio::System::DeviceIterator i = sys.devicesBegin(); i != sys.devicesEnd(); ++i)
		{
			cJSON* jDev = cJSON_CreateObject();
			cJSON_AddItemToArray(jDevices,jDev);

			// Mark global and API specific default devices:
			cJSON* jDefault = cJSON_CreateArray();

			if ((*i).isSystemDefaultInputDevice())
				cJSON_AddStringToObject(jDefault,"input","system");
			else if ((*i).isHostApiDefaultInputDevice())
				cJSON_AddStringToObject(jDefault,"input",(*i).hostApi().name());

			if ((*i).isSystemDefaultOutputDevice())
				cJSON_AddStringToObject(jDefault,"output","system");
			else if ((*i).isHostApiDefaultOutputDevice())
				cJSON_AddStringToObject(jDefault,"output",(*i).hostApi().name());

			if (jDefault->child)
				cJSON_AddItemToObject(jDev,"default",jDefault);
			else
				cJSON_Delete(jDefault);

			// Print device info:
			cJSON_AddStringToObject(jDev,"name",(*i).name());
			cJSON_AddStringToObject(jDev,"api",(*i).hostApi().name());
			cJSON_AddNumberToObject(jDev,"in", (*i).maxInputChannels());
			cJSON_AddNumberToObject(jDev,"out", (*i).maxOutputChannels());

			// latency
			cJSON* jLat = cJSON_CreateObject();
			cJSON_AddItemToObject(jDev,"latency",jLat);

			cJSON_AddNumberToObject(jLat,"inLow",(*i).defaultLowInputLatency());
			cJSON_AddNumberToObject(jLat,"outLow",(*i).defaultLowOutputLatency());
			cJSON_AddNumberToObject(jLat,"inHigh",(*i).defaultHighInputLatency());
			cJSON_AddNumberToObject(jLat,"outHigh",(*i).defaultHighOutputLatency());

#ifdef WIN32
			// ASIO specific latency information:
			if ((*i).hostApi().typeId() == paASIO)
			{
				cJSON* jAsio = cJSON_CreateObject();
				cJSON_AddItemToObject(jDev,"asio",jAsio);

				portaudio::AsioDeviceAdapter asioDevice((*i));

				cJSON_AddNumberToObject(jAsio,"minBufSize",asioDevice.minBufferSize());
				cJSON_AddNumberToObject(jAsio,"maxBufSize",asioDevice.maxBufferSize());
				cJSON_AddNumberToObject(jAsio,"preferredBufSize",asioDevice.preferredBufferSize());

				// -1 indicates power of 2 buf size required
				cJSON_AddNumberToObject(jAsio,"granularity",asioDevice.granularity());
			}
#endif // WIN32

			/*
			cJSON* jSr = cJSON_CreateArray();
			cJSON_AddItemToObject(jDev,"sampleRates",jSr);

			// Poll for standard sample rates:
			portaudio::DirectionSpecificStreamParameters inputParameters((*i), (*i).maxInputChannels(), portaudio::INT16, true, 0.0, NULL);
			portaudio::DirectionSpecificStreamParameters outputParameters((*i), (*i).maxOutputChannels(), portaudio::INT16, true, 0.0, NULL);

			if (inputParameters.numChannels() > 0 && outputParameters.numChannels() > 0)
			{
//				cJSON* jRate = cJSON_CreateObject();
//				cJSON_AddItemToArray(jSr,jRate);
//				cJSON_AddNumberToObject(jRate,"in",inputParameters.numChannels());
//				cJSON_AddNumberToObject(jRate,"out",outputParameters.numChannels());

				printSupportedStandardSampleRates(jSr,inputParameters, outputParameters);
			}
			*/
		}
	}
	catch (const portaudio::PaException &e)
	{
		_error=string("A PortAudio error occured: ")+e.paErrorText();
	}
	catch (const portaudio::PaCppException &e)
	{
		_error=string("A PortAudioCpp error occured: ")+e.what();
	}
	catch (const std::exception &e)
	{
		_error=string("A generic exception occured: ")+e.what();
	}
	catch (...)
	{
		_error="An unknown exception occured.";
	}

	if (_error.size())
		cJSON_ReplaceItemInObjectOrAdd(json,"error",cJSON_CreateString(_error.c_str()));

	return _error.size()==0;
}

void SoundInterface::setProcessor(Processor* p)
{
	_processor=p;
}

void SoundInterface::printSupportedStandardSampleRates(cJSON* json,
		const portaudio::DirectionSpecificStreamParameters &inputParameters,
		const portaudio::DirectionSpecificStreamParameters &outputParameters)
{
	static double STANDARD_SAMPLE_RATES[] = {
		8000.0, 9600.0, 11025.0, 12000.0, 16000.0, 22050.0, 24000.0, 32000.0,
		44100.0, 48000.0, 88200.0, 96000.0, -1 }; // negative terminated list

	for (int i = 0; STANDARD_SAMPLE_RATES[i] > 0; ++i)
	{
		portaudio::StreamParameters tmp = portaudio::StreamParameters(inputParameters, outputParameters, STANDARD_SAMPLE_RATES[i], 0, paNoFlag);
		if (tmp.isSupported())
		{
			cJSON_AddItemToArray(json,cJSON_CreateNumber(STANDARD_SAMPLE_RATES[i]));
		}
	}
}
