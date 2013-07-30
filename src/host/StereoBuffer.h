/*
 * StereoBuffer.h
 *
 *  Created on: Jul 30, 2013
 *      Author: slippy
 */

#ifndef STEREOBUFFER_H_
#define STEREOBUFFER_H_

class StereoBuffer {
public:
	StereoBuffer();
	StereoBuffer(int length);
	StereoBuffer(float* left,float* right,int length);
	virtual ~StereoBuffer();

	void silence();
	void initWrapper(StereoBuffer* buffer);
	void initWrapper(float* left,float* right,int length);
	void initAlloc(int length);

	float* left;
	float* right;
	bool owned;
	int length;

	void ensureLength(int length);


};

#endif /* STEREOBUFFER_H_ */
