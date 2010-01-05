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

bool RingBuffer_Init(int size);
void RingBuffer_Unload();
int RingBuffer_GetWritableSpace();
int RingBuffer_GetReadableSpace();
int RingBuffer_ReadData(uint8_t *destData, int amountToRead);
int RingBuffer_WriteData(uint8_t *srcData, int srcDataSize);