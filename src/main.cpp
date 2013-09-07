

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "portaudio.h"
#include "Host.h"
#include "HttpServer.h"
#include <unistd.h>
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

/*******************************************************************/
int main(int argc,char* argv[])
{
    PaStreamParameters inputParameters, outputParameters;
    PaStream *stream;
    PaError err;
    paTestData data;
    int i;
    HttpServer server;

	#ifdef __SSE__
    printf("compiled with SSE\n");
	#endif

    server.setPluginHost(&data.host);

    /* initialise sinusoidal wavetable */
    for( i=0; i<TABLE_SIZE; i++ )
    {
        data.sine[i] = (float) sin( ((double)i/(double)TABLE_SIZE) * M_PI * 2. );
    }
    data.left_phase = data.right_phase = 0;
    data.toneBlocks=0;

    // get args to specify what sound device
    char* devName = 0;

    for (int i=1;i<argc;i++)
    {
    	char* arg = argv[i];

    	if (strstr(arg,"device=")==arg)
    		devName = &arg[strlen("device=")];
    	if (strstr(arg,"mono=")==arg)
    	    g_monoChannel = atoi(&arg[strlen("mono=")]);
    	if (strstr(arg,"testtone")==arg)
    		g_testTone = atoi(&arg[strlen("testtome=")]);
    }

    err = Pa_Initialize();
    if( err != paNoError ) goto error;

    setAudioIO(&inputParameters,devName,&outputParameters,devName);

    if (inputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default input device.\n");
      goto error;
    }
    inputParameters.channelCount = 2;       /* stereo input */
    inputParameters.sampleFormat = PA_SAMPLE_TYPE | INTERLEAVED;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    if (outputParameters.device == paNoDevice) {
      fprintf(stderr,"Error: No default output device.\n");
      goto error;
    }
    outputParameters.channelCount = 2;       /* stereo output */
    outputParameters.sampleFormat = PA_SAMPLE_TYPE | INTERLEAVED;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
              &stream,
              &inputParameters,
              &outputParameters,
              SAMPLE_RATE,
              FRAMES_PER_BUFFER,
              paClipOff,
              processCallback,
              &data );
    if( err != paNoError )
    {
    	printf("trying again with mono input...\n");

    	inputParameters.channelCount = 1;
        err = Pa_OpenStream(
                  &stream,
                  &inputParameters,
                  &outputParameters,
                  SAMPLE_RATE,
                  FRAMES_PER_BUFFER,
                  paClipOff,
                  processCallback,
                  &data );

        if( err != paNoError )
        {
        	goto error;
        }
        else
        {
        	spareRightBuffer = (float*)malloc(sizeof(float)*FRAMES_PER_BUFFER);
        	memset(spareRightBuffer,0,sizeof(float)*FRAMES_PER_BUFFER);
        }
    }

    printf("pause for effect...\n");
    sleep(2);

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    printf("Hit ENTER to stop program.\n");
    getchar();
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;

    printf("Finished. gNumNoInputs = %d\n", gNumNoInputs );
    Pa_Terminate();
    server.stop();

    if (spareRightBuffer)
    	free(spareRightBuffer);
    return 0;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occured while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
    server.stop();
    return -1;
}
