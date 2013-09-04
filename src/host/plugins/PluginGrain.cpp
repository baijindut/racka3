/* grain.cpp

 Computer Music Toolkit - a library of LADSPA plugins. Copyright (C)
 2000-2002 Richard W.E. Furse. The author may be contacted at
 richard@muse.demon.co.uk.

 modified a bit by chay for racka3

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public Licence as
 published by the Free Software Foundation; either version 2 of the
 Licence, or (at your option) any later version.

 This library is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307, USA. */

#include "PluginGrain.h"
#include <stdio.h>
#include "portaudio.h"
#include "../Plugin.h"

#define GRN_INPUT        0
#define GRN_OUTPUT       1
#define GRN_DENSITY      2
#define GRN_SCATTER      3
#define GRN_GRAIN_LENGTH 4
#define GRN_GRAIN_ATTACK 5

/** Period (in seconds) from which grains are selected. */
#define GRAIN_MAXIMUM_HISTORY  6
#define GRAIN_MAXIMUM_BLOCK    1 /* (seconds) */

#define GRAIN_MAXIMUM_SCATTER (GRAIN_MAXIMUM_HISTORY - GRAIN_MAXIMUM_BLOCK)
#define GRAIN_MAXIMUM_LENGTH  (GRAIN_MAXIMUM_HISTORY - GRAIN_MAXIMUM_BLOCK)

/** What quality should we require when sampling the normal
 distribution to generate grain counts? */
#define GRAIN_NORMAL_RV_QUALITY 16

/*****************************************************************************/

inline float
BOUNDED_BELOW(const float fData,
	      const float fLowerBound) {
  if (fData <= fLowerBound)
    return fLowerBound;
  else
    return fData;
}

inline float BOUNDED_ABOVE(const float fData,
				 const float fUpperBound) {
  if (fData >= fUpperBound)
    return fUpperBound;
  else
    return fData;
}

inline double
sampleNormalDistribution(const double dMean,
			 const double dStandardDeviation,
			 const long   lQuality = 12) {

  double dValue = 0;
  for (long lIter = 0; lIter < lQuality; lIter++)
    dValue += rand();

  double dSampleFromNormal01 = (dValue / RAND_MAX) - (lQuality * 0.5);

  return dMean + dStandardDeviation * dSampleFromNormal01;
}

/** Pointers to this can be used as linked list of grains. */
class Grain
{
private:

	long m_lReadPointer;
	long m_lGrainLength;
	long m_lAttackTime;

	long m_lRunTime;

	bool m_bFinished;

	float m_fAttackSlope;
	float m_fDecaySlope;

public:

	Grain(const long lReadPointer, const long lGrainLength,
			const long lAttackTime) :
			m_lReadPointer(lReadPointer), m_lGrainLength(lGrainLength), m_lAttackTime(
					lAttackTime), m_lRunTime(0), m_bFinished(false)
	{
		if (lAttackTime <= 0)
		{
			m_fAttackSlope = 0;
			m_fDecaySlope = float(1.0 / lGrainLength);
		}
		else
		{
			m_fAttackSlope = float(1.0 / lAttackTime);
			if (lAttackTime >= lGrainLength)
				m_fDecaySlope = 0;
			else
				m_fDecaySlope = float(1.0 / (lGrainLength - lAttackTime));
		}
	}

	bool isFinished() const
	{
		return m_bFinished;
	}

	/** NULL if end of grain list. */
	Grain * m_poNextGrain;

	void run(const unsigned long lSampleCount, float * pfOutput,
			const float * pfHistoryBuffer,
			const unsigned long lHistoryBufferSize)
	{

		float fAmp;
		if (m_lRunTime < m_lAttackTime)
			fAmp = m_fAttackSlope * m_lRunTime;
		else
			fAmp = m_fDecaySlope * (m_lGrainLength - m_lRunTime);

		for (unsigned long lSampleIndex = 0; lSampleIndex < lSampleCount;
				lSampleIndex++)
		{

			if (fAmp < 0)
			{
				m_bFinished = true;
				break;
			}

			*(pfOutput++) += fAmp * pfHistoryBuffer[m_lReadPointer];

			m_lReadPointer = (m_lReadPointer + 1) & (lHistoryBufferSize - 1);

			if (m_lRunTime < m_lAttackTime)
				fAmp += m_fAttackSlope;
			else
				fAmp -= m_fDecaySlope;

			m_lRunTime++;
		}
	}
};

/** This plugin cuts an audio stream up and uses it to generate a
 granular texture. */
class GrainScatter
{
private:

	Grain * m_poCurrentGrains;

	long m_lSampleRate;

	float * m_pfBuffer;

	/** Buffer size, a power of two. */
	unsigned long m_lBufferSize;

	/** Write pointer in buffer. */
	unsigned long m_lWritePointer;

    float _density;
    float _scatter;
    float _glength;
    float _gattack;

