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
#include <windows.h>
#include <algorithm>
#include <assert.h>

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
//	delete Items;
}

void RenderList::CreateIndicies(uint16_t *indexArray, uint32_t numVerts)
{
	switch (numVerts)
	{
		default:
			OutputDebugString("unexpected number of verts to render\n");
			break;
		case 0:
			OutputDebugString("Asked to render 0 verts\n");
			break;
		case 3:
		{
			AddIndicies(indexArray, 0,2,1, 3);
			break;
		}
		case 4:
		{
			AddIndicies(indexArray, 0,1,2, 4);
			AddIndicies(indexArray, 0,2,3, 4);
			break;
		}
		case 5:
		{
			AddIndicies(indexArray, 0,1,4, 5);
			AddIndicies(indexArray, 1,3,4, 5);
			AddIndicies(indexArray, 1,2,3, 5);
			break;
		}
		case 6:
		{
			AddIndicies(indexArray, 0,4,5, 6);
			AddIndicies(indexArray, 0,3,4, 6);
			AddIndicies(indexArray, 0,2,3, 6);
			AddIndicies(indexArray, 0,1,2, 6);
			break;
		}
		case 7:
		{
			AddIndicies(indexArray, 0,5,6, 7);
			AddIndicies(indexArray, 0,4,5, 7);
			AddIndicies(indexArray, 0,3,4, 7);
			AddIndicies(indexArray, 0,2,3, 7);
			AddIndicies(indexArray, 0,1,2, 7);
			break;
		}
		case 8:
		{
			AddIndicies(indexArray, 0,6,7, 8);
			AddIndicies(indexArray, 0,5,6, 8);
			AddIndicies(indexArray, 0,4,5, 8);
			AddIndicies(indexArray, 0,3,4, 8);
			AddIndicies(indexArray, 0,2,3, 8);
			AddIndicies(indexArray, 0,1,2, 8);
			break;
		}
	}
}

void RenderList::AddIndicies(uint16_t *indexArray, uint32_t a, uint32_t b, uint32_t c, uint32_t n)
{
	indexArray[this->indexCount]   = (vertexCount - (n) + (a));
	indexArray[this->indexCount+1] = (vertexCount - (n) + (b));
	indexArray[this->indexCount+2] = (vertexCount - (n) + (c));
	this->indexCount+=3;
}

void RenderList::AddItem(uint32_t numVerts, uint32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum FILTERING_MODE_ID filteringMode)
{
	assert(numVerts != 0);

	uint32_t realNumVerts = GetRealNumVerts(numVerts);

	Items[listIndex].sortKey = 0; // zero it out
	Items[listIndex].sortKey = (textureID << 24) | (translucencyMode << 20) | (filteringMode << 16);

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
	for (std::vector<RenderItem2>::iterator it = Items.begin(); it != /*Items.end()*/Items.begin() + listIndex; ++it)
	{
		// set texture
		R_SetTexture(0, it->sortKey >> 24);
		ChangeTranslucencyMode((enum TRANSLUCENCY_TYPE)	((it->sortKey >> 20) & 15));
		ChangeFilteringMode((enum FILTERING_MODE_ID)	((it->sortKey >> 16) & 15));

		uint32_t numPrimitives = (it->indexEnd - it->indexStart) / 3;

		if (numPrimitives)
		{
			// bjd - FIXME
			if (this->vertexCount == 0)
			{
				return;
			}
			R_DrawIndexedPrimitive(this->vertexCount, it->indexStart, numPrimitives);
		}
	}
}

#if 0
int main(int argc, char *argv[])
{
	RenderList list;
	list.Init(300);

	list.AddItem(123, 1, 3);
	list.AddItem(1024, 1, 5);
	list.AddItem(30, 2, 9);
	list.AddItem(30, 3, 6);
	list.AddItem(45, 2, 5);
	list.AddItem(12, 3, 3);

	list.AddItem(70, 3, 6);

	list.AddItem(1200, 3, 3);

	// loop and print list
	char buf[100];
	sprintf(buf, "%d list items\n", list.GetSize());
	OutputDebugString(buf);

	for (uint32_t i = 0; i < list.GetSize(); i++)
	{
//		sprintf(buf, "textureID %d, numVerts: %d, numIndicies: %d\n", list.Items[i].textureID, list.Items[i].vertEnd - list.Items[i].vertStart, list.Items[i].indexEnd - list.Items[i].indexStart);
//		OutputDebugString(buf);

		sprintf(buf, "textureID %d, shaderID: %d, sortKey: %d\n", list.Items[i].sortKey >> SORT_TEXTURE_SHIFT, (uint16_t)list.Items[i].sortKey, list.Items[i].sortKey);
		OutputDebugString(buf);
	}

	// sort and list again
	OutputDebugString("\n");

	list.Sort();

	for (uint32_t i = 0; i < list.GetSize(); i++)
	{
//		sprintf(buf, "textureID %d, numVerts: %d, numIndicies: %d\n", list.Items[i].textureID, list.Items[i].vertEnd - list.Items[i].vertStart, list.Items[i].indexEnd - list.Items[i].indexStart);
//		OutputDebugString(buf);

		sprintf(buf, "textureID %d, shaderID: %d\n", list.Items[i].sortKey >> SORT_TEXTURE_SHIFT, (uint16_t)list.Items[i].sortKey);
		OutputDebugString(buf);
	}
}
#endif