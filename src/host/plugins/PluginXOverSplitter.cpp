/*
 * PluginFreqSplitter.cpp
 *
 *  Created on: 6 Aug 2013
 *      Author: lenovo
 */

#include "PluginXOverSplitter.h"

PluginXOver::PluginXOver()
{
	_type = PLUGIN_SPLITTER;

	_nSplitFreq = 440;

    lpfl = new AnalogFilter (2, _nSplitFreq, 1, 0);
    lpfr = new AnalogFilter (2, _nSplitFreq, 1, 0);
    hpfl = new AnalogFilter (3, _nSplitFreq, 1, 0);
    hpfr = new AnalogFilter (3, _nSplitFreq, 1, 0);

	registerPlugin(2,"Crossover Splitter","Splits the current signal into two",1);
	registerParam(1,"Frequency","Cutoff for split","","All A","All B",0,5000,1,440);
}

PluginXOver::~PluginXOver()
{
	delete lpfl;
	delete lpfr;
	delete hpfl;
	delete hpfr;
}

int PluginXOver::process(StereoBuffer* input)
{
    int i;
	float* inLeft = input->left;
	float* inRight = input->right;

	float* outLeftA = _outputBuffers[0]->left;
	float* outRightA = _outputBuffers[0]->right;

	float* outLeftB = _outputBuffers[1]->left;
	float* outRightB = _outputBuffers[1]->right;

	// split input to both output buffers
	for (i=0;i<input->length;i++)
	{
		outLeftA[i]=inLeft[i] / 2.0;
		outRightA[i]=inRight[i] / 2.0;
		outLeftB[i]=outLeftA[i];
		outRightB[i]=outRightA[i];
	}

	// perform filter in output buffers
    lpfl->filterout (outLeftA);
    lpfr->filterout (outRightA);
    hpfl->filterout (outLeftB);
    hpfr->filterout (outRightB);

	return paContinue;
}

void PluginXOver::panic()
{
	lpfl->cleanup ();
	hpfl->cleanup ();
	lpfr->cleanup ();
	hpfr->cleanup ();
	Plugin::panic();
}

void PluginXOver::setParam(int npar, int value)
{
	switch (npar)
	{
	case 1:
		_nSplitFreq = value;
		lpfl->setfreq(_nSplitFreq);
		lpfr->setfreq(_nSplitFreq);
		hpfl->setfreq(_nSplitFreq);
		hpfr->setfreq(_nSplitFreq);
		break;
	default:
		break;
	}
}

int PluginXOver::getParam(int npar)
{
	switch (npar)
	{
	case 1:
		return _nSplitFreq;
		break;
	default:
		break;
	}
	return 0;
}
