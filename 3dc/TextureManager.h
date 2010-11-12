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
	uint32_t		width;
	uint32_t		height;
	r_Texture		texture;
	TextureUsage	usage;
	uint8_t			bitsPerPixel;
};

uint32_t Tex_Create(const std::string &textureName, uint32_t width, uint32_t height, uint32_t bpp, enum TextureUsage usageType);
uint32_t Tex_CreateFromAvPTexture(const std::string &textureName, AVPTEXTURE &AvPTexure, enum TextureUsage usageType);
uint32_t Tex_AddTexture(const std::string &textureName, r_Texture texture, uint32_t width, uint32_t height, enum TextureUsage usage = TextureUsage_Normal);
uint32_t Tex_CreateFromFile(const std::string &filePath);
uint32_t Tex_CheckExists(const char* fileName);
const Texture& Tex_GetTextureDetails(uint32_t textureID);
std::string& Tex_GetName(uint32_t textureID);
void Tex_GetNamesVector(std::vector<std::string> &namesArray);
void Tex_GetDimensions(uint32_t textureID, uint32_t &width, uint32_t &height);
bool Tex_Lock(uint32_t textureID, uint8_t **data, uint32_t *pitch, enum TextureLock lockType = TextureLock_Normal);
bool Tex_Unlock(uint32_t textureID);
const r_Texture& Tex_GetTexture(uint32_t textureID);
void Tex_DeInit();
void Tex_Release(uint32_t textureID);
void Tex_ReleaseDynamicTextures();
void Tex_ReloadDynamicTextures();

#endif

