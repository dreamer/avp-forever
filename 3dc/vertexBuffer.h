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

#include "renderer.h"

class VertexBuffer
{
	public:
		enum USAGE
		{
			USAGE_STATIC,
			USAGE_DYNAMIC
		};
		enum FVF
		{
			FVF_LVERTEX,
			FVF_ORTHO,
			FVF_FMV
		};

		// constructor
		VertexBuffer(IDirect3DDevice9 *d3dDevice):
			vertexBuffer(0),
			indexBuffer(0),
			vbLength(0),
			vbUsage(0),
			vbPool(D3DPOOL_DEFAULT),
			vbFVF(FVF_ORTHO),
			vbFVFsize(0),
			vbIsLocked(false),
			ibIsLocked(false)
		{
			this->d3dDevice = d3dDevice;
			d3dDevice->AddRef();
			// addRef?
		}

		// deconstructor
		~VertexBuffer()
		{
			SAFE_RELEASE(vertexBuffer);
			SAFE_RELEASE(indexBuffer);
		}

	bool VertexBuffer::Create(uint32_t size, enum FVF fvf, enum USAGE usage);
	bool VertexBuffer::Lock(void **data);
	bool VertexBuffer::Unlock();
	bool VertexBuffer::Draw();

	private:
		IDirect3DDevice9		*d3dDevice;
		IDirect3DVertexBuffer9	*vertexBuffer;
		IDirect3DIndexBuffer9	*indexBuffer;

		uint32_t	vbSize;
		uint32_t	ibSize;

		uint32_t	vbLength;
		uint32_t	vbUsage;
		D3DPOOL		vbPool;
		FVF			vbFVF;
		uint32_t	vbFVFsize;

		bool	vbIsLocked;
		bool	ibIsLocked;

		HRESULT	LastError;
};