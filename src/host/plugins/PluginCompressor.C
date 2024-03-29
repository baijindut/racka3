/*
  rakarrack - a guitar effects software

 Compressor.C  -  Compressor Effect
 Based on artscompressor.cc by Matthias Kretz <kretz@kde.org>
 Stefan Westerfeld <stefan@space.twc.de>

  Copyright (C) 2008-2010 Josep Andreu
  Author: Josep Andreu

	Patches:
	September 2009  Ryan Billing (a.k.a. Transmogrifox)
		--Modified DSP code to fix discontinuous gain change at threshold.
		--Improved automatic gain adjustment function
		--Improved handling of knee
		--Added support for user-adjustable knee
		--See inline comments

 This program is free software; you can redistribute it and/or modify
 it under the terms of version 2 of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License (version 2) for more details.

 You should have received a copy of the GNU General Public License
 (version2)  along with this program; if not, write to the Free Software
 Foundation,
 Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <math.h>
#include "PluginCompressor.h"
#include "portaudio.h"
#define  MIN_GAIN  0.00001f        // -100dB  This will help prevent evaluation of denormal numbers

PluginCompressor::PluginCompressor ()
{
    rvolume = 0.0f;
    rvolume_db = 0.0f;
    lvolume = 0.0f;
    lvolume_db = 0.0f;
    tthreshold = -24;
    tratio = 4;
    toutput = -10;
    tatt = 20;
    trel = 50;
    a_out = 1;
    stereo = 0;
    tknee = 30;
    rgain = 1.0f;
    rgain_old = 1.0f;
    lgain = 1.0f;
    lgain_old = 1.0f;
    lgain_t = 1.0f;
    rgain_t = 1.0f;
    ratio = 1.0;
    kpct = 0.0f;
    peak = 0;
    lpeak = 0.0f;
    rpeak = 0.0f;
    rell = relr = attr = attl = 1.0f;

    ltimer = rtimer = 0;
    hold = (int) (SAMPLE_RATE*0.0125);  //12.5ms
    clipping = 0;
    limit = 0;

    registerPlugin(1,"Compressor",
    				   "Art Compressor",
    				   1);

    registerParam(1,"Threshold","","dB","","",-60,-3,1,-30);
    registerParam(2,"Ratio","","","","",2,42,1,2);
    registerParam(3,"Output","","","","",-40,0,1,-6);
    registerParam(4,"Attack","","ms","","",10,250,1,20);
    registerParam(5,"Release","","ms","","",0,500,1,120);
    registerParam(6,"Auto Output","","","off","on",0,1,1,1);
    registerParam(7,"Knee","","","","",0,100,1,0);
    registerParam(8,"Stereo","Take level from both channels","","off","on",0,1,1,0);
    registerParam(9,"Peak","Peak mode Compression","","off","on",0,1,1,0);
}

PluginCompressor::~PluginCompressor ()
{
}


void PluginCompressor::cleanup ()
{

    lgain = rgain = 1.0f;
    lgain_old = rgain_old = 1.0f;
    rpeak = 0.0f;
    lpeak = 0.0f;
    limit = 0;
    clipping = 0;
}


void PluginCompressor::setParam (int np, int value)
{

    switch (np) {

    case 1:
        tthreshold = value;
        thres_db = (float)tthreshold;    //implicit type cast int to float
        break;

    case 2:
        tratio = value;
        ratio = (float)tratio;
        break;

    case 3:
        toutput = value;
        break;

    case 4:
        tatt = value;
        att = cSAMPLE_RATE /(((float)value / 1000.0f) + cSAMPLE_RATE);
        attr = att;
        attl = att;
        break;

    case 5:
        trel = value;
        rel = cSAMPLE_RATE /(((float)value / 1000.0f) + cSAMPLE_RATE);
        rell = rel;
        relr = rel;
        break;

    case 6:
        a_out = value;
        break;

    case 7:
        tknee = value;  //knee expressed a percentage of range between thresh and zero dB
        kpct = (float)tknee/100.1f;
        break;

    case 8:
        stereo = value;
        break;
    case 9:
        peak = value;
        break;
    }

    kratio = logf(ratio)/LOG_2;  //  Log base 2 relationship matches slope
    knee = -kpct*thres_db;

    coeff_kratio = 1.0 / kratio;
    coeff_ratio = 1.0 / ratio;
    coeff_knee = 1.0 / knee;

    coeff_kk = knee * coeff_kratio;


    thres_mx = thres_db + knee;  //This is the value of the input when the output is at t+k
    makeup = -thres_db - knee/kratio + thres_mx/ratio;
    makeuplin = dB2rap(makeup);
    if (a_out)
        outlevel = dB2rap((float)toutput) * makeuplin;
    else
        outlevel = dB2rap((float)toutput);

}

int PluginCompressor::getParam (int np)
{
    switch (np)

    {

    case 1:
        return (tthreshold);
        break;
    case 2:
        return (tratio);
        break;
    case 3:
        return (toutput);
        break;
    case 4:
        return (tatt);
        break;
    case 5:
        return (trel);
        break;
    case 6:
        return (a_out);
        break;
    case 7:
        return (tknee);
        break;
    case 8:
        return (stereo);
        break;
    case 9:
        return (peak);
        break;
    }

    return (0);

}

int PluginCompressor::process(StereoBuffer* input)
{
    int i;
	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

	for (i=0;i<PERIOD;i++)
	{
		outLeft[i]=inLeft[i];
		outRight[i]=inRight[i];
	}

    for (i = 0; i < PERIOD; i++)
    {
        float rdelta = 0.0f;
        float ldelta = 0.0f;
        //Right Channel

        if(peak) {
            if (rtimer > hold) {
                rpeak *= 0.9998f;   //The magic number corresponds to ~0.1s based on T/(RC + T),
                rtimer--;
            }
            if (ltimer > hold) {
                lpeak *= 0.9998f;	//leaky peak detector.
                ltimer --;  //keeps the timer from eventually exceeding max int & rolling over
            }
            ltimer++;
            rtimer++;
            if(rpeak<fabs(outRight[i])) {
                rpeak = fabs(outRight[i]);
                rtimer = 0;
            }
            if(lpeak<fabs(outLeft[i])) {
                lpeak = fabs(outLeft[i]);
                ltimer = 0;
            }

            if(lpeak>20.0f) lpeak = 20.0f;
            if(rpeak>20.0f) rpeak = 20.0f; //keeps limiter from getting locked up when signal levels go way out of bounds (like hundreds)

        } else {
            rpeak = outRight[i];
            lpeak = outLeft[i];
        }

        if(stereo) {
            rdelta = fabsf (rpeak);
            if(rvolume < 0.9f) {
                attr = att;
                relr = rel;
            } else if (rvolume < 1.0f) {
                attr = att + ((1.0f - att)*(rvolume - 0.9f)*10.0f);	//dynamically change attack time for limiting mode
                relr = rel/(1.0f + (rvolume - 0.9f)*9.0f);  //release time gets longer when signal is above limiting
            } else {
                attr = 1.0f;
                relr = rel*0.1f;
            }

            if (rdelta > rvolume)
                rvolume = attr * rdelta + (1.0f - attr)*rvolume;
            else
                rvolume = relr * rdelta + (1.0f - relr)*rvolume;


            rvolume_db = rap2dB (rvolume);
            if (rvolume_db < thres_db) {
                rgain = outlevel;
            } else if (rvolume_db < thres_mx) {
                //Dynamic ratio that depends on volume.  As can be seen, ratio starts
                //at something negligibly larger than 1 once volume exceeds thres, and increases toward selected
                // ratio by the time it has reached thres_mx.  --Transmogrifox

                eratio = 1.0f + (kratio-1.0f)*(rvolume_db-thres_db)* coeff_knee;
                rgain =   outlevel*dB2rap(thres_db + (rvolume_db-thres_db)/eratio - rvolume_db);
            } else {
                rgain = outlevel*dB2rap(thres_db + coeff_kk + (rvolume_db-thres_mx)*coeff_ratio - rvolume_db);
                limit = 1;
            }

            if ( rgain < MIN_GAIN) rgain = MIN_GAIN;
            rgain_t = .4f * rgain + .6f * rgain_old;
        };

//Left Channel
        if(stereo)  {
            ldelta = fabsf (lpeak);
        } else  {
            ldelta = 0.5f*(fabsf (lpeak) + fabsf (rpeak));
        };  //It's not as efficient to check twice, but it's small expense worth code clarity

        if(lvolume < 0.9f) {
            attl = att;
            rell = rel;
        } else if (lvolume < 1.0f) {
            attl = att + ((1.0f - att)*(lvolume - 0.9f)*10.0f);	//dynamically change attack time for limiting mode
            rell = rel/(1.0f + (lvolume - 0.9f)*9.0f);  //release time gets longer when signal is above limiting
        } else {
            attl = 1.0f;
            rell = rel*0.1f;
        }

        if (ldelta > lvolume)
            lvolume = attl * ldelta + (1.0f - attl)*lvolume;
        else
            lvolume = rell*ldelta + (1.0f - rell)*lvolume;

        lvolume_db = rap2dB (lvolume);

        if (lvolume_db < thres_db) {
            lgain = outlevel;
        } else if (lvolume_db < thres_mx) { //knee region
            eratio = 1.0f + (kratio-1.0f)*(lvolume_db-thres_db)* coeff_knee;
            lgain =   outlevel*dB2rap(thres_db + (lvolume_db-thres_db)/eratio - lvolume_db);
        } else {
            lgain = outlevel*dB2rap(thres_db + coeff_kk + (lvolume_db-thres_mx)*coeff_ratio - lvolume_db);
            limit = 1;
        }


        if ( lgain < MIN_GAIN) lgain = MIN_GAIN;
        lgain_t = .4f * lgain + .6f * lgain_old;

        if (stereo) {
            outLeft[i] *= lgain_t;
            outRight[i] *= rgain_t;
            rgain_old = rgain;
            lgain_old = lgain;
        } else {
            outLeft[i] *= lgain_t;
            outRight[i] *= lgain_t;
            lgain_old = lgain;
        }

        if(peak) {
            if(outLeft[i]>0.999f) {            //output hard limiting
                outLeft[i] = 0.999f;
                clipping = 1;
            }
            if(outLeft[i]<-0.999f) {
                outLeft[i] = -0.999f;
                clipping = 1;
            }
            if(outRight[i]>0.999f) {
                outRight[i] = 0.999f;
                clipping = 1;
            }
            if(outRight[i]<-0.999f) {
                outRight[i] = -0.999f;
                clipping = 1;
            }
            //highly probably there is a more elegant way to do that, but what the hey...
        }
    }

    return paContinue;
}

