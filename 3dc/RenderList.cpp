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
#include <assert.h>

extern uint32_t GetNumIndices(uint32_t numVerts);

// bit getters
#define get_unused(src)			((src & 4278190080) >> 24)
#define get_commandType(src)	((src & 16711680) >> 16)
#define get_sequenceNumber(src)	((src & 65472) >> 6)
#define get_command(src)		((src & 32) >> 5)
#define get_transType1(src)		((src & 24) >> 3)
#define get_layerID(src)		(src & 7)

#define get_texID(src)			((src & 4294443008) >> 19)
#define get_transType(src)		((src & 522240) >> 11)
#define get_filterType(src)		((src & 1792) >> 8)
#define get_texAddressType(src)	((src & 192) >> 6)

// bit setters
#define set_unused(out, in)			out |= (in << 24)
#define set_commandType(out, in)	out |= (in << 16)
#define set_sequenceNumber(out, in)	out |= (in << 6)

#define set_command(out, in)		out |= (in << 5)
#define set_transType1(out, in)		out |= (in << 3)
#define set_layerID(out, in)		out |= in

#define set_texID(out, in)			out |= (in << 19)
#define set_transType(out, in)		out |= (in << 11)
#define set_filterType(out, in)		out |= (in << 8)
#define set_texAddressType(out, in)	out |= (in << 6)

RenderList::RenderList(size_t size)
{
	capacity = size;
	listIndex   = 0;
	vertexCount = 0;
	indexCount  = 0;

	layer = 1;

	useIndicesOffset = false;

	// treat the vector as an array so resize it to desired size
	Items.reserve(size);
	Items.resize(size);
}

void RenderList::EnableIndicesOffset()
{
	useIndicesOffset = true;
}

void RenderList::IncrementIndexCount(uint32_t nI)
{
	this->indexCount += nI;
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

void RenderList::AddCommand(int commandType)
{
	Items[listIndex].sortKey = 0;
	set_command     (Items[listIndex].sortKey, 1);
	set_commandType (Items[listIndex].sortKey, commandType);
	listIndex++;
}

void RenderList::SetLayer(int layer)
{
	this->layer = layer;
//	Items[listIndex].layerID = layer;
}

void RenderList::AddItem(uint32_t numVerts, texID_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum FILTERING_MODE_ID filteringMode, enum TEXTURE_ADDRESS_MODE textureAddress)
{
	assert(numVerts != 0);
	assert(textureID < 8191);

	if (GetSize()+1 >= GetCapacity())
	{
		// list is full
		OutputDebugString("List full\n");
		return;
	}

	uint32_t numIndices = GetNumIndices(numVerts);

	Items[listIndex].sortKey = 0; // zero it out

	set_layerID        (Items[listIndex].sortKey, layer);
	set_texID          (Items[listIndex].sortKey, textureID);
	set_transType      (Items[listIndex].sortKey, translucencyMode);
	set_filterType     (Items[listIndex].sortKey, filteringMode);
	set_texAddressType (Items[listIndex].sortKey, textureAddress);

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
		Items[listIndex].vertStart  = vertexCount;
		Items[listIndex].indexStart = indexCount;
	}

	Items[listIndex].vertEnd = vertexCount + numVerts;
	Items[listIndex].indexEnd = indexCount + numIndices;
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
	indexCount  = 0;
}

void RenderList::Draw()
{
	uint32_t baseIndexValue = 0;

	for (size_t i = 0; i < listIndex; i++)
	{
		RenderItem *it = &Items[i];

		if (get_command(it->sortKey)) {
			switch (get_commandType(it->sortKey)) {
				case kCommandZClear:
					R_ClearZBuffer();
					break;
				case kCommandZWriteEnable:
					ChangeZWriteEnable(ZWRITE_ENABLED);
					break;
				case kCommandZWriteDisable:
					ChangeZWriteEnable(ZWRITE_DISABLED);
					break;
			}
		}
		else
		{
			uint32_t numPrimitives = (it->indexEnd - it->indexStart) / 3;

			if (numPrimitives)
			{
				// set texture
				R_SetTexture(0, get_texID(it->sortKey));
				ChangeTranslucencyMode((enum TRANSLUCENCY_TYPE)			get_transType(it->sortKey));
				ChangeFilteringMode(0, (enum FILTERING_MODE_ID)			get_filterType(it->sortKey));
				ChangeTextureAddressMode(0, (enum TEXTURE_ADDRESS_MODE) get_texAddressType(it->sortKey));

				R_DrawIndexedPrimitive(baseIndexValue, 0, this->vertexCount, it->indexStart, numPrimitives);
			}
		}
	}
}