    float _elapsed;
    float _nextwait;

public:

	GrainScatter(unsigned long lSampleRate) :
			m_poCurrentGrains(NULL), m_lSampleRate(lSampleRate)
	{
		m_lWritePointer = 0;
		_elapsed=0;
		_nextwait=0;
		/* Buffer size is a power of two bigger than max delay time. */
		unsigned long lMinimumBufferSize = (unsigned long) ((float) lSampleRate
				* GRAIN_MAXIMUM_HISTORY);
		m_lBufferSize = 1;
		while (m_lBufferSize < lMinimumBufferSize)
			m_lBufferSize <<= 1;
		m_pfBuffer = new float[m_lBufferSize];
	}

	~GrainScatter()
	{
		delete[] m_pfBuffer;
	}

	void setParam(int npar, float value);
	void activateGrainScatter();
	void runGrainScatter(float* in, float* out,unsigned long SampleCount);

};

void GrainScatter::setParam(int npar, float value)
{
	switch (npar)
	{
	case 1:
		_density = value;
		break;
	case 2:
		_scatter = value;
		break;
	case 3:
		_glength = value;
		break;
	case 4:
		_gattack = value;
		break;
	default:
		break;
	};
}
;

/*****************************************************************************/

/** Initialise and activate a plugin instance. */
void GrainScatter::activateGrainScatter()
{

	GrainScatter * poGrainScatter = this;

	/* Need to reset the delay history in this function rather than
	 instantiate() in case deactivate() followed by activate() have
	 been called to reinitialise a delay line. */
	memset(poGrainScatter->m_pfBuffer, 0,
			sizeof(float) * poGrainScatter->m_lBufferSize);

	poGrainScatter->m_lWritePointer = 0;
}

/*****************************************************************************/

void GrainScatter::runGrainScatter(float* in, float* out,unsigned long SampleCount)
{

	GrainScatter * poGrainScatter = this;

	float * pfInput = in;
	float * pfOutput = out;


	/* Move the delay line along. */
	if (poGrainScatter->m_lWritePointer + SampleCount
			> poGrainScatter->m_lBufferSize)
	{
		memcpy(poGrainScatter->m_pfBuffer + poGrainScatter->m_lWritePointer,
				pfInput,
				sizeof(float)
						* (poGrainScatter->m_lBufferSize
								- poGrainScatter->m_lWritePointer));
		memcpy(poGrainScatter->m_pfBuffer,
				pfInput
						+ (poGrainScatter->m_lBufferSize
								- poGrainScatter->m_lWritePointer),
				sizeof(float)
						* (SampleCount
								- (poGrainScatter->m_lBufferSize
										- poGrainScatter->m_lWritePointer)));
	}
	else
	{
		memcpy(poGrainScatter->m_pfBuffer + poGrainScatter->m_lWritePointer,
				pfInput, sizeof(float) * SampleCount);
	}
	poGrainScatter->m_lWritePointer = ((poGrainScatter->m_lWritePointer
			+ SampleCount) & (poGrainScatter->m_lBufferSize - 1));

	/* Empty the output buffer. */
	memset(pfOutput, 0, SampleCount * sizeof(float));

	/* Process current grains. */
	Grain ** ppoGrainReference = &(poGrainScatter->m_poCurrentGrains);
	while (*ppoGrainReference != NULL)
	{
		(*ppoGrainReference)->run(SampleCount, pfOutput,
				poGrainScatter->m_pfBuffer, poGrainScatter->m_lBufferSize);
		if ((*ppoGrainReference)->isFinished())
		{
			Grain *poNextGrain = (*ppoGrainReference)->m_poNextGrain;
			delete *ppoGrainReference;
			*ppoGrainReference = poNextGrain;
		}
		else
		{
			ppoGrainReference = &((*ppoGrainReference)->m_poNextGrain);
		}
	}

	float fSampleRate = float(poGrainScatter->m_lSampleRate);
	float fDensity = _density;

	/* We want to average fDensity new grains per second. We need to
	 use a RNG to generate a new grain count from the fraction of a
	 second we are dealing with. Use a normal distribution and
	 choose standard deviation also to be fDensity. This could be
	 separately parameterised but any guarantees could be confusing
	 given that individual grains are uniformly distributed within
	 the block. Note that fDensity isn't quite grains/sec as we
	 discard negative samples from the RV. */
/*	double dGrainCountRV_Mean = fDensity * SampleCount / fSampleRate;
	double dGrainCountRV_SD = dGrainCountRV_Mean;
	double dGrainCountRV = sampleNormalDistribution(dGrainCountRV_Mean, dGrainCountRV_SD, GRAIN_NORMAL_RV_QUALITY);

	unsigned long lNewGrainCount = 0;
	if (dGrainCountRV > 0)
		lNewGrainCount = (unsigned long) (0.5 + dGrainCountRV); */

	unsigned long lNewGrainCount = 0;

	_elapsed += (SampleCount / fSampleRate);
	if (_elapsed > _nextwait)
	{
		lNewGrainCount=1;
		_elapsed =0;

		float r = ( ( rand() / (float)RAND_MAX ) - 0.5 ) / _density;
		_nextwait = 1 / (_density + r);
	}

	if (lNewGrainCount > 0)
	{

		float fScatter = _scatter;
		float fGrainLength = _glength;
		float fAttack = _gattack;

		long lScatterSampleWidth = long(fSampleRate * fScatter) + 1;
		long lGrainLength = long(fSampleRate * fGrainLength);
		long lAttackTime = long(fSampleRate * fAttack);

		for (unsigned long lIndex = 0; lIndex < lNewGrainCount; lIndex++)
		{

			long lOffset = rand() % SampleCount;

			long lGrainReadPointer =
					(poGrainScatter->m_lWritePointer - SampleCount + lOffset
							- (rand() % lScatterSampleWidth));
			while (lGrainReadPointer < 0)
				lGrainReadPointer += poGrainScatter->m_lBufferSize;
			lGrainReadPointer &= (poGrainScatter->m_lBufferSize - 1);

			Grain * poNewGrain = new Grain(lGrainReadPointer, lGrainLength,
					lAttackTime);

			poNewGrain->m_poNextGrain = poGrainScatter->m_poCurrentGrains;
			poGrainScatter->m_poCurrentGrains = poNewGrain;

			poNewGrain->run(SampleCount - lOffset, pfOutput + lOffset,
					poGrainScatter->m_pfBuffer,
					poGrainScatter->m_lBufferSize);
		}
	}
}

