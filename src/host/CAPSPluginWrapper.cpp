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

	_p=0;

	// get get the descriptor
	int i=0;
	const LADSPA_Descriptor* d=ladspa_descriptor(i);
	const LADSPA_Descriptor* _p=0;
	while (d)
	{
		if (strcasecmp(name.c_str(),d->Label)==0)
		{
			_p = d;
		}
		d=ladspa_descriptor(++i);
	}

	// if we got a descriptor
	if (_p)
	{
		// create actual instance
		_h = _p->instantiate(_p,SAMPLE_RATE);
		if (_h)
		{
			// register plugin itself
			registerPlugin(1,(char*)_p->Label,(char*)_p->Name,1);

			// loop over params and register
			for (i=0;i<_p->PortCount;i++)
			{
				LADSPA_PortDescriptor desc = _p->PortDescriptors[i];
				LADSPA_PortRangeHint hint = _p->PortRangeHints[i];
				const char* name = _p->PortNames[i];
				const char* meta = _p->PortMetaData[i];

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

					PluginParam* param=0;
					float multiplier = 1.0f;
					int offset=0;

					// todo get value from first preset
					value = min;

					if (labelJson && LADSPA_IS_HINT_INTEGER(hint.HintDescriptor))
					{
						// we have labels
						offset = min;

						vector<string> labels;

						// todo: parse that shit


						// decode labels
						// {0:'breathy',1:'fat A',2:'fat B',3:'fat C',4:'fat D'}

						// register, but there may be a offset
						param = registerParam(i,(char*)name,labels,value);
					}
					else
					{
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
					}

					// we have to have the values for params locally
					if (param)
					{
						_paramOffset.push_back(offset);
						_paramValues.push_back(value);
						_paramMultipliers.push_back(multiplier);
						_p->connect_port(_h,i,&(_paramValues[_paramValues.size()-1]));
					}
				}
			}	// end params loop

			// connect audio ports
			_inputBuffer = new StereoBuffer(PERIOD);

			for (i=0;i< (_inputAudioPorts.size()>2?2:_inputAudioPorts.size());i++ )
				_p->connect_port(_h,_inputAudioPorts[i],i==0?_inputBuffer->left:_inputBuffer->right);

			for (i=0;i< (_outputAudioPorts.size()>2?2:_outputAudioPorts.size());i++ )
				_p->connect_port(_h,_outputAudioPorts[i],i==0?_outputBuffers[0]->left:_outputBuffers[0]->right);
		}
	}

	if (_p && _h)
	{
		_p->activate(_h);
	}
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
	int i;

	// copy to input
	if (_inputAudioPorts.size()==1)
	{
		// stereo mixdown
		for (i=0;i<input->length;i++)
		{
			_inputBuffer->left[i]= (input->left[i] * 0.5) + (input->right[i] * 0.5);
		}
	}
	else
	{
		// standard stereo
		for (i=0;i<input->length;i++)
		{
			_inputBuffer->left[i]=input->left[i];
			_inputBuffer->right[i]=input->right[i];
		}
	}

	_p->run(_h,_inputBuffer->length);

	return paContinue;
}

void CAPSPluginWrapper::setParam(int npar, int value)
{
	if (npar>=0 && npar<=_paramValues.size())
	{
		_paramValues[npar]= (value / _paramMultipliers[npar])+_paramOffset[npar];
	}
}

int CAPSPluginWrapper::getParam(int npar)
{
	if (npar>=0 && npar<=_paramValues.size())
	{
		return (_paramValues[npar] * _paramMultipliers[npar])-_paramOffset[npar];
	}

	return 0;
}

