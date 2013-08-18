
#include <math.h>
#include "PluginReverb.h"
#include <stdio.h>
#include "portaudio.h"

PluginReverb::PluginReverb ()
{
    registerPlugin(1,"Reverb",
    				   "Freeverb",
    				   1);

    registerParam(1,"Freeze","","","","",0,1,1,0);
    registerParam(2,"Room Size","","","","",0,127,1,63);
    registerParam(3,"Damping","","","","",	0,127,1,63);
    registerParam(4,"Wet","","","","",		0,127,1,63);
    registerParam(5,"Dry","","","","",		0,127,1,63);
    registerParam(6,"Width","","","","",	0,127,1,63);

};

PluginReverb::~PluginReverb ()
{

};



int PluginReverb::process(StereoBuffer* input)
{
	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

	_model.processreplace(inLeft,inRight,outLeft,outRight,input->length,1);

    return paContinue;
}


void
PluginReverb::setParam (int npar, int value)
{
    switch (npar) {
    case 1:
    	_model.setmode(value>0?1:0);
    	break;
    case 2:
    	_model.setroomsize(value / 127.0);
    	break;
    case 3:
    	_model.setdamp(value / 127.0);
    	break;
    case 4:
    	_model.setwet(value / 127.0);
    	break;
    case 5:
    	_model.setdry(value / 127.0);
    	break;
    case 6:
    	_model.setwidth(value / 127.0);
    	break;
	default:
		break;
    };
};

int
PluginReverb::getParam (int npar)
{
    switch (npar) {
    case 1:
    	return (int)_model.getmode();
    	break;
    case 2:
    	return _model.getroomsize() * 127;
    	break;
    case 3:
    	return _model.getdamp() * 127;
    	break;
    case 4:
    	return _model.getwet() * 127;
    	break;
    case 5:
    	return _model.getdry() * 127;
    	break;
    case 6:
    	return _model.getwidth() * 127;
    	break;
	default:
		break;
    };

    return (0);
};
