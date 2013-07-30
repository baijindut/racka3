/*
 * StereoBuffer.cpp
 *
 *  Created on: Jul 30, 2013
 *      Author: slippy
 */

#include "StereoBuffer.h"
#include <stdlib.h>
#include <string.h>

StereoBuffer::StereoBuffer()
{
	length = 0;
	owned = false;
	left = 0;
	right =0;
}

StereoBuffer::StereoBuffer(int length) {
	initAlloc(length);
}

StereoBuffer::StereoBuffer(float* left, float* right,int length) {
	initWrapper(left,right,length);
}

void StereoBuffer::initAlloc(int length)
{
	this->length = length;
	left = (float*)malloc(length*sizeof(float));
	right = (float*)malloc(length*sizeof(float));
	silence();
	owned = true;
}

void StereoBuffer::silence()
{
	if (length)
	{
		memset(left,0,length*sizeof(float));
		memset(right,0,length*sizeof(float));
	}
}

void StereoBuffer::initWrapper(StereoBuffer* buffer)
{
	initWrapper(buffer->left,buffer->right,buffer->length);
}

void StereoBuffer::initWrapper(float* left,float* right,int length)
{
	this->length = length;
	this->left = left;
	this->right = right;
	owned = false;
}

StereoBuffer::~StereoBuffer()
{
	if (owned)
	{
		free(left);
		free(right);
	}
}

void StereoBuffer::ensureLength(int length)
{
	if (owned && length > this->length)
	{
		left = (float*)realloc(left,length*sizeof(float));
		right = (float*)realloc(left,length*sizeof(float));
		this->length = length;
	}
}
