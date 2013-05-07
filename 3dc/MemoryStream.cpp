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

#include "MemoryStream.h"
#include <stdlib.h>
#include <cstring>
#include <algorithm>

MemoryReadStream::MemoryReadStream(uint8_t *buffer, size_t size)
{
	_size  = size;
	_start = buffer;
	_end   = buffer + size;
	_currentOffset = _start;
}

MemoryReadStream::~MemoryReadStream()
{
}

void MemoryReadStream::SkipBytes(size_t nBytes)
{
	_currentOffset += nBytes;
}

size_t MemoryReadStream::GetBytesRead()
{
	return _currentOffset - _start;
}

uint8_t MemoryReadStream::GetByte()
{
	uint8_t ret;
	memcpy(&ret, _currentOffset, sizeof(ret));
	_currentOffset += sizeof(ret);
	return ret;
}

uint16_t MemoryReadStream::GetUint16LE()
{
	uint16_t ret;
	memcpy(&ret, _currentOffset, sizeof(ret));
	_currentOffset += sizeof(ret);
	return ret;
}

uint16_t MemoryReadStream::GetUint16BE()
{
	uint16_t ret;
	memcpy(&ret, _currentOffset, sizeof(ret));
	_currentOffset += sizeof(ret);
	return _byteswap_ushort(ret);
}

uint32_t MemoryReadStream::GetUint32LE()
{
	uint32_t ret;
	memcpy(&ret, _currentOffset, sizeof(ret));
	_currentOffset += sizeof(ret);
	return ret;
}

uint32_t MemoryReadStream::GetUint32BE()
{
	uint32_t ret;
	memcpy(&ret, _currentOffset, sizeof(ret));
	_currentOffset += sizeof(ret);
	return _byteswap_ulong(ret);
}

uint64_t MemoryReadStream::GetUint64LE()
{
	uint64_t ret;
	memcpy(&ret, _currentOffset, sizeof(ret));
	_currentOffset += sizeof(ret);
	return ret;
}

uint64_t MemoryReadStream::GetUint64BE()
{
	uint64_t ret;
	memcpy(&ret, _currentOffset, sizeof(ret));
	_currentOffset += sizeof(ret);
	return _byteswap_uint64(ret);
}

uint8_t MemoryReadStream::PeekByte()
{
	return *_currentOffset;
}

size_t MemoryReadStream::GetBytes(uint8_t *buffer, size_t nBytes)
{
	if (!buffer) {
		return 0;
	}

	size_t count = std::min((size_t)(_end - _currentOffset), nBytes);

	memcpy(_currentOffset, buffer, count);
	_currentOffset += count;
	return count;
}

// Write stream implementation
MemoryWriteStream::MemoryWriteStream(uint8_t *buffer, size_t size)
{
	_size  = size;
	_start = buffer;
	_end   = buffer + size;
	_currentOffset = _start;
}

MemoryWriteStream::~MemoryWriteStream()
{
}

void MemoryWriteStream::PutByte(uint8_t v)
{
	memcpy(_currentOffset, &v, sizeof(v));
	_currentOffset += sizeof(v);
}

void MemoryWriteStream::PutUint16LE(uint16_t v)
{
	memcpy(_currentOffset, &v, sizeof(v));
	_currentOffset += sizeof(v);
}

void MemoryWriteStream::PutUint16BE(uint16_t v)
{
	uint16_t _v = _byteswap_ushort(v);
	memcpy(_currentOffset, &_v, sizeof(_v));
	_currentOffset += sizeof(_v);
}

void MemoryWriteStream::PutUint32LE(uint32_t v)
{
	memcpy(_currentOffset, &v, sizeof(v));
	_currentOffset += sizeof(v);
}

void MemoryWriteStream::PutUint32BE(uint32_t v)
{
	uint32_t _v = _byteswap_ulong(v);
	memcpy(_currentOffset, &_v, sizeof(_v));
	_currentOffset += sizeof(_v);
}

void MemoryWriteStream::PutUint64LE(uint64_t v)
{
	memcpy(_currentOffset, &v, sizeof(v));
	_currentOffset += sizeof(v);
}

void MemoryWriteStream::PutUint64BE(uint64_t v)
{
	uint64_t _v = _byteswap_uint64(v);
	memcpy(_currentOffset, &_v, sizeof(_v));
	_currentOffset += sizeof(_v);
}

size_t MemoryWriteStream::GetBytesWritten()
{
	return _currentOffset - _start;
}

size_t MemoryWriteStream::PutBytes(uint8_t *buffer, size_t nBytes)
{
	if (!buffer) {
		return 0;
	}

	size_t count = std::min((size_t)(_end - _currentOffset), nBytes);

	memcpy(_currentOffset, buffer, count);
	_currentOffset += count;
	return count;
}
