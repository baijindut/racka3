

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


/*******************************************************************/
int main(int argc,char* argv[])
{

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
    		g_testTone = atoi(&arg[strlen("testtone=")]);
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
