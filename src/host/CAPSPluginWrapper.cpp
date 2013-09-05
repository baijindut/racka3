/*
 * LadspaPluginHost.cpp
 *
 *  Created on: 4 Sep 2013
 *      Author: lenovo
 */

#include "CAPSPluginWrapper.h"
#include "caps-0.9.16/ladspa.h"

#include <vector>
#include <string>
#include "settings.h"


CAPSPluginWrapper::CAPSPluginWrapper(string  name)
{
	vector <string> names;

	// get get the descriptor
	int i=0;
	const LADSPA_Descriptor* d=ladspa_descriptor(i);
	const LADSPA_Descriptor* c=0;
	while (d)
	{
		if (strcasecmp(name.c_str(),d->Label)==0)
		{
			c = d;
		}
		d=ladspa_descriptor(++i);
	}

	// if we got a descriptor
	if (c)
	{
		// create actual instance
		_h = c->instantiate(c,SAMPLE_RATE);
		if (_h)
		{



			// register plugin itself
			registerPlugin(1,(char*)c->Label,(char*)c->Name,1);

			// loop over params and register
			for (i=0;i<c->PortCount;i++)
			{
				LADSPA_PortDescriptor desc = c->PortDescriptors[i];
				LADSPA_PortRangeHint hint = c->PortRangeHints[i];
				const char* name = c->PortNames[i];
				const char* meta = c->PortMetaData[i];

				float min,max,step,value;
				char* labelJson;

				labelJson = (char*)meta;

				if (LADSPA_IS_PORT_AUDIO(desc))
				{
					// audio port
					if (LADSPA_IS_PORT_INPUT(desc))
						_inputAudioPorts.push_back(i);
					else
						_outputAudioPorts.push_back(i);
				}
				else if (LADSPA_IS_PORT_CONTROL(desc) && LADSPA_IS_PORT_INPUT(desc))
				{
					// param
					min = hint.LowerBound;
					max = hint.UpperBound;

					if (labelJson)
					{
						// we have labels

						// decode labels

						// register, but there may be a offset
					}
					else
					{
						// todo get vale from preset
						value = min;

						PluginParam* param;
						float multiplier = 1.0f;

						if (LADSPA_IS_HINT_INTEGER(hint.HintDescriptor))
						{
							param = registerParam(i,(char*)name,"","","","",min,max,1,value);
						}
						else
						{
							if (min==0 && max ==1)
							{
								multiplier = 127;
							}
							param = registerParam(i,(char*)name,"","","","",0,127,1,value*multiplier);

						}

						// we have o have the values for params locally
						_paramValues.push_back(value);
						_paramMultipliers.push_back(multiplier);
						c->connect_port(_h,i,&(_paramValues[_paramValues.size()-1]));
					}
				}
			}	// end params loop

			// connect audio ports
			// act depending on number of ports _inputAudioPorts
			//c->connect_port(_h,i,&(_paramValues[_paramValues.size()-1]));
		}
	}

	_p=c;
}

CAPSPluginWrapper::~CAPSPluginWrapper()
{
	if (_p && _h)
	{
		_p->deactivate(_h);
	}
}

int CAPSPluginWrapper::process(StereoBuffer* input)
{
	// todo

	return paContinue;
}

void CAPSPluginWrapper::setParam(int npar, int value)
{
	if (npar>=0 && npar<=_paramValues.size())
	{
		_paramValues[npar]= value / _paramMultipliers[npar];
	}
}

int CAPSPluginWrapper::getParam(int npar)
{
	if (npar>=0 && npar<=_paramValues.size())
	{
		return _paramValues[npar] * _paramMultipliers[npar];
	}

	return 0;
}

