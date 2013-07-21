/*
 * settings.h
 *
 *  Created on: 21 Jul 2013
 *      Author: slippy
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "portaudio.h"

#define SAMPLE_RATE			 (44100)
#define fSAMPLE_RATE		 (44100.0)
#define PA_SAMPLE_TYPE      paFloat32
#define FRAMES_PER_BUFFER   (512)
#define PERIOD				 FRAMES_PER_BUFFER
typedef float SAMPLE;


#endif /* SETTINGS_H_ */
