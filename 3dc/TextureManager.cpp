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

#include <sstream>
#include "renderer.h"
#include "console.h"
#include <assert.h>
#include "RimLoader.h"

std::vector<Texture> textureList;
std::vector<Texture>::iterator texIt;

size_t lastBytesUsed = 0;
void Tex_CheckMemoryUsage();

bool Tex_Lock(texID_t textureID, uint8_t **data, uint32_t *pitch, enum TextureLock lockType)
{
	// check if it's valid
	if (textureList[textureID].isValid == false)
	{
		data = 0;
		pitch = 0;
		return false;
	}

	if (!textureList[textureID].texture) {
		return false;
	}

	return R_LockTexture(textureList[textureID], data, pitch, lockType);
}

bool Tex_Unlock(texID_t textureID)
{
	// check if it's valid
	if (textureList[textureID].isValid == false) {
		return false;
	}

	if (!textureList[textureID].texture) {
		return false;
	}

	return R_UnlockTexture(textureList[textureID]);
}

// for avp's FMV code
void Tex_GetNamesVector(std::vector<std::string> &namesArray)
{
	for (size_t i = 0; i < textureList.size(); i++)
	{
		// only add valid textures
		if (textureList[i].isValid) {
			namesArray.push_back(textureList[i].name);
		}
	}
}

static texID_t Tex_GetFreeID()
{
	for (size_t i = 0; i < textureList.size(); i++)
	{
		if (textureList[i].isValid == false)
		{
			return i; // this slot has no texture, return the ID so we can reuse it
		}
	}

	// no free slots in the vector, we'll be adding to the end
	return textureList.size();
}

texID_t Tex_CheckExists(const std::string &textureName)
{
	for (uint32_t i = 0; i < textureList.size(); i++)
	{
		if ((textureList[i].name == textureName) && (textureList[i].isValid)) {
			return i;
		}
	}

	return MISSING_TEXTURE;
}

texID_t Tex_AddExistingTexture(Texture &texture)
{
	// get the next available ID
	texID_t textureID = Tex_GetFreeID();

	// store it
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = texture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(texture);
	}

	return textureID;
}

texID_t Tex_AddTexture(const std::string &textureName, r_Texture texture, uint32_t width, uint32_t height, uint32_t bitsPerPixel, enum TextureUsage usage)
{
	// get the next available ID
	texID_t textureID = Tex_GetFreeID();

	Texture newTexture;
	newTexture.name = textureName;
	newTexture.texture = texture;
	newTexture.width  = width;
	newTexture.height = height;
	newTexture.bitsPerPixel = bitsPerPixel;
	newTexture.usage   = usage;
	newTexture.isValid = true;

	// store it
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}

	return textureID;
}

texID_t Tex_CreateTallFontTexture(const std::string &textureName, AVPTEXTURE &AvPTexure, enum TextureUsage usageType)
{
	Texture newTexture;
	texID_t textureID;
	newTexture.name = textureName;

	// see if it exists already
	textureID = Tex_CheckExists(textureName);
	if ((textureID != MISSING_TEXTURE) && (textureList[textureID].isValid))
	{
		return textureID;
	}

	if (!R_CreateTallFontTexture(AvPTexure, usageType, newTexture))
	{
		// log error
		return MISSING_TEXTURE;
	}

	// get the next available ID
	textureID = Tex_GetFreeID();

	// set as valid
	newTexture.isValid = true;

	// add texture to manager
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}

	return textureID;
}

texID_t Tex_CreateFromRIM(const std::string &fileName)
{
	texID_t textureID;
	
	textureID = Tex_CheckExists(fileName);
	if (textureID != MISSING_TEXTURE)
	{
		return textureID;
	}

	RimLoader newRim;

	Texture newTexture;
	newTexture.name = fileName;

	uint32_t width, height;
	uint32_t bitsPerPixel = 32;

	if (!newRim.Open(fileName))
	{
		return MISSING_TEXTURE;
	}

	newRim.GetDimensions(width, height);

	TextureUsage usage = TextureUsage_Normal;

	// TODO - FMV textures must be dynamic. Try handle this some other way
	size_t found = fileName.find("graphics/FMVs");
	if (found != std::string::npos)
	{
		usage = TextureUsage_Dynamic;
	}

	if (!R_CreateTexture(width, height, bitsPerPixel, usage, newTexture))
	{
		// log error
		return MISSING_TEXTURE;
	}

	// lock texture and decode RIM data into it
	uint8_t *dest = 0;
	uint32_t pitch = 0;
	if (!R_LockTexture(newTexture, &dest, &pitch, TextureLock_Normal))
	{
		// log error
		return MISSING_TEXTURE;
	}

	newRim.Decode(dest, pitch);

	R_UnlockTexture(newTexture);

	// get the next available ID
	textureID = Tex_GetFreeID();

	// set as valid
	newTexture.isValid = true;

	// add texture to manager
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}

	return textureID;
}

texID_t Tex_CreateFromAvPTexture(const std::string &textureName, AVPTEXTURE &AvPTexure, enum TextureUsage usageType)
{
	Texture newTexture;
	newTexture.name = textureName;

	if (!R_CreateTextureFromAvPTexture(AvPTexure, usageType, newTexture))
	{
		// log error
		return MISSING_TEXTURE;
	}

	// get the next available ID
	texID_t textureID = Tex_GetFreeID();

	// set as valid
	newTexture.isValid = true;

	// add texture to manager
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}

	return textureID;
}

