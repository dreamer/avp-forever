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

#include "vertexBuffer.h"

bool VertexBuffer::Lock(void **data)
{
	uint32_t lockType = 0;

	if (this->vbUsage == VB_DYNAMIC)
		lockType = D3DLOCK_DISCARD;

	LastError = vertexBuffer->Lock(0, 0, /*(void**)&data*/data, this->vbUsage);
	if (FAILED(LastError))
	{
//		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}
/*
	LastError = d3d.lpD3DIndexBuffer->Lock(0, 0, (void**)&mainIndex, D3DLOCK_DISCARD);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}
*/
	return true;
}

bool VertexBuffer::Unlock()
{
	LastError = vertexBuffer->Unlock();
	if (FAILED(LastError))
	{
//		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool VertexBuffer::Create(uint32_t size, enum VB_FVF fvf, enum USAGE usage)
{
	switch (fvf)
	{
		case VB_FVF_LVERTEX:
			this->vbLength = size * sizeof(D3DLVERTEX);
			break;
		case VB_FVF_ORTHO:
			this->vbLength = size * sizeof(ORTHOVERTEX);
			break;
		case VB_FVF_FMV:
			this->vbLength = size * sizeof(FMVVERTEX);
			break;
		default:
			// error and return
			break;
	}

	switch (usage)
	{
		case VB_STATIC:
			this->vbUsage = 0;
			this->vbPool = D3DPOOL_MANAGED;
			break;
		case VB_DYNAMIC:
			this->vbUsage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
			this->vbPool = D3DPOOL_DEFAULT;
			break;
		default:
			// error and return
			break;
	}

	LastError = d3dDevice->CreateVertexBuffer(this->vbLength, this->vbUsage, 0, this->vbPool, &vertexBuffer, NULL);
	if (FAILED(LastError))
	{
//		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}