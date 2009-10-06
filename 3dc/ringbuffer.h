#pragma once

/* ring buffer stuff */
struct ringBuffer 
{
	int		readPos;
	int		writePos;
	int		amountFilled;
	bool	initialFill;
	char	*buffer;
	int		bufferCapacity;
};

bool RingBuffer_Init(int size);
void RingBuffer_Unload();
int RingBuffer_GetWritableSpace();
int RingBuffer_GetReadableSpace();
int RingBuffer_ReadData(char *destData, int amountToRead);
int RingBuffer_WriteData(char *srcData, int srcDataSize);