/*
 * Processor.h
 *
 *  Created on: Sep 14, 2013
 *      Author: slippy
 */

#ifndef PROCESSOR_H_
#define PROCESSOR_H_

#include "portaudio.h"

class Processor
{
public:
	Processor(){};
	virtual ~Processor(){};

	virtual int process(float* inLeft,float* inRight,float* outLeft,float* outRight,
			 unsigned long framesPerBuffer,
			 const PaStreamCallbackTimeInfo *timeInfo,
			 PaStreamCallbackFlags statusFlags) =0;

	virtual void panic()=0;
};

#endif /* PROCESSOR_H_ */
