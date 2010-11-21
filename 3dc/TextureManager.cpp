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

#include "TextureManager.h"
#include <sstream>

std::vector<Texture> textureList;
std::vector<Texture>::iterator texIt;

void Tex_CheckMemoryUsage();

bool Tex_Lock(uint32_t textureID, uint8_t **data, uint32_t *pitch, enum TextureLock lockType)
{
	r_Texture texture = Tex_GetTexture(textureID);
	if (!texture)
		return false;

	return R_LockTexture(texture, data, pitch, lockType);
}

bool Tex_Unlock(uint32_t textureID)
{
	r_Texture texture = Tex_GetTexture(textureID);
	if (!texture)
		return false;

	return R_UnlockTexture(texture);
}

// for avp's fmv code
void Tex_GetNamesVector(std::vector<std::string> &namesArray)
{
	for (uint32_t i = 0; i < textureList.size(); i++)
	{
		namesArray.push_back(textureList[i].name);
	}
}

static uint32_t Tex_GetFreeID()
{
	for (uint32_t i = 0; i < textureList.size(); i++)
	{
		if (textureList[i].texture == NULL)
		{
			return i; // this slot has no texture, return the ID so we can reuse it
		}
	}

	// no free slots in the vector, we'll be adding to the end
	return static_cast<uint32_t>(textureList.size());
}

uint32_t Tex_CheckExists(const char* fileName)
{
	for (uint32_t i = 0; i < textureList.size(); i++)
	{
		if (textureList[i].name == fileName)
			return i;
	}

	return MISSING_TEXTURE;
}

uint32_t Tex_AddTexture(const std::string &textureName, r_Texture texture, uint32_t width, uint32_t height, uint32_t bitsPerPixel, enum TextureUsage usage)
{
	// get the next available ID
	uint32_t textureID = Tex_GetFreeID();

	Texture newTexture;
	newTexture.name = textureName;
	newTexture.texture = texture;
	newTexture.width = width;
	newTexture.height = height;
	newTexture.bitsPerPixel = bitsPerPixel; 
	newTexture.usage = usage;

	// store it
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}
/*
	std::stringstream ss;
	ss << "added texture at ID: " << textureID << " with name " << textureName << " width: " << newTexture.width << " height: " << newTexture.height << std::endl;
	OutputDebugString(ss.str().c_str());

	Tex_CheckMemoryUsage();
*/
	return textureID;
}

uint32_t Tex_CreateTallFontTexture(const std::string &textureName, AVPTEXTURE &AvPTexure, enum TextureUsage usageType)
{
	Texture newTexture;
	newTexture.name = textureName;

	if (!R_CreateTallFontTexture(AvPTexure, usageType, newTexture))
	{
		// log error
		return MISSING_TEXTURE;
	}

	// get the next available ID
	uint32_t textureID = Tex_GetFreeID();

	// add texture to manager
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}
/*
	std::stringstream ss;
	ss << "added texture at ID: " << textureID << " with name " << textureName << " width: " << newTexture.width << " height: " << newTexture.height << std::endl;
	OutputDebugString(ss.str().c_str());

	Tex_CheckMemoryUsage();
*/
	return textureID;
}

uint32_t Tex_CreateFromAvPTexture(const std::string &textureName, AVPTEXTURE &AvPTexure, enum TextureUsage usageType)
{
	Texture newTexture;
	newTexture.name = textureName;

	if (!R_CreateTextureFromAvPTexture(AvPTexure, usageType, newTexture))
	{
		// log error
		return MISSING_TEXTURE;
	}

	// get the next available ID
	uint32_t textureID = Tex_GetFreeID();

	// add texture to manager
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}
/*
	std::stringstream ss;
	ss << "added texture at ID: " << textureID << " with name " << textureName << " width: " << newTexture.width << " height: " << newTexture.height << std::endl;
	OutputDebugString(ss.str().c_str());

	Tex_CheckMemoryUsage();
*/
	return textureID;
}

