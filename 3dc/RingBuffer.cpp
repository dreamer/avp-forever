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
#include <new>
#include "os_header.h"
#include "RingBuffer.h"

bool RingBuffer::Init(uint32_t size)
{
	_buffer = new(std::nothrow) uint8_t[size];
	if (_buffer == NULL) {
		return false;
	}

	_bufferCapacity = size;
	_readPos = 0;
	_writePos = 0;
	_amountFilled = 0;

	if (pthread_mutex_init(&_criticalSection, NULL) != 0) {
		return false;
	}
	_criticalSectionInited = true;
	return true;
}

RingBuffer::~RingBuffer()
{
	delete[] _buffer;
	if (_criticalSectionInited) {
		pthread_mutex_destroy(&_criticalSection);
	}
}

uint32_t RingBuffer::GetWritableSize()
{
	// load ring buffer
	int32_t freeSpace = 0;

	pthread_mutex_lock(&_criticalSection);

	// if our read and write positions are at the same location, does that mean the buffer is empty or full?
	if (_readPos == _writePos)
	{
		// if fill size is greater than 0, we must be full? ..
		if (_amountFilled == _bufferCapacity)
		{
			// this should really just be zero store size free logically?
			freeSpace = 0;
		}
		else if (_amountFilled == 0)
		{
			// buffer fill size must be zero, we can assume there's no data in buffer, ie freespace is entire buffer
			freeSpace = _bufferCapacity;
		}
	}
	else
	{
		// standard logic - buffers have a gap, work out writable space between them
		freeSpace = _readPos - _writePos;
		if (freeSpace < 0) {
			freeSpace += _bufferCapacity;
		}
	}

	pthread_mutex_unlock(&_criticalSection);

	return freeSpace;
}

uint32_t RingBuffer::GetReadableSize()
{
	// find out how we need to read data. do we split into 2 memcpy-s or not?
	int32_t readableSpace = 0;

	pthread_mutex_lock(&_criticalSection);

	// if our read and write positions are at the same location, does that mean the buffer is empty or full?
	if (_readPos == _writePos)
	{
		// if fill size is greater than 0, we must be full? ..
		if (_amountFilled == _bufferCapacity)
		{
			// this should really just be entire buffer logically?
			readableSpace = _bufferCapacity;
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
		readableSpace = _writePos - _readPos;
		if (readableSpace < 0) {
			readableSpace += _bufferCapacity;
		}
	}

	pthread_mutex_unlock(&_criticalSection);

	return readableSpace;
}

uint32_t RingBuffer::ReadData(uint8_t *destData, uint32_t amountToRead)
{
	assert (_buffer);
	assert (destData != NULL);

	uint32_t firstSize = 0;
	uint32_t secondSize = 0;
	uint32_t totalRead = 0;

	// grab how much readable data there is
	uint32_t readableSpace = GetReadableSize();

	pthread_mutex_lock(&_criticalSection);

	if (amountToRead > readableSpace) {
		amountToRead = readableSpace;
	}

	firstSize = _bufferCapacity - _readPos;

	if (firstSize > amountToRead) {
		firstSize = amountToRead;
	}

	// read first bit of data
	memcpy(&destData[0], &_buffer[_readPos], firstSize);
	totalRead += firstSize;

	secondSize = amountToRead - firstSize;

	if (secondSize > 0)
	{
		memcpy(&destData[firstSize], &_buffer[0], secondSize);
		totalRead += secondSize;
	}

	// update our ring buffer read position and track the fact we've taken some data from it
	_readPos = (_readPos + totalRead) % _bufferCapacity;
	_amountFilled -= totalRead;

	pthread_mutex_unlock(&_criticalSection);

	return totalRead;
}

uint32_t RingBuffer::WriteData(uint8_t *srcData, uint32_t srcDataSize)
{
	assert (_buffer);
	assert (srcData != NULL);

	uint32_t firstSize = 0;
	uint32_t secondSize = 0;
	uint32_t totalWritten = 0;

	// grab how much writable space there is
	uint32_t availableSpace = GetWritableSize();

	pthread_mutex_lock(&_criticalSection);

	if (srcDataSize > availableSpace) {
		srcDataSize = availableSpace;
	}

	// space free from write cursor to end of buffer
	firstSize = _bufferCapacity - _writePos;

	// is first size big enough to hold all our data?
	if (firstSize > srcDataSize) {
		firstSize = srcDataSize;
	}

	// first part. from write cursor to end of buffer
	memcpy(&_buffer[_writePos], &srcData[0], firstSize);
	totalWritten += firstSize;

	secondSize = srcDataSize - firstSize;

	// need to do second copy due to wrap
	if (secondSize > 0)
	{
		// copy second part. start of buffer to play cursor
		memcpy(&_buffer[0], &srcData[firstSize], secondSize);
		totalWritten += secondSize;
	}

	_writePos = (_writePos + firstSize + secondSize) % _bufferCapacity;
	_amountFilled += firstSize + secondSize;

	pthread_mutex_unlock(&_criticalSection);

	return totalWritten;
}
