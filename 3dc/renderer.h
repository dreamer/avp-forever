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

#ifndef _renderer_h_
#define _renderer_h_

#include <stdint.h>
#include <string>
#include <vector>
#include "renderStates.h"

enum R_USAGE
{
	USAGE_DYNAMIC,
	USAGE_STATIC
};

enum R_FVF
{
	FVF_LVERTEX,
	FVF_ORTHO,
	FVF_FMV
};

typedef size_t texID_t;

// forward declarations
class VertexBuffer;
class IndexBuffer;
class VertexDeclaration;
struct r_VertexShader;
struct r_PixelShader;
struct Texture;
struct AVPTEXTURE;

bool R_BeginScene();
bool R_EndScene();

// texture functions
bool R_SetTexture            (uint32_t stage, texID_t textureID);
void R_UnsetTexture          (texID_t textureID);
bool R_LockTexture           (Texture &texture, uint8_t **data, uint32_t *pitch, enum TextureLock lockType);
bool R_UnlockTexture         (Texture &texture);
bool R_CreateTexture         (uint32_t width, uint32_t height, uint32_t bitsPerPixel, enum TextureUsage usageType, Texture &texture);
void R_ReleaseTexture        (Texture &texture);
bool R_CreateTextureFromFile (const std::string &fileName, Texture &texture);
bool R_CreateTallFontTexture       (AVPTEXTURE &tex, enum TextureUsage usageType, Texture &texture);
bool R_CreateTextureFromAvPTexture (AVPTEXTURE &AvPTexture, enum TextureUsage usageType, Texture &texture);

// vertex buffer functions
bool R_CreateVertexBuffer  (class VertexBuffer &vertexBuffer);
bool R_ReleaseVertexBuffer (class VertexBuffer &vertexBuffer);
bool R_LockVertexBuffer    (class VertexBuffer &vertexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, void **data, enum R_USAGE usage);
bool R_UnlockVertexBuffer  (class VertexBuffer &vertexBuffer);
bool R_SetVertexBuffer     (class VertexBuffer &vertexBuffer);
bool R_DrawPrimitive       (uint32_t numPrimitives);
bool R_DrawIndexedPrimitive(uint32_t numVerts, uint32_t startIndex, uint32_t numPrimitives);

// index buffer functions
bool R_CreateIndexBuffer   (class IndexBuffer &indexBuffer);
bool R_ReleaseIndexBuffer  (class IndexBuffer &indexBuffer);
bool R_LockIndexBuffer     (class IndexBuffer &indexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, uint16_t **data, enum R_USAGE usage);
bool R_UnlockIndexBuffer   (class IndexBuffer &indexBuffer);
bool R_SetIndexBuffer      (class IndexBuffer &indexBuffer);

// vertex declaration
bool R_CreateVertexDeclaration  (class VertexDeclaration *vertexDeclaration);
bool R_SetVertexDeclaration     (class VertexDeclaration &declaration);
bool R_ReleaseVertexDeclaration (class VertexDeclaration &declaration);

// vertex shader functions
bool R_CreateVertexShader      (const std::string &fileName, r_VertexShader &vertexShader, VertexDeclaration *vertexDeclaration);
bool R_SetVertexShader         (r_VertexShader &vertexShader);
void R_ReleaseVertexShader     (r_VertexShader &vertexShader);
bool R_SetVertexShaderConstant (r_VertexShader &vertexShader, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData);

// pixel shader functions
bool R_CreatePixelShader  (const std::string &fileName, r_PixelShader &pixelShader);
bool R_SetPixelShader     (r_PixelShader &pixelShader);
void R_ReleasePixelShader (r_PixelShader &pixelShader);

void R_NextVideoMode();
void R_PreviousVideoMode();
std::string& R_GetVideoModeDescription();
void R_SetCurrentVideoMode();

void ChangeTranslucencyMode   (enum TRANSLUCENCY_TYPE translucencyRequired);
void ChangeTextureAddressMode (uint32_t samplerIndex, enum TEXTURE_ADDRESS_MODE textureAddressMode);
void ChangeFilteringMode      (uint32_t samplerIndex, enum FILTERING_MODE_ID filteringRequired);
void ChangeZWriteEnable       (enum ZWRITE_ENABLE zWriteEnable);

bool InitialiseDirect3D();
bool R_ChangeResolution		(uint32_t width, uint32_t height);
void DrawAlphaMenuQuad		(uint32_t topX, uint32_t topY, texID_t textureID, uint32_t alpha);
void DrawTallFontCharacter	(uint32_t topX, uint32_t topY, texID_t textureID, uint32_t texU, uint32_t texV, uint32_t charWidth, uint32_t alpha);
void DrawCloudTable			(uint32_t topX, uint32_t topY, uint32_t wordLength, uint32_t alpha);
void DrawFadeQuad			(uint32_t topX, uint32_t topY, uint32_t alpha);
void DrawSmallMenuCharacter (uint32_t topX, uint32_t topY, uint32_t texU, uint32_t texV, uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha);
void DrawQuad				(uint32_t x, uint32_t y, uint32_t width, uint32_t height, texID_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType);
void DrawFmvFrame			(uint32_t frameWidth, uint32_t frameHeight, const std::vector<texID_t> &textureIDs);
void DrawFontQuad			(uint32_t x, uint32_t y, uint32_t charWidth, uint32_t charHeight, texID_t textureID, float *uvArray, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType);
void CreateScreenShotImage  ();
void DeRedTexture           (Texture &texture);
void SetTransforms();
uint32_t R_GetNumVideoModes();
char*    R_GetDeviceName();

#ifdef USE_D3D9
	#include <../win32/d3_func.h>
#endif

#ifdef USE_D3D_XBOX
	#include <../xbox/src/d3_func.h>
#endif

#ifdef USE_OPENGL
	#include "ogl_func.h" // doesn't exist yet
#endif

#ifdef USE_GLIDE
	#include "glide_func.h" // hehe
#endif

#endif // include guard