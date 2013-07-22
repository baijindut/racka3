/*
 * Host.h
 *
 *  Created on: 20 Jul 2013
 *      Author: slippy
 */

#ifndef HOST_H_
#define HOST_H_

#include "portaudio.h"
#include <deque>
#include "Plugin.h"
#include "../settings.h"
using namespace std;

class Host
{
public:
	Host();
	virtual ~Host();

	int process(const float *inputBuffer,
				 float *outputBuffer,
                 unsigned long framesPerBuffer,
                 const PaStreamCallbackTimeInfo* timeInfo,
                 PaStreamCallbackFlags statusFlags);


private:
	deque <Plugin*> _plugins;

	// 2 buffers to swap, for input and output
	float _bufferLeft[2][FRAMES_PER_BUFFER*2];
	float _bufferRight[2][FRAMES_PER_BUFFER*2];

};

#endif /* HOST_H_ */
