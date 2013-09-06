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
    LADSPA_Handle _h;

    vector <float> _paramMultipliers;
    vector <float> _paramValues;
    vector <int> _paramOffset;

    vector <int> _inputAudioPorts;
    vector <int> _outputAudioPorts;

    StereoBuffer* _inputBuffer;
};

#endif /* LADSPAPLUGINHOST_H_ */
