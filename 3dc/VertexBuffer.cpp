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

#include "VertexBuffer.h"
#include "console.h"

enum VertexPrimitive
{
	VertexPrimitive_List,
	VertexPrimitive_Strip
};

bool VertexBuffer::Set()
{
	return R_SetVertexBuffer(*this);
}

bool VertexBuffer::Lock(void **data)
{
	return R_LockVertexBuffer(*this, 0, 0, data, this->usage);
}

bool VertexBuffer::Unlock()
{
	return R_UnlockVertexBuffer(*this);
}

bool VertexBuffer::Release()
{
	return R_ReleaseVertexBuffer(*this);
}

bool VertexBuffer::Create(uint32_t capacity, uint32_t stride, enum R_USAGE usage)
{
	// store values for later use
	this->capacity = capacity;
	this->usage = usage;

	this->stride = stride;
	this->sizeInBytes = this->capacity * this->stride;

	return R_CreateVertexBuffer(*this);
}
