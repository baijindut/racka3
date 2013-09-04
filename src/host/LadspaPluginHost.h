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

class LadspaPluginWrapper: public Plugin
{
public:
	LadspaPluginWrapper(LadspaPlugin* ladspaPlugin);
	virtual ~LadspaPluginWrapper();

	int process(StereoBuffer* input);

private:
    void setParam (int npar, int value);
    int getParam (int npar);

    LadspaPlugin* _p;
};

#endif /* LADSPAPLUGINHOST_H_ */
