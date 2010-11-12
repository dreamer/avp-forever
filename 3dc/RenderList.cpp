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

#include "RenderList.h"
#include "renderer.h"
#include <algorithm>
#include <assert.h>
#include "console.h"

extern uint32_t GetRealNumVerts(uint32_t numVerts);

RenderList::RenderList(size_t size)
{
	capacity = size;
	listIndex = 0;
	vertexCount = 0;
	indexCount = 0;

	// treat the vector as an array so resize it to desired size
	Items.reserve(size);
	Items.resize(size);
}

RenderList::~RenderList()
{
	// nothing to do here
}

void RenderList::AddTriangle(uint16_t *indexArray, uint32_t a, uint32_t b, uint32_t c, uint32_t n)
{
	AddIndices(indexArray, a,b,c, n);
}

// we use this function to create the indices needed to render our screen space quads. 
// We generate the indices differently here than the engine itself does for quads 
//(which it uses for general level geometry.
void RenderList::CreateOrthoIndices(uint16_t *indexArray)
{
	AddIndices(indexArray, 0,1,2, 4);
	AddIndices(indexArray, 2,1,3, 4);
}

// generates triangle indices for the verts we've just added to a vertex buffer
// this function was originally in d3d_render.cpp
void RenderList::CreateIndices(uint16_t *indexArray, uint32_t numVerts)
{
	switch (numVerts)
	{
		default:
			Con_PrintError("Asked to render unexpected number of verts in CreateIndices");
			break;
		case 0:
			Con_PrintError("Asked to render 0 verts in CreateIndices");
			break;
		case 3:
		{
			AddIndices(indexArray, 0,2,1, 3);
			break;
		}
		case 4:
		{
			// winding order for main avp rendering system
			AddIndices(indexArray, 0,1,2, 4);
			AddIndices(indexArray, 0,2,3, 4);
			break;
		}
		case 5:
		{
			AddIndices(indexArray, 0,1,4, 5);
			AddIndices(indexArray, 1,3,4, 5);
			AddIndices(indexArray, 1,2,3, 5);
			break;
		}
		case 6:
		{
			AddIndices(indexArray, 0,4,5, 6);
			AddIndices(indexArray, 0,3,4, 6);
			AddIndices(indexArray, 0,2,3, 6);
			AddIndices(indexArray, 0,1,2, 6);
			break;
		}
		case 7:
		{
			AddIndices(indexArray, 0,5,6, 7);
			AddIndices(indexArray, 0,4,5, 7);
			AddIndices(indexArray, 0,3,4, 7);
			AddIndices(indexArray, 0,2,3, 7);
			AddIndices(indexArray, 0,1,2, 7);
			break;
		}
		case 8:
		{
			AddIndices(indexArray, 0,6,7, 8);
			AddIndices(indexArray, 0,5,6, 8);
			AddIndices(indexArray, 0,4,5, 8);
			AddIndices(indexArray, 0,3,4, 8);
			AddIndices(indexArray, 0,2,3, 8);
			AddIndices(indexArray, 0,1,2, 8);
			break;
		}
	}
}

// adds a single triangle's worth of indices to an index buffer in the correct order
// where a, b and c are the index order, and n is the number of verts we've just added to the VB
void RenderList::AddIndices(uint16_t *indexArray, uint32_t a, uint32_t b, uint32_t c, uint32_t n)
{
	indexArray[this->indexCount]   = (vertexCount - (n) + (a));
	indexArray[this->indexCount+1] = (vertexCount - (n) + (b));
	indexArray[this->indexCount+2] = (vertexCount - (n) + (c));
	this->indexCount+=3;
}

void RenderList::AddItem(uint32_t numVerts, uint32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum FILTERING_MODE_ID filteringMode, enum TEXTURE_ADDRESS_MODE textureAddress, enum ZWRITE_ENABLE zWriteEnable)
{
	assert(numVerts != 0);

	if (listIndex >= GetCapacity())
	{
		// list is full, flush it
		OutputDebugString("List full. Flushing...\n");
		Draw();
	}

	uint32_t realNumVerts = GetRealNumVerts(numVerts);

	Items[listIndex].sortKey = 0; // zero it out
	Items[listIndex].sortKey = (textureID << 24) | (translucencyMode << 20) | (filteringMode << 16) | (textureAddress << 15) | (zWriteEnable << 14);

	// lets see if we can merge this item with the previous item
	if ((listIndex != 0) &&		// only do this check if we're not adding the first item
		(Items[listIndex-1].sortKey == Items[listIndex].sortKey))
	{
		// our new item uses the same states as previous item.
		listIndex--;
	}
	else
	{
		// we need to add a new item
		Items[listIndex].vertStart = vertexCount;
		Items[listIndex].indexStart = indexCount;
	}

	Items[listIndex].vertEnd = vertexCount + numVerts;
	Items[listIndex].indexEnd = indexCount + realNumVerts;
	listIndex++;

	vertexCount += numVerts;
}

void RenderList::Sort()
{
	// sort the list on the sortKey int value. We use the vector
	// as an array so we can't do Items.end() as we don't use push_back
	// or pop_back. Our size is controlled by the value of listIndex
	std::sort(Items.begin(), Items.begin() + listIndex);
}

void RenderList::Reset()
{
	// reset our index back to the first item
	listIndex = 0;

	vertexCount = 0;
	indexCount = 0;
}

void RenderList::Draw()
{
	for (std::vector<RenderItem>::iterator it = Items.begin(); it != Items.begin() + listIndex; ++it)
	{
		// set texture
		R_SetTexture(0, (it->sortKey >> 24) & 65535);
		ChangeTranslucencyMode((enum TRANSLUCENCY_TYPE)	((it->sortKey >> 20) & 15));
		ChangeFilteringMode((enum FILTERING_MODE_ID)	((it->sortKey >> 16) & 15));
		ChangeTextureAddressMode(0, (enum TEXTURE_ADDRESS_MODE) ((it->sortKey >> 15) & 1));
		ChangeZWriteEnable((enum ZWRITE_ENABLE) ((it->sortKey >> 14) & 1));

		uint32_t numPrimitives = (it->indexEnd - it->indexStart) / 3;

		if (numPrimitives)
		{
			R_DrawIndexedPrimitive(this->vertexCount, it->indexStart, numPrimitives);
		}
	}
}
