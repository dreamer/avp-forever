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

#include <stdint.h>
#include <vector>
#include "renderStates.h"

// part of sort test
enum Shaders
{
	SHADER_SKY,
	SHADER_DECAL,
	SHADER_WATER,
	SHADER_FMV,
	SHADER_FIRE
};

#define SORT_TEXTURE_SHIFT	16
#define SORT_SHADER_SHIFT	32

struct RenderItem2
{
	uint32_t	vertStart;
	uint32_t	vertEnd;

	uint32_t	indexStart;
	uint32_t	indexEnd;

//	uint32_t	textureID;
//	uint32_t	shaderID;

	uint32_t	sortKey;

	bool operator<(const RenderItem2& rhs) const {return sortKey < rhs.sortKey;}
};

class RenderList
{
	private:
		size_t		capacity;
		size_t		listIndex;	// used as list size
		uint32_t	totalVerts;

	public:
		std::vector<RenderItem2> Items;
		RenderList(size_t size);
		~RenderList();

	void Reset();
	void RenderList::Init(size_t size);
	size_t RenderList::GetCapacity();
	void RenderList::Sort();
//	void AddItem(uint32_t numVerts, uint32_t textureID, uint32_t shaderID);
	void RenderList::AddItem(uint32_t numVerts, uint32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum FILTERING_MODE_ID filteringMode);
};