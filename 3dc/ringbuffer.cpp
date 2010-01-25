#include <assert.h>
#include "os_header.h"
#include "ringbuffer.h"

RingBuffer::RingBuffer(int size)
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

int RingBuffer::GetWritableSize()
{
	// load ring buffer
	int freeSpace = 0;
	int firstSize = 0;
	int secondSize = 0;

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

int RingBuffer::GetReadableSize()
{
	// find out how we need to read data. do we split into 2 memcpys or not?
	int firstSize = 0;
	int secondSize = 0;
	int readableSpace = 0;

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

int RingBuffer::ReadData(uint8_t *destData, int amountToRead)
{
	assert (buffer);
	assert (destData != NULL);

	int firstSize = 0;
	int secondSize = 0;
	int totalRead = 0;

	// grab how much readable data there is
	int readableSpace = GetReadableSize();

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

int RingBuffer::WriteData(uint8_t *srcData, int srcDataSize)
{
	assert (buffer);
	assert (srcData != NULL);

	int firstSize = 0;
	int secondSize = 0;
	int totalWritten = 0;

	// grab how much writable space there is
	int availableSpace = GetWritableSize();

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