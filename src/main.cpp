

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "portaudio.h"
#include "Host.h"
#include "HttpServer.h"

#include "settings.h"

#ifndef M_PI
#define M_PI  (3.14159265)
#endif

#define TABLE_SIZE   (200)
typedef struct
{
    float sine[TABLE_SIZE];
    int left_phase;
    int right_phase;
    int toneBlocks;
    Host host;
}
paTestData;

static int processCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData );

static int gNumNoInputs = 0;
/* This routine will be called by the PortAudio engine when audio is needed.
** It may be called at interrupt level on some machines so don't do anything
** that could mess up the system like calling malloc() or free().
*/
static int processCallback( const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData )
{
    float *inLeft = ((float **) inputBuffer)[0];
    float *inRight = ((float **) inputBuffer)[1];
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
    	if (data->toneBlocks!=-1)
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
					*outLeft++ = data->sine[data->left_phase];  /* left */
					*outRight++ = data->sine[data->right_phase];  /* right */
					data->left_phase += 1;
					if( data->left_phase >= TABLE_SIZE ) data->left_phase -= TABLE_SIZE;
					data->right_phase += 3; /* higher pitch so we can distinguish left and right. */
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
    		return data->host.process((const float*)inputBuffer,
    									(float*)outputBuffer,
    									framesPerBuffer,
    									timeInfo,
    									statusFlags);
    	}
    }

    return paContinue;
}

PaError setAudioIO(PaStreamParameters* inputParameters,char* inputName, PaStreamParameters* outputParameters,char* outputName)
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
		    	inputParameters->device=i;
		}
	}

	outputParameters->device = Pa_GetDefaultOutputDevice();
	if (outputName)
	{
		for(i=0;i<numDevices;i++)
		{
		    deviceInfo = Pa_GetDeviceInfo(i);
		    if (strstr(deviceInfo->name,outputName))
		    	outputParameters->device=i;
		}
	}

	printf("\nUsing \"%s\" for input, and \"%s\" for output\n",
			Pa_GetDeviceInfo(inputParameters->device)->name,
			Pa_GetDeviceInfo(outputParameters->device)->name);

	return paNoError;
}

/*******************************************************************/
int main(int argc,char* argv[])
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    int i;
    HttpServer server;

    server.setPluginHost(&data.host);

    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.left_phase = data.right_phase = 0;
    data.toneBlocks=0;

    // get args to specify what sound device
    char* inName = 0;
    char* outName =0;

    if (argc>2)
    {
    	inName=argv[1];
    	outName=argv[2];
    }

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    setAudioIO(&inputParameters,inName,&outputParameters,outName);

    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    if (inputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default input device.\n");
      goto error;
    }
    inputParameters.channelCount = 2;       /* stereo input */
    inputParameters.sampleFormat = PA_SAMPLE_TYPE | paNonInterleaved;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = PA_SAMPLE_TYPE | paNonInterleaved;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              0, /* paClipOff, */  /* we won't output out of range samples so don't bother clipping them */
              processCallback,
              &data );
    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    printf("Hit ENTER to stop program.\n");
    getchar();
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    printf("Finished. gNumNoInputs = %d\n", gNumNoInputs );
    Pa_Terminate();
    server.stop();
    return 0;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    server.stop();
    return -1;
}
