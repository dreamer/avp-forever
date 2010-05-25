// Copyright (C) 2010 Barry Duncan. All Rights Reserved.
// The original author of this code can be contacted at: bduncan22@hotmail.com

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <assert.h>
#include "os_header.h"
#include "ringbuffer.h"

RingBuffer::RingBuffer(uint32_t size)
{
	buffer = new uint8_t[size];
	bufferCapacity = size;
	readPos = 0;
	writePos = 0;
	amountFilled = 0;
	initialFill = false;

	InitializeCriticalSection(&mCriticalSection);
}

RingBuffer::~RingBuffer()
{
	if (buffer)
	{
		delete[] buffer;
	}

	DeleteCriticalSection(&mCriticalSection);
}

uint32_t RingBuffer::GetWritableSize()
{
	// load ring buffer
	int32_t freeSpace = 0;
//	uint32_t firstSize = 0;
//	uint32_t secondSize = 0;

	EnterCriticalSection(&mCriticalSection);

	// if our read and write positions are at the same location, does that mean the buffer is empty or full?
	if (readPos == writePos)
	{
		// if fill size is greater than 0, we must be full? ..
		if (amountFilled == bufferCapacity)
		{
			// this should really just be zero store size free logically?
			freeSpace = 0;
		}
		else if (amountFilled == 0)
		{
			// buffer fill size must be zero, we can assume there's no data in buffer, ie freespace is entire buffer
			freeSpace = bufferCapacity;
		}
	}
	else
	{
		// standard logic - buffers have a gap, work out writable space between them
		freeSpace = readPos - writePos;
		if (freeSpace < 0)
			freeSpace += bufferCapacity;
	}

	LeaveCriticalSection(&mCriticalSection);

	return freeSpace;
}

uint32_t RingBuffer::GetReadableSize()
{
	// find out how we need to read data. do we split into 2 memcpys or not?
//	uint32_t firstSize = 0;
//	uint32_t secondSize = 0;
	int32_t readableSpace = 0;

	EnterCriticalSection(&mCriticalSection);

	// if our read and write positions are at the same location, does that mean the buffer is empty or full?
	if (readPos == writePos)
	{
		// if fill size is greater than 0, we must be full? ..
		if (amountFilled == bufferCapacity)
		{
			// this should really just be entire buffer logically?
			readableSpace = bufferCapacity;
		}
		else
		{
			// buffer fill size must be zero, we can assume there's no data in buffer
			readableSpace = 0;
		}
	}
	else
	{
		// standard logic - buffers have a gap, work out readable space between them
		readableSpace = writePos - readPos;
		if (readableSpace < 0)
			readableSpace += bufferCapacity;
	}

	LeaveCriticalSection(&mCriticalSection);

	return readableSpace;
}

uint32_t RingBuffer::ReadData(uint8_t *destData, uint32_t amountToRead)
{
	assert (buffer);
	assert (destData != NULL);

	uint32_t firstSize = 0;
	uint32_t secondSize = 0;
	uint32_t totalRead = 0;

	// grab how much readable data there is
	uint32_t readableSpace = GetReadableSize();

	EnterCriticalSection(&mCriticalSection);

	if (amountToRead > readableSpace)
	{
		amountToRead = readableSpace;
	}

	firstSize = bufferCapacity - readPos;

	if (firstSize > amountToRead)
	{
		firstSize = amountToRead;
	}

	// read first bit of data
	memcpy(&destData[0], &buffer[readPos], firstSize);
	totalRead += firstSize;

	secondSize = amountToRead - firstSize;

	if (secondSize > 0)
	{
		memcpy(&destData[firstSize], &buffer[0], secondSize);
		totalRead += secondSize;
	}

	// update our ring buffer read position and track the fact we've taken some data from it
	readPos = (readPos + totalRead) % bufferCapacity;
	amountFilled -= totalRead;

	LeaveCriticalSection(&mCriticalSection);

	return totalRead;
}

uint32_t RingBuffer::WriteData(uint8_t *srcData, uint32_t srcDataSize)
{
	assert (buffer);
	assert (srcData != NULL);

	uint32_t firstSize = 0;
	uint32_t secondSize = 0;
	uint32_t totalWritten = 0;

	// grab how much writable space there is
	uint32_t availableSpace = GetWritableSize();

	EnterCriticalSection(&mCriticalSection);

	if (srcDataSize > availableSpace)
	{
		srcDataSize = availableSpace;
	}

	// space free from write cursor to end of buffer
	firstSize = bufferCapacity - writePos;

	// is first size big enough to hold all our data?
	if (firstSize > srcDataSize)
		firstSize = srcDataSize;

	// first part. from write cursor to end of buffer
	memcpy( &buffer[writePos], &srcData[0], firstSize);
	totalWritten += firstSize;

	secondSize = srcDataSize - firstSize;

	// need to do second copy due to wrap
	if (secondSize > 0)
	{
		// copy second part. start of buffer to play cursor
		memcpy( &buffer[0], &srcData[firstSize], secondSize);
		totalWritten += secondSize;
	}

	writePos = (writePos + firstSize + secondSize) % bufferCapacity;
	amountFilled += firstSize + secondSize;

	LeaveCriticalSection(&mCriticalSection);

	return totalWritten;
}
