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

#pragma once

#include <stdint.h>
#include <pthread.h>

class RingBuffer
{
	private:
		uint32_t	_readPos;
		uint32_t	_writePos;
		uint32_t	_amountFilled;
		uint8_t	   *_buffer;
		uint32_t	_bufferCapacity;
		pthread_mutex_t _criticalSection;
		bool _criticalSectionInited;

	public:
		RingBuffer() :
			_readPos(0),
			_writePos(0),
			_amountFilled(0),
			_buffer(NULL),
			_bufferCapacity(0),
			_criticalSectionInited(false)
			{}
		bool Init(uint32_t size);
		~RingBuffer();
		uint32_t GetWritableSize();
		uint32_t GetReadableSize();
		uint32_t ReadData(uint8_t *destData, uint32_t amountToRead);
		uint32_t WriteData(uint8_t *srcData, uint32_t srcDataSize);
};
