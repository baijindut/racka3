/*
 * SoundInterface.cpp
 *
 *  Created on: Sep 14, 2013
 *      Author: slippy
 */

#include "SoundInterface.h"

#ifndef M_PI
#define M_PI  (3.14159265)
#endif


SoundInterface::SoundInterface()
{
	// TODO Auto-generated constructor stub

}

SoundInterface::~SoundInterface()
{
	// TODO Auto-generated destructor stub
}




static int processCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData );

static int gNumNoInputs = 0;

// copy mono channel to stereo. -1 = off, 0= channel0 to both, 1=channel1 to both
static int g_monoChannel = -1;

// play test tone on startup or not
static bool g_testTone = 0;

/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
float* spareRightBuffer=0;

static int processCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
	float *inLeft,*inRight;

	if (!spareRightBuffer) // <-- implies we are in stereo mode
	{
		inLeft = ((float **) inputBuffer)[0];
    	inRight = ((float **) inputBuffer)[1];

    	// copy 0->1 or 1->0
    	if (g_monoChannel>=0)
    	{
    		if (g_monoChannel==0)
    		{
    			for (int t=0;t<framesPerBuffer;t++)
    				inRight[t]=inLeft[t];
    		}
    		else
    		{
    			for (int t=0;t<framesPerBuffer;t++)
    				inLeft[t]=inRight[t];
    		}
    	}
	}
	else // we are in mono mode.
	{
		inLeft = ((float**)inputBuffer)[0];
		inRight = spareRightBuffer;

    	for (int t=0;t<framesPerBuffer;t++)
    		inRight[t]=inLeft[t];
	}

    float *outLeft = ((float **) outputBuffer)[0];
    float *outRight = ((float **) outputBuffer)[1];

    unsigned int i;
    (void) timeInfo; /* Prevent unused variable warnings. */
    (void) statusFlags;
    paTestData *data = (paTestData*)userData;

    if( inputBuffer == NULL )
    {

        for( i=0; i<framesPerBuffer; i++ )
        {
            *outLeft++ = 0;  /* left - silent */
            *outRight++ = 0;  /* right - silent */
        }
        gNumNoInputs += 1;
    }
    else
    {
    	// initial test tone
    	if (g_testTone && (data->toneBlocks!=-1))
    	{
    		if (data->toneBlocks++ > 100)
    		{
    	        for( i=0; i<framesPerBuffer; i++ )
    	        {
    	            *outLeft++ = 0;  /* left - silent */
    	            *outRight++ = 0;  /* right - silent */
    	        }
    			printf("stopped tone\n");
    			data->toneBlocks = -1;
    		}
    		else
    		{
				for( i=0; i<framesPerBuffer; i++ )
				{
					*outLeft++ = data->sine[data->left_phase];
					*outRight++ = data->sine[data->right_phase];
					data->left_phase += 1;
					if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
					data->right_phase += 3;
					if( data->right_phase >= TABLE_SIZE ) data->right_phase -= TABLE_SIZE;
				}
    		}
    	}
    	else
    	{
    		if (framesPerBuffer!=PERIOD)
    		{
    			printf("desired period %d not delivered (actual %d)\n",(int)PERIOD,(int)framesPerBuffer);
    		}

    		// processing here
    		return data->host.process(inLeft,inRight,outLeft,outRight,
    									framesPerBuffer,
    									timeInfo,
    									statusFlags);
    	}
    }

    return paContinue;
}

PaError setAudioIO(PaStreamParameters* inputParameters,
					char* inputName,
					PaStreamParameters* outputParameters,
					char* outputName)
{
	int numDevices;
	PaError err;
	const   PaDeviceInfo *deviceInfo;
	int i;

	numDevices = Pa_GetDeviceCount();
	if( numDevices < 0 )
	{
	    printf( "ERROR: Pa_CountDevices returned 0x%x\n", numDevices );
	    err = numDevices;
	    return err;
	}

	// list devices
	printf("\nList of audio devices:\n");
	for( i=0; i<numDevices; i++ )
	{
	    deviceInfo = Pa_GetDeviceInfo( i );
	    printf("%d (%d in, %d out): %s\n",i,deviceInfo->maxInputChannels,deviceInfo->maxOutputChannels,deviceInfo->name);
	}

	// use default device if no name specified


	inputParameters->device = Pa_GetDefaultInputDevice();
	if (inputName)
	{
		for(i=0;i<numDevices;i++)
		{
		    deviceInfo = Pa_GetDeviceInfo(i);
		    if (strstr(deviceInfo->name,inputName))
		    {
		    	inputParameters->device=i;
		    	break;
		    }
		}
	}

	outputParameters->device = Pa_GetDefaultOutputDevice();
	if (outputName)
	{
		for(i=0;i<numDevices;i++)
		{
		    deviceInfo = Pa_GetDeviceInfo(i);
		    if (strstr(deviceInfo->name,outputName))
		    {
		    	outputParameters->device=i;
		    	break;
		    }
		}
	}

	printf("\nUsing \"%s\" for input, and \"%s\" for output\n",
			Pa_GetDeviceInfo(inputParameters->device)->name,
			Pa_GetDeviceInfo(outputParameters->device)->name);

	return paNoError;
}

bool SoundInterface::init(int period, int rate, string devicename)
{

}

bool SoundInterface::isGood()
{
	return _error.size()==0;
}

string SoundInterface::getLastError()
{
	return _error;
}
