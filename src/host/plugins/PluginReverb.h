
#ifndef REVERB_H
#define REVERB_H
#include "global.h"
#include "../Plugin.h"
#include "../StereoBuffer.h"
#include "Freeverb/revmodel.h"

class PluginReverb : public Plugin
{

public:
    PluginReverb ();
    ~PluginReverb ();

	int process(StereoBuffer* input);

	void panic();

private:
    void setParam (int npar, int value);
    int getParam (int npar);

private:
    revmodel _model;
};

#endif
