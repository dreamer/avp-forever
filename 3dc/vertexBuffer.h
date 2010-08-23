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
// THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _vertexBuffer_h_
#define _vertexBuffer_h_

#include <stdint.h>
#include "renderer.h"

class VertexBuffer
{
	public:
		enum FVF
		{
			FVF_LVERTEX,
			FVF_ORTHO,
			FVF_FMV
		};

		// constructor
		VertexBuffer():
			vertexBuffer(0),
			vbMaxVerts(0),
			ibSize(0),
			vbSizeInBytes(0),
			vbUsage(USAGE_DYNAMIC),
			vbFVF(FVF_ORTHO),
			vbFVFsize(0),
			vbIsLocked(false),
			ibIsLocked(false)
		{
		}

		// deconstructor
		~VertexBuffer()
		{
			R_ReleaseVertexBuffer(vertexBuffer);
		}

		// public functions
		bool VertexBuffer::Create(uint32_t size, enum FVF fvf, enum R_USAGE usage);
		bool VertexBuffer::Release();
		bool VertexBuffer::Lock(void **data);
		bool VertexBuffer::Unlock();
		bool VertexBuffer::Draw();
		uint32_t VertexBuffer::GetCapacity();

	private:
		r_VertexBuffer	*vertexBuffer;
		uint32_t		vbMaxVerts;
		uint32_t		ibSize;
		uint32_t		vbSizeInBytes;
		enum R_USAGE	vbUsage;
		FVF				vbFVF;
		uint32_t		vbFVFsize;

		bool	vbIsLocked;
		bool	ibIsLocked;
};

#endif
