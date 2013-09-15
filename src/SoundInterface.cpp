/*
 * SoundInterface.cpp
 *
 *  Created on: Sep 14, 2013
 *      Author: slippy
 */

#include "SoundInterface.h"
using namespace std;

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define CATCHER \
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
	_sys = &portaudio::System::instance();
}

SoundInterface::~SoundInterface()
{

}

bool SoundInterface::close()
{
	_error.clear();

	try
	{
		if (_stream)
		{
			_stream->stop();
			_stream->close();
			delete _stream;
			_stream=0;
		}
	}
	CATCHER

	return _error.size();
}

bool SoundInterface::init(cJSON* json)
{
	try
	{
		if (_stream)
			close();

		// todo: get input and output streams based on requested device name

		// Set up the parameters required to open a (Callback)Stream:
		portaudio::DirectionSpecificStreamParameters outParams(_sys->defaultOutputDevice(),
															   2,
															   portaudio::FLOAT32,
															   false,
															   _sys->defaultOutputDevice().defaultLowOutputLatency(),
															   NULL);
		portaudio::DirectionSpecificStreamParameters inParams (_sys->defaultInputDevice(),
															   2,
															   portaudio::FLOAT32,
															   false,
															   _sys->defaultInputDevice().defaultLowOutputLatency(),
															   NULL);
		portaudio::StreamParameters params(inParams, outParams, SAMPLE_RATE, PERIOD, paClipOff);

		// Create (and open) a new Stream, using the SineGenerator::generate function as a callback:
		_stream = new portaudio::MemFunCallbackStream<SoundInterface>(params, *this, &SoundInterface::process);

		_stream->start();
	}
	CATCHER

	return _error.size()==0;
}

int SoundInterface::process(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer,const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags)
{
	if (_processor)
	{
		float *inLeft = ((float **) inputBuffer)[0];
		float *inRight = ((float **) inputBuffer)[1];
		float *outLeft = ((float **) outputBuffer)[0];
		float *outRight = ((float **) outputBuffer)[1];

		return _processor->process(inLeft,inRight,outLeft,outRight,framesPerBuffer,timeInfo,statusFlags);
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