uint32_t Tex_Create(const std::string &textureName, uint32_t width, uint32_t height, uint32_t bitsPerPixel, enum TextureUsage usageType)
{
	Texture newTexture;
	newTexture.name = textureName;

	if (!R_CreateTexture(width, height, bitsPerPixel, usageType, newTexture))
	{
		// log error
		return MISSING_TEXTURE;
	}
	
	// get the next available ID
	uint32_t textureID = Tex_GetFreeID();

	// add texture to manager
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}
/*
	std::stringstream ss;
	ss << "added texture at ID: " << textureID << " with name " << textureName << " width: " << width << " height: " << height << std::endl;
	OutputDebugString(ss.str().c_str());

	Tex_CheckMemoryUsage();
*/
	return textureID;
}

uint32_t Tex_CreateFromFile(const std::string &filePath)
{
	// get the next available ID
	uint32_t textureID = Tex_GetFreeID();

	Texture	newTexture;
	newTexture.name = filePath; // use file path as name

	if (!R_CreateTextureFromFile(filePath, newTexture))
	{
		// return a reference to our missing texture texture
		return MISSING_TEXTURE;
	}

	// store it
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}
/*
	std::stringstream ss;
	ss << "added texture at ID: " << textureID << " with name " << filePath << " width: " << newTexture.width << " height: " << newTexture.height << std::endl;
	OutputDebugString(ss.str().c_str());

	Tex_CheckMemoryUsage();
*/
	return textureID;
}

std::string& Tex_GetName(uint32_t textureID)
{
	return textureList[textureID].name;
}

const r_Texture& Tex_GetTexture(uint32_t textureID)
{
	return (textureList[textureID].texture);
}

const Texture& Tex_GetTextureDetails(uint32_t textureID)
{
	return (textureList[textureID]);
}

void Tex_ReleaseDynamicTextures()
{
	for (texIt = textureList.begin(); texIt != textureList.end(); ++texIt)
	{
		if ((texIt->texture) && (texIt->usage == TextureUsage_Dynamic))
		{
			R_ReleaseTexture(texIt->texture);
		}
	}
}

void Tex_ReloadDynamicTextures()
{
	for (texIt = textureList.begin(); texIt != textureList.end(); ++texIt)
	{
		// check for NOT texture (ie released ones)
		if ((texIt->texture != NULL) && (texIt->usage == TextureUsage_Dynamic))
		{
			R_CreateTexture(texIt->width, texIt->height, texIt->bitsPerPixel, texIt->usage, (*texIt));
		}
	}
}

void Tex_GetDimensions(uint32_t textureID, uint32_t &width, uint32_t &height)
{
	width  = textureList[textureID].width;
	height = textureList[textureID].height;
}

size_t lastBytesUsed = 0;

void Tex_CheckMemoryUsage()
{
	#define MB	(1024*1024)

	size_t bytesUsed = 0;

	for (texIt = textureList.begin(); texIt != textureList.end(); ++texIt)
	{
		if (texIt->texture != NULL)
		{
			bytesUsed += (texIt->width * texIt->height) * (texIt->bitsPerPixel / 8);
		}
	}

	if (bytesUsed == lastBytesUsed)
	{
		return;
	}

	float test = (float)bytesUsed / float(MB);

	std::stringstream ss;
	ss << "MB of textures used: " << test << std::endl;
	OutputDebugString(ss.str().c_str());

	lastBytesUsed = bytesUsed;
}

void Tex_Release(uint32_t textureID)
{
	if (textureID == MISSING_TEXTURE)
	{
		// hold onto this (and what about NO_TEXTURE?)
		return;
	}

	// only release a valid texture
	if (textureList.at(textureID).texture != NULL)
	{
		R_ReleaseTexture(textureList[textureID].texture);
/*
		std::stringstream ss;
		ss << "releasing texture at ID: " << textureID << " with name " << textureList[textureID].name << " width: " << textureList[textureID].width << " height: " << textureList[textureID].height << std::endl;
		OutputDebugString(ss.str().c_str());
*/
		// blank the name out
		textureList[textureID].name = ("");

//		Tex_CheckMemoryUsage();
	}
}

void Tex_DeInit()
{
	for (uint32_t i = 0; i < textureList.size(); i++)
	{
		Tex_Release(i);
	}
/*
	for (texIt = textureList.begin(); texIt != textureList.end(); ++texIt)
	{
		if (texIt->texture)
		{
			texIt->texture->Release();
			texIt->texture = NULL;
		}
	}
*/
}