/*****************************************************************************/



/*****************************************************************************/

PluginGrain::PluginGrain()
{
	_leftProcesor = new GrainScatter(SAMPLE_RATE);
	_rightProcesor = new GrainScatter(SAMPLE_RATE);

	panic();

	registerPlugin(1, "Grain", "Grain Scatter", 1);
	//{64, 64, 33, 0,  0, 90,      40, 85, 64, 119, 0, 0},
	registerParam(1, "Density", "", "grain/s", "", "", 1, 10, 1, 1);
	registerParam(2, "Scatter", "", "msec", "", "", 0, GRAIN_MAXIMUM_SCATTER * 1000, 1, 100);
	registerParam(3, "Grain Length", "", "msec", "", "", 0, 200, 20, 64);
	registerParam(4, "Grain Attack", "", "msec", "", "", 0, 50, 1, 10);
}


PluginGrain::~PluginGrain()
{
	delete _leftProcesor;
	delete _rightProcesor;
}

int PluginGrain::process(StereoBuffer* input)
{
	float* inLeft = input->left;
	float* inRight = input->right;
	float* outLeft = _outputBuffers[0]->left;
	float* outRight = _outputBuffers[0]->right;

	_leftProcesor->runGrainScatter(inLeft,outLeft,input->length);
	_rightProcesor->runGrainScatter(inRight,outRight,input->length);

	return paContinue;
}

void PluginGrain::panic()
{
	_leftProcesor->activateGrainScatter();
	_rightProcesor->activateGrainScatter();
	Plugin::panic();
}

void PluginGrain::setParam(int npar, int value)
{
	switch (npar)
	{
	case 1:
		_density = (float)value;
		_density = _density < 0 ? 0 : _density > 10 ? 10 : _density;
		_leftProcesor->setParam(npar,_density);
		_rightProcesor->setParam(npar,_density);
		break;
	case 2:
		_scatter = value / 1000.0;
		_scatter = _scatter < 0 ? 0 : _scatter > GRAIN_MAXIMUM_SCATTER * 1000 ? GRAIN_MAXIMUM_SCATTER * 1000 : _scatter;
		_leftProcesor->setParam(npar,_scatter);
		_rightProcesor->setParam(npar,_scatter);
		break;
	case 3:
		_glength = value / 1000.0;
		_glength = _glength < 0 ? 0 : _glength > 200 ? 200 : _glength;
		_leftProcesor->setParam(npar,_glength);
		_rightProcesor->setParam(npar,_glength);
		break;
	case 4:
		_gattack = value / 1000.0;
		_gattack = _gattack < 0 ? 0 : _gattack > 50 ? 50 : _gattack;
		_leftProcesor->setParam(npar,_gattack);
		_rightProcesor->setParam(npar,_gattack);
		break;
	default:
		break;
	};
}
;

int PluginGrain::getParam(int npar)
{

	switch (npar)
	{
	case 1:
		return _density;
		break;
	case 2:
		return 	_scatter * 1000;
		break;
	case 3:
		return _glength * 1000;
		break;
	case 4:
		return _gattack * 1000;
		break;
	default:
		break;
	};

	return (0);
}
;

/*****************************************************************************/
