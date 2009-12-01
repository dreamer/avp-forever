#include <assert.h>
#ifdef WIN32
#include <windows.h>
#endif
#ifdef _XBOX
#include <xtl.h>
#endif

#include "ringbuffer.h"

static ringBuffer ring = {0};

bool RingBuffer_Init(int size)
{
	/* initalise data buffer in ring buffer */
	ring.buffer = new byte[size];

	if (ring.buffer == NULL)
		return false;

	ring.bufferCapacity = size;
	ring.readPos = 0;
	ring.writePos = 0;
	ring.amountFilled = 0;
	ring.initialFill = false;

	return true;
}

void RingBuffer_Unload()
{
	if (ring.buffer)
	{
		delete []ring.buffer;
		ring.buffer = NULL;
	}

	ring.bufferCapacity = 0;
	ring.readPos = 0;
	ring.writePos = 0;
	ring.amountFilled = 0;
	ring.initialFill = false;
}

int RingBuffer_GetWritableSpace()
{
	/* load ring buffer */
	int freeSpace = 0;
	int firstSize = 0;
	int secondSize = 0;

	/* if our read and write positions are at the same location, does that mean the buffer is empty or full? */
	if (ring.readPos == ring.writePos)
	{
		/* if fill size is greater than 0, we must be full? .. */
		if (ring.amountFilled == ring.bufferCapacity)
		{
			/* this should really just be zero store size free logically? */
			freeSpace = 0;
		}
		else if (ring.amountFilled == 0)
		{
			/* buffer fill size must be zero, we can assume there's no data in buffer, ie freespace is entire buffer */
			freeSpace = ring.bufferCapacity;
		}
	}
	else 
	{
		/* standard logic - buffers have a gap, work out writable space between them */
		freeSpace = ring.readPos - ring.writePos;
		if (freeSpace < 0) 
			freeSpace += ring.bufferCapacity;
	}

	return freeSpace;
}

int RingBuffer_GetReadableSpace()
{
	/* find out how we need to read data. do we split into 2 memcpys or not? */
	int firstSize = 0;
	int secondSize = 0;
	int readableSpace = 0;

	/* if our read and write positions are at the same location, does that mean the buffer is empty or full? */
	if (ring.readPos == ring.writePos)
	{
		/* if fill size is greater than 0, we must be full? .. */
		if (ring.amountFilled == ring.bufferCapacity)
		{
			/* this should really just be entire buffer logically? */
			readableSpace = ring.bufferCapacity;
		}
		else
		{
			/* buffer fill size must be zero, we can assume there's no data in buffer */
			readableSpace = 0;
		}
	}
	else 
	{
		/* standard logic - buffers have a gap, work out readable space between them */
		readableSpace = ring.writePos - ring.readPos;
		if (readableSpace < 0) readableSpace += ring.bufferCapacity;
	}

	return readableSpace;
}

int RingBuffer_ReadData(byte *destData, int amountToRead)
{
	assert (ring.buffer);
	assert (destData != NULL);

	int firstSize = 0;
	int secondSize = 0;
	int totalRead = 0;

	/* grab how much readable data there is */
	int readableSpace = RingBuffer_GetReadableSpace();

	if (amountToRead > readableSpace)
	{
		amountToRead = readableSpace;
	}

	firstSize = ring.bufferCapacity - ring.readPos;

	if (firstSize > amountToRead)
	{
		firstSize = amountToRead;
	}

	/* read first bit of data */
	memcpy(&destData[0], &ring.buffer[ring.readPos], firstSize);
	totalRead += firstSize;

	secondSize = amountToRead - firstSize;

	if (secondSize > 0)
	{
		memcpy(&destData[firstSize], &ring.buffer[0], secondSize);
		totalRead += secondSize;
	}

	/* update our ring buffer read position and track the fact we've taken some data from it */
	ring.readPos = (ring.readPos + totalRead) % ring.bufferCapacity;
	ring.amountFilled -= totalRead;

	return totalRead;
}

int RingBuffer_WriteData(byte *srcData, int srcDataSize)
{
	assert (ring.buffer);
	assert (srcData != NULL);

	int firstSize = 0;
	int secondSize = 0;
	int totalWritten = 0;

	/* grab how much writable space there is */
	int availableSpace = RingBuffer_GetWritableSpace();

	if (srcDataSize > availableSpace)
	{
		srcDataSize = availableSpace;
	}

	/* space free from write cursor to end of buffer */
	firstSize = ring.bufferCapacity - ring.writePos;

	/* is first size big enough to hold all our data? */
	if (firstSize > srcDataSize) 
		firstSize = srcDataSize;

	/* first part. from write cursor to end of buffer */
	memcpy( &ring.buffer[ring.writePos], &srcData[0], firstSize);
	totalWritten += firstSize;

	secondSize = srcDataSize - firstSize;

	/* need to do second copy due to wrap */
	if (secondSize > 0)
	{
//			printf("wrapped. firstSize: %d secondSize: %d totalWrite: %d dataSize: %d\n", firstSize, secondSize, (firstSize + secondSize), dataSize);
//			printf("write pos: %d read pos: %d free space: %d\n", writePos, readPos, freeSpace);
		/* copy second part. start of buffer to play cursor */
		memcpy( &ring.buffer[0], &srcData[firstSize], secondSize);
		totalWritten += secondSize;
	}

	ring.writePos = (ring.writePos + firstSize + secondSize) % ring.bufferCapacity;
	ring.amountFilled += firstSize + secondSize;

	if (ring.amountFilled > ring.bufferCapacity)
	{
		//std::cout << "fillSize greater than store size!\n" << std::endl;
	}

	return totalWritten;
}