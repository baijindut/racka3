/*
 * PluginBackingTrack.cpp
 *
 *  Created on: 1 Aug 2013
 *      Author: lenovo
 */

#include "PluginBackingTrack.h"
#include "portaudio.h"
#include <stdio.h>

PluginBackingTrack::PluginBackingTrack()
{
    registerPlugin(1,"Backing Track",
    				 "An audio source, for when you dont have a guitar",
    				 1);
    registerParam(0,"Level","","","","",0,127,1,64);
    registerParam(1,"Playing","","","stop","play",0,1,1,0);

    loadFile("data/riff41.raw");
}

PluginBackingTrack::~PluginBackingTrack()
{
	free(_rawAudio);
}

int PluginBackingTrack::process(StereoBuffer* input)
{
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

	if (_bPlaying && _rawAudio)
	{
		for (int i=0;i<input->length;i++)
		{
			outLeft[i] = _rawAudio[_rawAudioPos++] * _fLevel;
			outRight[i] = _rawAudio[_rawAudioPos++] * _fLevel;

			if (_rawAudioPos >= _rawAudioLen)
				_rawAudioPos=0;
		}
	}
	else
	{
		for (int i=0;i<input->length;i++)
		{
			outLeft[i] = 0.0f;
			outRight[i] = 0.0f;
		}
	}

	return paContinue;
}

void PluginBackingTrack::setParam(int npar, int value)
{
	switch (npar)
	{
	case 0:	// level
		_nLevel = value < 0 ? 0 : value >127 ?127 : value;
		_fLevel =  2.0 * (_nLevel / 127.0 );
		break;
	case 1:	// play
		_nPlaying = value;
		_bPlaying = (bool)_nPlaying;
		_rawAudioPos=0;
		break;
	default:
		break;
	}
}

int PluginBackingTrack::getParam(int npar)
{
	switch (npar)
	{
	case 0:	// level
		return _nLevel;
		break;
	case 1:	// play
		return (int)_bPlaying;
		break;
	default:
		break;
	}
	return 0;
}

void PluginBackingTrack::loadFile(char* fname)
{
    _rawAudio=0;
	FILE* f = fopen(fname,"r");
	if (f)
	{
		fseek(f,0,SEEK_END);
		int size = ftell(f);
		rewind(f);

		int frames = size / sizeof(unsigned short);

		_rawAudio = (float*)malloc(frames*sizeof(float)*2);
		_rawAudioPos=0;
		_rawAudioLen=0;

		for (int i=0;i<frames;i++)
		{
			unsigned short frame[2];

			if (1!=fread(frame,sizeof(frame),1,f))
				break;

			_rawAudio[_rawAudioLen++] = frame[0] / 65535.0;
			_rawAudio[_rawAudioLen++] = frame[1] / 65535.0;
		}

		fclose(f);
	}

}
