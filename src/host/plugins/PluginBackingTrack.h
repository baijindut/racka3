/*
 * PluginBackingTrack.h
 *
 *  Created on: 1 Aug 2013
 *      Author: lenovo
 */

#ifndef PLUGINBACKINGTRACK_H_
#define PLUGINBACKINGTRACK_H_

#include "../Plugin.h"

class PluginBackingTrack: public Plugin {
public:
	PluginBackingTrack();
	virtual ~PluginBackingTrack();

	int process(StereoBuffer* input);

private:
    void setParam (int npar, int value);
    int getParam (int npar);

private:
    int _nLevel;
    float _fLevel;

    int _nPlaying;
    bool _bPlaying;

    float* _rawAudio;
    int _rawAudioLen;
    int _rawAudioPos;

    int _nHold;

    int _nCurrentHold;
};

#endif /* PLUGINBACKINGTRACK_H_ */
