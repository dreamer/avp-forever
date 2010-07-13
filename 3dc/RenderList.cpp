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
#include <cstring>
#include <cstdio>
#include <windows.h>
#include <algorithm>

RenderList::RenderList()
{
	listCapacity = 0;
	listIndex = 0;

	totalVerts = 0;
}

RenderList::~RenderList()
{
//	delete Items;
}

void RenderList::AddItem(uint32_t numVerts, uint32_t textureID, uint32_t shaderID)
{
	// lets see if we can merge this item with the previous item
	if ((listIndex != 0) &&		// only do this check if we're not adding the first item
		(shaderID == Items[listIndex-1].shaderID) &&
		(textureID == Items[listIndex-1].textureID))
	{
		// our new item uses the same states as previous item.
		listIndex--;
	}
	else
	{
		// we need to add a new item
		Items[listIndex].vertStart = totalVerts;
		Items[listIndex].indexStart = 0; // TODO. totalIndices
	}

	Items[listIndex].sortKey = (textureID << SORT_TEXTURE_SHIFT) | shaderID;

	Items[listIndex].textureID = textureID;
	Items[listIndex].shaderID = shaderID;
	Items[listIndex].vertEnd = totalVerts + numVerts;
	Items[listIndex].indexEnd = 0; // TODO. totalIndicies + numIndices
	listIndex++;

	totalVerts += numVerts;
}

size_t RenderList::GetSize()
{
	return listIndex;
}

void RenderList::Init(size_t size)
{
	// treat the vector as an array so resize it to desired size
	Items.reserve(size);
	Items.resize(size);

	listCapacity = size;
}

void RenderList::Sort()
{
	std::sort(Items.begin(), Items.begin() + listIndex);
}

void RenderList::Reset()
{
	// reset our index back to the first item
	listIndex = 0;
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