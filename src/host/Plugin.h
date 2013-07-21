/*
 * Plugin.h
 *
 *  Created on: 21 Jul 2013
 *      Author: slippy
 */

#ifndef PLUGIN_H_
#define PLUGIN_H_

#include "portaudio.h"

class Plugin {
public:
	Plugin();
	virtual ~Plugin();

	virtual int process(float* inLeft,float* inRight,float* outLeft,float* outRight,
						  unsigned long framesPerBuffer) =0;

};

#endif /* PLUGIN_H_ */
