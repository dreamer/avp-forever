#pragma once

#include "stdint.h"

struct ringBuffer 
{
	int		readPos;
	int		writePos;
	int		amountFilled;
	bool	initialFill;
	uint8_t	*buffer;
	int		bufferCapacity;
};

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
/*
bool RingBuffer_Init(int size);
void RingBuffer_Unload();
int RingBuffer_GetWritableSpace();
int RingBuffer_GetReadableSpace();
int RingBuffer_ReadData(uint8_t *destData, int amountToRead);
int RingBuffer_WriteData(uint8_t *srcData, int srcDataSize);
*/