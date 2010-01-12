#pragma once

#include "stdint.h"

class RingBuffer
{
	private:
		int		readPos;
		int		writePos;
		int		amountFilled;
		bool	initialFill;
		uint8_t	*buffer;
		int		bufferCapacity;
	public:
		RingBuffer(int size);
		~RingBuffer();
		int RingBuffer::GetWritableSize();
		int RingBuffer::GetReadableSize();
		int RingBuffer::ReadData(uint8_t *destData, int amountToRead);
		int RingBuffer::WriteData(uint8_t *srcData, int srcDataSize);
};