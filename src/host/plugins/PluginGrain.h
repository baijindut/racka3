
#ifndef GRAIN_H
#define GRAIN_H
#include "global.h"

#include "../Plugin.h"
#include "../StereoBuffer.h"

class GrainScatter;
class PluginGrain : public Plugin
{

public:
    PluginGrain ();
    ~PluginGrain ();

	int process(StereoBuffer* input);

	void panic();

private:
    void setParam (int npar, int value);
    int getParam (int npar);
    void cleanup ();

private:
    float _density;
    float _scatter;
    float _glength;
    float _gattack;

    GrainScatter* _leftProcesor;
    GrainScatter* _rightProcesor;
};

#endif
