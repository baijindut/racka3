/*
 * PluginMixSplitter.cpp
 *
 *  Created on: 6 Aug 2013
 *      Author: lenovo
 */

#include "PluginMixSplitter.h"

PluginMixSplitter::PluginMixSplitter()
{
	_type = PLUGIN_SPLITTER;

	_nSplit = 63;
	_fA = _fB = 0.5;

	registerPlugin(2,"Mix Splitter","Splits the current signal into two",1);
	registerParam(1,"A/B Mix","How much signal to which path","","Only A","Only B",0,127,1,63);
}

PluginMixSplitter::~PluginMixSplitter()
{

}

int PluginMixSplitter::process(StereoBuffer* input)
{
    int i;
	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeftA = _outputBuffers[0]->left;
	float* outRightA = _outputBuffers[0]->right;
	float* outLeftB = _outputBuffers[1]->left;
	float* outRightB = _outputBuffers[1]->right;

	// special case for equal split
	if (_nSplit==63)
	{
		for (i=0;i<input->length;input++)
		{
			outLeftA[i] = outLeftB[i] = (inLeft[i] * 0.5);
			outRightA[i] = outRightB[i] = (inRight[i] * 0.5);
		}
	}
	else
	{
		for (i=0;i<input->length;input++)
		{
			outLeftA[i] = inLeft[i] * _fA;
			outRightA[i] = inRight[i] * _fA;

			outLeftB[i] = inLeft[i] * _fB;
			outRightB[i] = inRight[i] * _fB;
		}
	}

	return paContinue;
}

void PluginMixSplitter::panic()
{
	Plugin::panic();
}

void PluginMixSplitter::setParam(int npar, int value)
{
	switch (npar)
	{
	case 1:
		_nSplit = value <= 0 ? 0 : value > 127 ? 127 : value;
		_fA = _nSplit / 127.0;
		_fB = 1 - _fA;
		break;
	default:
		break;
	}
}

int PluginMixSplitter::getParam(int npar)
{
	switch (npar)
	{
	case 1:
		return _nSplit;
		break;
	default:
		break;
	}
	return 0;
}
