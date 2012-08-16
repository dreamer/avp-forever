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

#ifndef _indexBuffer_h_
#define _indexBuffer_h_

#include <stdint.h>
#include "renderer.h"

class IndexBuffer
{
	public:
		// members
		r_IndexBuffer	indexBuffer;
		uint32_t		capacity; // number of indices we can hold
		uint32_t		sizeInBytes;
		enum R_USAGE	usage;
		bool			isLocked;

		// constructor
		IndexBuffer():
			capacity(0),
			sizeInBytes(0),
			usage(USAGE_DYNAMIC),
			isLocked(false)
		{
		}

		// deconstructor
		~IndexBuffer()
		{
			this->Release();
		}

		// public functions
		bool IndexBuffer::Create(uint32_t capacity, enum R_USAGE usage);
		bool IndexBuffer::Release();
		bool IndexBuffer::Lock(uint16_t **data);
		bool IndexBuffer::Unlock();
		bool IndexBuffer::Set();
		uint32_t IndexBuffer::GetCapacity() const { return capacity; }
		uint32_t IndexBuffer::GetSize() const { return nIndices; }
		void IndexBuffer::SetSize(uint32_t size);

	private:
		size_t nIndices; // number of indices in the buffer
};

#endif
