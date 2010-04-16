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

#include "textureManager.h"
#include <vector>

extern "C" {
extern uint32_t CreateD3DTextureFromFile(const char* fileName, Texture &texture);
}

std::vector<Texture> textureList;

static uint32_t Tex_GetFreeID()
{
	for (uint32_t i = 0; i < textureList.size(); i++)
	{
		if (textureList[i].texture == NULL)
		{
			return texIDoffset + i; // this slot has no texture, return the ID so we can reuse it
		}
	}

	// no free slots in the vector, we'll be adding to the end
	return texIDoffset + textureList.size();
}

uint32_t Tex_AddTexture(LPDIRECT3DTEXTURE9 texture, uint32_t width, uint32_t height)
{
	// get the next available ID
	uint32_t textureID = Tex_GetFreeID();

	Texture newTexture;
	newTexture.texture = texture;
	newTexture.width = width;
	newTexture.height = height;

	// store it
	if ((textureID - texIDoffset) < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID - texIDoffset] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}

	char buf[100];
	sprintf(buf, "added tex at ID: %d\n", textureID);
	OutputDebugString(buf);

	return textureID;
}

uint32_t Tex_LoadFromFile(const std::string &fileName)
{
	// get the next available ID
	uint32_t textureID = Tex_GetFreeID();

	Texture		newTexture;
//	Tex_Info	texInfo;

	uint32_t ret = CreateD3DTextureFromFile(fileName.c_str(), newTexture);

//	newTexture.width = texInfo.width;
//	newTexture.height = texInfo.height;

	// store it
	if ((textureID - texIDoffset) < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID - texIDoffset] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}

	return textureID;
}

LPDIRECT3DTEXTURE9 Tex_GetTexture(uint32_t textureID)
{
	return (textureList[textureID - texIDoffset].texture);
}

void Tex_GetDimensions(uint32_t textureID, uint32_t &width, uint32_t &height)
{
	width  = textureList[textureID - texIDoffset].width;
	height = textureList[textureID - texIDoffset].height;
}

void Tex_Release(uint32_t textureID)
{
	if (textureID < texIDoffset) // bad value
		return;

	if (textureList.at(textureID - texIDoffset).texture)
	{
		// this is bad, temporary..
		textureList[textureID - texIDoffset].texture->Release();
		textureList[textureID - texIDoffset].texture = NULL;
	}

	char buf[100];
	sprintf(buf, "released tex at ID: %d\n", textureID);
	OutputDebugString(buf);
}

void Tex_DeInit()
{
	for (std::vector<Texture>::iterator it = textureList.begin(); it != textureList.end(); ++it)
	{
		if ((*it).texture)
		{
			(*it).texture->Release();
			(*it).texture = NULL;
		}
	}
}
