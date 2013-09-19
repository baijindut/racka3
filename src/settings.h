/*
 * settings.h
 *
 *  Created on: 21 Jul 2013
 *      Author: slippy
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "portaudio.h"

static int SAMPLE_RATE	= (44100);
static float fSAMPLE_RATE = (44100.0);
static float cSAMPLE_RATE = (float)(1 / (44100.0));

#define PA_SAMPLE_TYPE      paFloat32
#define PERIOD				 FRAMES_PER_BUFFER
#define fPERIOD				 ((float)FRAMES_PER_BUFFER)
#define SAMPLE				float
#define INTERLEAVED			paNonInterleaved

// this is extremely shit and needs refactoring:
extern int FRAMES_PER_BUFFER;

#define STORAGE_DIR			"www/data/"


#endif /* SETTINGS_H_ */
