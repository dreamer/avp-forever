// Copyright (C) 2011 Barry Duncan. All Rights Reserved.
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

#ifndef _MemoryStream_h_
#define _MemoryStream_h_

#include <stdint.h>

class MemoryReadStream
{
	public:
		MemoryReadStream(uint8_t *buffer, size_t size);
		~MemoryReadStream();

		uint8_t  GetByte();
		uint16_t GetUint16LE();
		uint16_t GetUint16BE();
		uint32_t GetUint32LE();
		uint32_t GetUint32BE();
		uint64_t GetUint64LE();
		uint64_t GetUint64BE();
		uint8_t  PeekByte();
		size_t   GetBytesRead();
		size_t   GetBytes(uint8_t *buffer, size_t nBytes);
		void     SkipBytes(size_t nBytes);

	private:
		size_t   _size;
		uint8_t *_start;
		uint8_t *_currentOffset;
		uint8_t *_end;
};

class MemoryWriteStream
{
	public:
		MemoryWriteStream(uint8_t *buffer, size_t size);
		~MemoryWriteStream();

		void  PutByte(uint8_t v);
		void PutUint16LE(uint16_t v);
		void PutUint16BE(uint16_t v);
		void PutUint32LE(uint32_t v);
		void PutUint32BE(uint32_t v);
		void PutUint64LE(uint64_t v);
		void PutUint64BE(uint64_t v);
		size_t GetBytesWritten();
		size_t PutBytes(uint8_t *buffer, size_t nBytes);

	private:
		size_t   _size;
		uint8_t *_start;
		uint8_t *_currentOffset;
		uint8_t *_end;
};

#endif
