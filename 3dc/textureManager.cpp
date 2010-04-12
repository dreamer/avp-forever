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
#include <string>

struct Texture
{
	std::string		name;
	uint32_t		width;
	uint32_t		height;
	D3DPOOL			poolType;
	D3DFORMAT		format;
	LPDIRECT3DTEXTURE9	texture;
};

std::vector<Texture> textureList;

uint32_t Tex_AddTexture(LPDIRECT3DTEXTURE9 texture, uint32_t width, uint32_t height)
{
	// get the next available ID
	uint32_t textureID = (uint32_t)textureList.size();

	Texture newTexture;
	newTexture.texture = texture;
	newTexture.width = width;
	newTexture.height = height;

	// store it
	textureList.push_back(newTexture);
	
	return textureID + texIDoffset;
}

LPDIRECT3DTEXTURE9 Tex_GetTexture(uint32_t textureID)
{
	return (textureList[textureID - texIDoffset].texture);
}

void Tex_GetInfo(uint32_t textureID, Tex_Info *info)
{
	info->width  = textureList[textureID - texIDoffset].width;
	info->height = textureList[textureID - texIDoffset].height;
}

void Tex_Release(uint32_t textureID)
{
	// this is bad, temporary..
	textureList[textureID - texIDoffset].texture->Release();
	textureList[textureID - texIDoffset].texture = NULL;
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
