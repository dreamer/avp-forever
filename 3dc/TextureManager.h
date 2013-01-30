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

#ifndef _TextureManager_h_
#define _TextureManager_h_

#include "renderer.h"
#include "stdint.h"
#include <string>
#include <vector>

enum TextureUsage
{
	TextureUsage_Normal,
	TextureUsage_Dynamic
};

enum TextureLock
{
	TextureLock_Normal,
	TextureLock_Discard
};

struct Texture
{
	std::string		name;
	TextureUsage	usage;
	bool			isValid;

	// width and height of textures before adjustment (eg 640x480)
	uint16_t		width;
	uint16_t		height;

	// texture width and height, possible resized up to power-of-two dimension by graphics api (eg 1024x512)
	uint16_t		realWidth;
	uint16_t		realHeight;

	uint8_t			bitsPerPixel;
	r_Texture		texture;

	// constructor
	Texture()
	{
		usage = TextureUsage_Normal;
		isValid = false;
		width  = 0;
		height = 0;
		realWidth  = 1;
		realHeight = 1;
		bitsPerPixel = 0;
		texture = 0;
	}
};

texID_t Tex_Create         (const std::string &textureName, uint32_t width, uint32_t height, uint32_t bitsPerPixel, enum TextureUsage usageType);
texID_t Tex_AddTexture     (const std::string &textureName, r_Texture texture, uint32_t width, uint32_t height, uint32_t bitsPerPixel, enum TextureUsage usage);
texID_t Tex_CreateFromRIM  (const std::string &fileName);
texID_t Tex_CreateFromFile (const std::string &filePath);
texID_t Tex_CheckExists    (const std::string &textureName);

texID_t Tex_AddExistingTexture(Texture &texture);
const Texture& Tex_GetTextureDetails(texID_t textureID);

// texID_t Tex_CreateFromAvPTexture  (const std::string &textureName, AVPTEXTURE &AvPTexure, enum TextureUsage usageType);
texID_t Tex_CreateTallFontTexture (const std::string &textureName, AVPTEXTURE &AvPTexure, enum TextureUsage usageType);

std::string& Tex_GetName	(texID_t textureID);
const r_Texture& Tex_GetTexture	(texID_t textureID);
void Tex_GetNamesVector		(std::vector<std::string> &namesArray);
void Tex_GetDimensions		(texID_t textureID, uint32_t &width, uint32_t &height);

bool Tex_Lock	(texID_t textureID, uint8_t **data, uint32_t *pitch, enum TextureLock lockType = TextureLock_Normal);
bool Tex_Unlock	(texID_t textureID);

void Tex_Release(texID_t textureID);
void Tex_DeInit();
void Tex_ReleaseDynamicTextures();
void Tex_ReloadDynamicTextures();
void Tex_CheckMemoryUsage();
void Tex_ListTextures();

#endif

