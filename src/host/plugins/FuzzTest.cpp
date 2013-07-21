/*
 * FuzzTest.cpp
 *
 *  Created on: 21 Jul 2013
 *      Author: slippy
 */

#include "FuzzTest.h"

FuzzTest::FuzzTest() {
	// TODO Auto-generated constructor stub

}

FuzzTest::~FuzzTest() {
	// TODO Auto-generated destructor stub
}

/* Non-linear amplifier with soft distortion curve. */
float CubicAmplifier( float input )
{
    float output, temp;
    if( input < 0.0 )
    {
        temp = input + 1.0f;
        output = (temp * temp * temp) - 1.0f;
    }
    else
    {
        temp = input - 1.0f;
        output = (temp * temp * temp) + 1.0f;
    }

    return output;
}
#define FUZZ(x) CubicAmplifier(CubicAmplifier(CubicAmplifier(CubicAmplifier(x))))

int FuzzTest::process(float* inLeft,float* inRight,float* outLeft,float* outRight,
		  unsigned long framesPerBuffer)
{

	for(int i=0; i<framesPerBuffer; i++ )
	{
		float left = *inLeft++;
		float right = *inRight++;

		*outLeft++ = FUZZ(left);  /* left - distorted */
		*outRight++ = FUZZ(right);  /* right - clean */
	}

	return paContinue;
}

