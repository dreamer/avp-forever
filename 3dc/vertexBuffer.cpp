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
#include "logString.h"

bool VertexBuffer::Draw()
{
	LastError = this->d3dDevice->SetStreamSource(0, this->vertexBuffer, 0, this->vbFVFsize);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	LastError = this->d3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool VertexBuffer::Lock(void **data)
{
	uint32_t lockType = 0;

	if (this->vbUsage == USAGE_DYNAMIC)
		lockType = D3DLOCK_DISCARD;

	LastError = vertexBuffer->Lock(0, 0, data, this->vbUsage);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
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
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool VertexBuffer::Create(uint32_t size, enum FVF fvf, enum USAGE usage)
{
	this->vbFVF = fvf;

	switch (this->vbFVF)
	{
		case FVF_LVERTEX:
			this->vbFVFsize = sizeof(D3DLVERTEX);
			break;
		case FVF_ORTHO:
			this->vbFVFsize = sizeof(ORTHOVERTEX);
			break;
		case FVF_FMV:
			this->vbFVFsize = sizeof(FMVVERTEX);
			break;
		default:
			// error and return
			break;
	}

	this->vbLength = size * this->vbFVFsize;

	switch (usage)
	{
		case USAGE_STATIC:
			this->vbUsage = 0;
			this->vbPool = D3DPOOL_MANAGED;
			break;
		case USAGE_DYNAMIC:
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
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}