texID_t Tex_Create(const std::string &textureName, uint32_t width, uint32_t height, uint32_t bitsPerPixel, enum TextureUsage usageType)
{
	Texture newTexture;
	newTexture.name = textureName;

	// check path for backslashes
	std::string::size_type pos = textureName.find("\\");
	if (std::string::npos != pos)
	{
		assert(0);
	}

	if (!R_CreateTexture(width, height, bitsPerPixel, usageType, newTexture))
	{
		// log error
		return MISSING_TEXTURE;
	}

	// get the next available ID
	texID_t textureID = Tex_GetFreeID();

	// set as valid
	newTexture.isValid = true;

	// add texture to manager
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}

	Tex_CheckMemoryUsage();

	return textureID;
}

texID_t Tex_CreateFromFile(const std::string &filePath)
{
	// get the next available ID
	texID_t textureID = Tex_GetFreeID();

	Texture	newTexture;
	newTexture.name = filePath; // use file path as name

	// check path for backslashes
	std::string::size_type pos = filePath.find("\\");
	if (std::string::npos != pos)
	{
		assert(0);
	}

	if (!R_CreateTextureFromFile(filePath, newTexture))
	{
		// return a reference to our missing texture texture
		return MISSING_TEXTURE;
	}

	// set as valid
	newTexture.isValid = true;

	// store it
	if (textureID < textureList.size()) // we're reusing a slot in this case
	{
		textureList[textureID] = newTexture; // replace in the old unused slot
	}
	else // adding on to the end
	{
		textureList.push_back(newTexture);
	}

	return textureID;
}

std::string& Tex_GetName(texID_t textureID)
{
	return textureList[textureID].name;
}

const r_Texture& Tex_GetTexture(texID_t textureID)
{
	// if the texture requested isn't valid, return a reference to our "missing texture" texture
	if (textureList[textureID].isValid == false)
	{
		return textureList[MISSING_TEXTURE].texture;
	}

	return (textureList[textureID].texture);
}

const Texture& Tex_GetTextureDetails(texID_t textureID)
{
	return (textureList[textureID]);
}

void Tex_ReleaseDynamicTextures()
{
	for (texIt = textureList.begin(); texIt != textureList.end(); ++texIt)
	{
		if ((texIt->isValid) && (texIt->usage == TextureUsage_Dynamic))
		{
			R_ReleaseTexture(*texIt);
			texIt->isValid = false;
		}
	}
}

void Tex_ReloadDynamicTextures()
{
	for (texIt = textureList.begin(); texIt != textureList.end(); ++texIt)
	{
		if (texIt->usage == TextureUsage_Dynamic)
		{
			assert(texIt->isValid == false);

			if (!R_CreateTexture(texIt->width, texIt->height, texIt->bitsPerPixel, texIt->usage, (*texIt)))
			{
				Con_PrintError("Couldn't reload texture " + texIt->name);
				texIt->isValid = false;
				return;
			}

			// set as valid
			texIt->isValid = true;

			std::stringstream ss;
			ss << "reloaded texture with name " << texIt->name << " width: " << texIt->width << " height: " << texIt->height << std::endl;
			OutputDebugString(ss.str().c_str());
		}
	}
}

void Tex_GetDimensions(texID_t textureID, uint32_t &width, uint32_t &height)
{
	width  = textureList[textureID].width;
	height = textureList[textureID].height;
}

void Tex_ListTextures()
{
	std::stringstream ss;

	for (size_t i = 0; i < textureList.size(); ++i)
	{
		if (textureList[i].isValid)
		{
			ss.str("");
			ss << "ID: " << i << " Name: " << textureList[i].name << std::endl;
			OutputDebugString(ss.str().c_str());
		}
	}
}

void Tex_CheckMemoryUsage()
{
	return;

	#define MB	(1024*1024)

	size_t bytesUsed = 0;

	for (texIt = textureList.begin(); texIt != textureList.end(); ++texIt)
	{
		if (texIt->isValid)
		{
			bytesUsed += (texIt->width * texIt->height) * (texIt->bitsPerPixel / 8);
		}
	}

	if (bytesUsed == lastBytesUsed)
	{
		return;
	}
/*
	float MBused = (float)bytesUsed / float(MB);

	std::stringstream ss;
	ss << "MB of textures used: " << MBused << std::endl;
	OutputDebugString(ss.str().c_str());
*/
	lastBytesUsed = bytesUsed;
}

void Tex_Release(texID_t textureID)
{
	if ((textureID == MISSING_TEXTURE) || (textureID == NO_TEXTURE))
	{
		// hold onto these (will be released on game exit in Tex_DeInit() )
		return;
	}

	// only release a valid texture
	if (textureList.at(textureID).isValid)
	{
		R_ReleaseTexture(textureList[textureID]);
		R_UnsetTexture(textureID);
/*
		std::stringstream ss;
		ss << "releasing texture at ID: " << textureID << " with name " << textureList[textureID].name << " width: " << textureList[textureID].width << " height: " << textureList[textureID].height << std::endl;
		OutputDebugString(ss.str().c_str());
*/
		// set as invalid
		textureList[textureID].isValid = false;

//		Tex_CheckMemoryUsage();
	}
}

void Tex_DeInit()
{
	for (size_t i = 0; i < textureList.size(); i++)
	{
		Tex_Release(i);
		textureList[i].isValid = false;
	}

	// special case releases
	R_ReleaseTexture(textureList[MISSING_TEXTURE]);
	R_UnsetTexture(MISSING_TEXTURE);

	R_ReleaseTexture(textureList[NO_TEXTURE]);
	R_UnsetTexture(NO_TEXTURE);
}
