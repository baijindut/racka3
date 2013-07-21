/*
 * FuzzTest.h
 *
 *  Created on: 21 Jul 2013
 *      Author: slippy
 */

#ifndef FUZZTEST_H_
#define FUZZTEST_H_

#include "global.h"
#include "delayline.h"
#include"../Plugin.h"

class FuzzTest : public Plugin
{
public:
	FuzzTest();
	virtual ~FuzzTest();

	int process(float* inLeft,float* inRight,float* outLeft,float* outRight,
			  unsigned long framesPerBuffer);
};

#endif /* FUZZTEST_H_ */
