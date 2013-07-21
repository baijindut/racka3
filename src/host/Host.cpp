/*
 * Host.cpp
 *
 *  Created on: 20 Jul 2013
 *      Author: slippy
 */

#include "Host.h"
#include <string.h>


#include "plugins/RBEcho.h"
#include "plugins/FuzzTest.h"


Host::Host()
{
	RBEcho* echo = new RBEcho();
	_plugins.push_back(echo);

//	FuzzTest* fuzz = new FuzzTest();
//	_plugins.push_back(fuzz);
}


Host::~Host()
{
	for(deque<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
	{
		delete *it;
	}
}

int Host::process(const float *inputBuffer,
			 float *outputBuffer,
             unsigned long framesPerBuffer,
             const PaStreamCallbackTimeInfo* timeInfo,
             PaStreamCallbackFlags statusFlags)
{
	int i;

	float *inLeft = ((float **) inputBuffer)[0];
	float *inRight = ((float **) inputBuffer)[1];
	float *outLeft = ((float **) outputBuffer)[0];
	float *outRight = ((float **) outputBuffer)[1];

	// copy to input buffer
	memcpy(_bufferLeft[0],inLeft,framesPerBuffer*sizeof(float));
	memcpy(_bufferRight[0],inRight,framesPerBuffer*sizeof(float));

	float* bufLeft1 = _bufferLeft[0];
	float* bufRight1 = _bufferRight[0];
	float* bufLeft2 = _bufferLeft[1];
	float* bufRight2 = _bufferLeft[1];
	float* latestLeftBuf = (float*)inLeft;
	float* latestRightBuf = (float*)inRight;

	if (_plugins.size())
	{
		for(deque<Plugin*>::iterator it=_plugins.begin(); it!=_plugins.end();++it)
		{
			// process
			(*it)->process(bufLeft1,bufRight1,bufLeft2,bufRight2,framesPerBuffer);

			// buffer swap
			latestLeftBuf = bufLeft2;
			latestRightBuf = bufRight2;
			bufLeft2 = bufLeft1;
			bufRight2 = bufLeft1;
			bufLeft1 = latestLeftBuf;
			bufRight1 = latestRightBuf;
		}
	}

	// copy to output buffer
	memcpy(outLeft,latestLeftBuf,framesPerBuffer*sizeof(float));
	memcpy(outRight,latestRightBuf,framesPerBuffer*sizeof(float));

	return paContinue;
}

