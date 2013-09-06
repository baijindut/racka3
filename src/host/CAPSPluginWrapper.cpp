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
#include <algorithm>

CAPSPluginWrapper::CAPSPluginWrapper()
{
	_p=0;
	_h[0]=0;
	_h[1]=0;
	_instanceCount=0;
}

bool CAPSPluginWrapper::loadCapsPlugin(string  name)
{
	vector <string> names;

	_p=0;

	// get get the descriptor
	int i=0;
	const LADSPA_Descriptor* d=ladspa_descriptor(i);
	while (d)
	{
		if (strcasecmp(name.substr(strlen(CAPS)).c_str(),d->Label)==0)
		{
			_p = d;
		}
		d=ladspa_descriptor(++i);
	}

	// if we got a descriptor
	if (_p)
	{
		// register plugin itself
		char pname[128];
		sprintf(pname,"%s%s",CAPS,_p->Label);
		registerPlugin(1,pname,(char*)_p->Name,1);

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
				vector<string> labels;
				bool doRegister=false;

				// todo get value from first preset

				value = min;

				if (labelJson && LADSPA_IS_HINT_INTEGER(hint.HintDescriptor))
				{
					// we have labels
					offset = min;

					// decode labels
					// they look like this:
					// {0:'breathy',1:'fat A',2:'fat B',3:'fat C',4:'fat D'}
					char* txt = (char*)malloc(strlen(labelJson)+1);
					char* a=labelJson;
					char* b=txt;
					while (*a)
					{
						if (*a != '{' && *a !='}' && *a != '\'')
						{
							*b = *a;
							b++;
						}
						a++;
					}
					*b=0;
					// now we have txt with just comma seperated key:value
					char* tok = strtok (txt,",");
					while (tok != NULL)
					{
						int key;
						char value[128];
						if (2==sscanf(tok,"%d:%[^\t\n]",&key,value))
						{
							labels.push_back(string(value));
						}

						tok = strtok (NULL, ",");
					}

					free(txt);

					// register, but there may be a offset
					doRegister = true;
				}
				else
				{
					if (LADSPA_IS_HINT_INTEGER(hint.HintDescriptor))
					{
						doRegister=true;
					}
					else
					{
						if (min==0 && max ==1)
						{
							multiplier = 127;
							min=0;max=127;
						}
						doRegister=true;
					}
				}

				// we have to have the values for params locally
				if (doRegister)
				{
					_paramOffset.push_back(offset);
					_paramValues.push_back(value);
					_paramMultipliers.push_back(multiplier);

					if (labels.size())
						param = registerParam(i,(char*)name,labels,value);
					else
						param = registerParam(i,(char*)name,"","","","",min,max,1,value*multiplier);
				}
			}
		}	// end params loop
	}

	// create actual instance(s)
	// scenarios:
	// mono input, stereo output: NOT ALLOWED
	// mono input, mono output: 2 instances
	// stereo input, stereo output: 1 instances

	assert(_inputAudioPorts.size() && _outputAudioPorts.size());
	assert(! (_inputAudioPorts.size()==1 && _outputAudioPorts.size()==2));

	_h[0] = _p->instantiate(_p,SAMPLE_RATE);
	_instanceCount =1;
	if (_inputAudioPorts.size()==1 && _outputAudioPorts.size()==1) {
		_h[1] = _p->instantiate(_p,SAMPLE_RATE);
		_instanceCount=2;
	}
	else
		_h[1]=0;

	if (_p)
	{
		for (i=0;i<_instanceCount;i++)
			_p->activate(_h[i]);

		return true;
	}

	return false;
}

CAPSPluginWrapper::~CAPSPluginWrapper()
{
	int i;

	for (i=0;i<_instanceCount;i++)
	{
		if (_p->deactivate)
			_p->deactivate(_h[i]);

		if (_p->cleanup)
			_p->cleanup(_h[i]);
	}

	// delete _p ?
}

int CAPSPluginWrapper::process(StereoBuffer* input)
{
	int i;

	if (_instanceCount==1)
	{
		// connect all ports
		for (i=0;i<_paramValues.size();i++) {
			_p->connect_port(_h[0],i,&(_paramValues[i]));
		}

		for (i=0;i< (_inputAudioPorts.size()>2?2:_inputAudioPorts.size());i++ ) {
			int port = _inputAudioPorts[i];
			_p->connect_port(_h[0],port,i==0?input->left:input->right);
		}

		for (i=0;i< (_outputAudioPorts.size()>2?2:_outputAudioPorts.size());i++ ) {
			int port = _outputAudioPorts[i];
			_p->connect_port(_h[0],port,i==0?_outputBuffers[0]->left:_outputBuffers[0]->right);
		}

		_p->run(_h[0],input->length);
	}
	else if (_instanceCount==2)
	{
		// connect all ports
		for (int inst=0;inst<2;inst++)
		{
			for (i=0;i<_paramValues.size();i++) {
				_p->connect_port(_h[inst],i,&(_paramValues[i]));
			}
		}

		int inPort = _inputAudioPorts[0];
		int outPort = _outputAudioPorts[0];

		_p->connect_port(_h[0],inPort,input->left);
		_p->connect_port(_h[1],inPort,input->right);

		_p->connect_port(_h[0],outPort,_outputBuffers[0]->left);
		_p->connect_port(_h[1],outPort,_outputBuffers[0]->right);

		_p->run(_h[0],input->length);
		_p->run(_h[1],input->length);
	}

	return paContinue;
}

void CAPSPluginWrapper::setParam(int npar, int value)
{
	if (npar>=0 && npar<=_paramValues.size())
	{
		float val = (value / _paramMultipliers[npar])+_paramOffset[npar];
		//printf("set val = %f\n",val);
		_paramValues[npar]= val;
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

