/*
 * LadspaPluginHost.h
 *
 *  Created on: 4 Sep 2013
 *      Author: lenovo
 */

#ifndef LADSPAPLUGINHOST_H_
#define LADSPAPLUGINHOST_H_

#include "caps-0.9.16/basics.h"
#include "Plugin.h"

#include <string>

#define CAPSMAXPARAMS 128


class CAPSPluginWrapper: public Plugin
{
public:
	CAPSPluginWrapper();
	virtual ~CAPSPluginWrapper();

	bool loadCapsPlugin(string name);

	int process(StereoBuffer* input);

private:
    void setParam (int npar, int value);
    int getParam (int npar);

    const LADSPA_Descriptor* _p;
    LADSPA_Handle _h[2]; // in case we need left and right
    int _instanceCount;

    float _paramMultipliers[CAPSMAXPARAMS];
    float _paramValues[CAPSMAXPARAMS];
    int _paramOffset[CAPSMAXPARAMS];
    bool _paramLegit[CAPSMAXPARAMS];

    vector <int> _inputAudioPorts;
    vector <int> _outputAudioPorts;

    float* _monoBuffer;
};

#endif /* LADSPAPLUGINHOST_H_ */
