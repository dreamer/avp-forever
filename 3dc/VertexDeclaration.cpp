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
#include <stdint.h>
#include <vector>

enum VD_TYPE
{
	VDTYPE_FLOAT1,
	VDTYPE_FLOAT2,
	VDTYPE_FLOAT3,
	VDTYPE_FLOAT4,

	VDTYPE_COLOR,

	VDTYPE_UBYTE4,
	VDTYPE_SHORT2,
	VDTYPE_SHORT4,

	VDTYPE_UBYTE4N,
	VDTYPE_SHORT2N,
	VDTYPE_SHORT4N,
	VDTYPE_USHORT2N,
	VDTYPE_USHORT4N,
	VDTYPE_UDEC3,
	VDTYPE_DEC3N,
	VDTYPE_FLOAT16_2,
	VDTYPE_FLOAT16_4,
	VDTYPE_UNUSED
};

enum VD_METHOD
{
	VDMETHOD_DEFAULT
};

enum VD_USAGE
{
	VDUSAGE_POSITION,
	VDUSAGE_BLENDWEIGHT,
	VDUSAGE_BLENDINDICES,
	VDUSAGE_NORMAL,
	VDUSAGE_PSIZE,
	VDUSAGE_TEXCOORD,
	VDUSAGE_TANGENT,
	VDUSAGE_BINORMAL,
	VDUSAGE_TESSFACTOR,
	VDUSAGE_POSITIONT,
	VDUSAGE_COLOR,
	VDUSAGE_FOG,
	VDUSAGE_DEPTH,
	VDUSAGE_SAMPLE
};

struct vertexElement
{
	uint16_t	stream;     // Stream index
	uint16_t	offset;     // Offset in the stream in bytes
	VD_USAGE	usage;
	VD_METHOD	method;
	VD_TYPE		type;
	uint8_t		usageIndex; // Semantic index
};

class VertexDeclaration
{
	private:
		std::vector<vertexElement> elements;
		r_vertexDeclaration	declaration;

	public:
		void Add(VD_TYPE type, VD_METHOD method, VD_USAGE usage);
		bool Create();
		void Set();
		void Delete();
};

bool VertexDeclaration::Create()
{
	if (elements.size() == 0)
	{
		// log error
		return false;
	}

	return true;
}

void VertexDeclaration::Add(VD_TYPE type, VD_METHOD method, VD_USAGE usage)
{
	vertexElement newElement;

	newElement.stream = 0;
	newElement.offset = 0; // TODO
	newElement.usage = usage;
	newElement.method = method;
	newElement.type = type;
	newElement.usageIndex = 0;

	this->elements.push_back(newElement);
}
