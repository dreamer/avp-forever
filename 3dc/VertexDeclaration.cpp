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

#include "VertexDeclaration.h"
#include <assert.h>

VertexDeclaration::VertexDeclaration()
{
	declaration = 0;
	offset = 0;
}

VertexDeclaration::~VertexDeclaration()
{
	Release();
}

bool VertexDeclaration::Create()
{
	if (elements.size() == 0)
	{
		// log error
		return false;
	}

	if (R_CreateVertexDeclaration(this))
	{
		// we don't need this anymore?
		this->elements.clear();
		return true;
	}
	
	return false;
}

bool VertexDeclaration::Release()
{
	return R_ReleaseVertexDeclaration(*this);
}

bool VertexDeclaration::Set()
{
	return R_SetVertexDeclaration(*this);
}

void VertexDeclaration::Add(uint16_t stream, VD_TYPE type, VD_METHOD method, VD_USAGE usage, uint8_t usageIndex)
{
	uint16_t elementSize = 0;

	// calculate element size
	switch (type)
	{
		case VDTYPE_FLOAT1:
		case VDTYPE_COLOR:
			elementSize = 4; // sizeof(float);
			break;
		case VDTYPE_FLOAT2:
			elementSize = 8; // sizeof(float) * 2;
			break;
		case VDTYPE_FLOAT3:
			elementSize = 12; // sizeof(float) * 3;
			break;
		case VDTYPE_FLOAT4:
			elementSize = 16; // sizeof(float) * 4;
			break;
		default:
			assert (1==0);
			break;
	}

	vertexElement newElement;
	newElement.stream = stream;
	newElement.offset = this->offset;
	newElement.usage  = usage;
	newElement.method = method;
	newElement.type = type;
	newElement.usageIndex = usageIndex;

	// increment our declaration element size offset
	this->offset += elementSize; 

	this->elements.push_back(newElement);
}
