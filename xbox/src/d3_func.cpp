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

#include "d3_func.h"
#include "fmvCutscenes.h"
#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
#include "logString.h"
#include "configFile.h"
#include "console.h"
#include "networking.h"
#include "font2.h"
#include <xgraphics.h>
#include <xgmath.h>
#include "AvP_UserProfile.h"

// Alien FOV - 115
// Marine & Predator FOV - 77

/*
// shaders
const char *orthoShader =
    "vs.1.1\n"
    "def c4, 0, 0, 0, 0\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "dp4 oPos.z, v0, c2\n"
    "dp4 oPos.w, v0, c3\n"
    "mov oD0, v1\n"
    "mov oD1, c4\n"
    "mov oT0, v2\0";

const char *vertShader =
    "vs.1.1\n"
    "dp4 oPos.x, v0, c0\n"
    "dp4 oPos.y, v0, c1\n"
    "dp4 oPos.z, v0, c2\n"
    "dp4 oPos.w, v0, c3\n"
    "mov oD0, v1\n"
    "mov oD1, v2\n"
    "mov oT0.xy, v3\0";

const char *pixelShader =
	"ps.1.1\n"
    "def c4, 0, 0, 0, 0\n"
    "tex t0\n"
    "add r0, v0, v1\n"
    "mul r0, t0, r0\0";
*/

D3DXMATRIX matOrtho;
D3DXMATRIX matProjection;
D3DXMATRIX matView;
D3DXMATRIX matIdentity;
D3DXMATRIX matViewPort;

static D3DXPLANE m_frustum[6];

extern void RenderListInit();
extern void RenderListDeInit();
extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);

// size of vertex and index buffers
const uint32_t kMaxVertices = 4096;
const uint32_t kMaxIndices  = 9216;

static HRESULT LastError;
texID_t NO_TEXTURE = 0;
texID_t MISSING_TEXTURE = 1;

// keep track of set render states
static bool	D3DAlphaBlendEnable;
D3DBLEND D3DSrcBlend;
D3DBLEND D3DDestBlend;
RENDERSTATES CurrentRenderStates;
bool D3DAlphaTestEnable = FALSE;
static bool D3DStencilEnable;
D3DCMPFUNC D3DStencilFunc;
static D3DCMPFUNC D3DZFunc;

int	VideoModeColourDepth;
int	NumAvailableVideoModes;
D3DINFO d3d;
bool usingStencil = false;

std::string shaderPath;
std::string videoModeDescription;

bool CreateVolatileResources();
bool ReleaseVolatileResources();
bool SetRenderStateDefaults();
void ToggleWireframe();

const int kMaxTextureStages = 4;
std::vector<uint32_t> setTextureArray;

// byte order macros for A8R8G8B8 d3d texture
enum
{
	BO_BLUE,
	BO_GREEN,
	BO_RED,
	BO_ALPHA
};

// TGA header structure
#pragma pack(1)
struct TGA_HEADER
{
	char		idlength;
	char		colourmaptype;
	char		datatypecode;
	int16_t		colourmaporigin;
	int16_t 	colourmaplength;
	char		colourmapdepth;
	int16_t		x_origin;
	int16_t		y_origin;
	int16_t		width;
	int16_t		height;
	char		bitsperpixel;
	char		imagedescriptor;
};
#pragma pack()

static TGA_HEADER TgaHeader = {0};

bool IsPowerOf2(int i)
{
	if ((i & -i) == i) {
		return true;
	}
	else return false;
}

int NearestSuperiorPow2(int i)
{
	int x = ((i - 1) & i);
	return x ? NearestSuperiorPow2(x) : i << 1;
}

uint32_t XPercentToScreen(float percent)
{
	return ((((float)ScreenDescriptorBlock.SDB_Width) / 100) * percent);
}

uint32_t YPercentToScreen(float percent)
{
	return ((((float)ScreenDescriptorBlock.SDB_Height) / 100) * percent);
}

uint32_t R_GetScreenWidth()
{
	return ScreenDescriptorBlock.SDB_Width;
}

bool ReleaseVolatileResources()
{
	Tex_ReleaseDynamicTextures();

	SAFE_RELEASE(d3d.mainVB);
	SAFE_RELEASE(d3d.mainIB);

	SAFE_RELEASE(d3d.particleVB);
	SAFE_RELEASE(d3d.particleIB);

	SAFE_RELEASE(d3d.orthoVB);
	SAFE_RELEASE(d3d.orthoIB);

	return true;
}

bool CheckPointIsInFrustum(D3DXVECTOR3 *point)
{
	// check if point is in front of each plane
	for (int i = 0; i < 6; i++)
	{
		if (D3DXPlaneDotCoord(&m_frustum[i], point) < 0.0f)
		{
			// its outside
			return false;
		}
	}

	// return here if the point is entirely within
	return true;
}

static float deltaTime = 0.0f;

void UpdateTestTimer()
{
	static float currentTime = timeGetTime();

	float newTime = timeGetTime();
	float frameTime = newTime - currentTime;
	currentTime = newTime;

	deltaTime = frameTime;

	std::stringstream ss;

	ss << deltaTime;

	Font_DrawCenteredText(ss.str());
}

float GetTestTimer()
{
	return deltaTime;
}

bool R_BeginScene()
{
	LastError = d3d.lpD3DDevice->BeginScene();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	LastError = d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_EndScene()
{
	LastError = d3d.lpD3DDevice->EndScene();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_CreateVertexBuffer(class VertexBuffer &vertexBuffer)
{
	// TODO - how to handle dynamic VBs? thinking of new-ing some space and using DrawPrimitiveUP functions?

	vertexBuffer.vertexBuffer.dynamicVBMemory = 0;
	vertexBuffer.vertexBuffer.dynamicVBMemorySize = 0;
	vertexBuffer.vertexBuffer.vertexBuffer = 0;

	D3DPOOL vbPool;
	DWORD	vbUsage;

	switch (vertexBuffer.usage)
	{
		case USAGE_STATIC:
			vbUsage = 0;
			vbPool  = D3DPOOL_MANAGED;
			break;
		case USAGE_DYNAMIC:
			vbUsage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
			vbPool  = D3DPOOL_DEFAULT;
			break;
		default:
			// error and return
			Con_PrintError("Unknown vertex buffer usage type");
			return false;
			break;
	}

	if (USAGE_STATIC == vertexBuffer.usage)
	{
		LastError = d3d.lpD3DDevice->CreateVertexBuffer(vertexBuffer.sizeInBytes, vbUsage, 0, vbPool, &vertexBuffer.vertexBuffer.vertexBuffer);
		if (FAILED(LastError))
		{
			Con_PrintError("Can't create static vertex buffer");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
		}
	}
	else if (USAGE_DYNAMIC == vertexBuffer.usage)
	{
		vertexBuffer.vertexBuffer.dynamicVBMemory = static_cast<uint8_t*>(GlobalAlloc(GMEM_FIXED, vertexBuffer.sizeInBytes));
		if (vertexBuffer.vertexBuffer.dynamicVBMemory == NULL)
		{
			Con_PrintError("Can't create dynamic vertex buffer - GlobalAlloc() failed");
			return false;
		}
		vertexBuffer.vertexBuffer.dynamicVBMemorySize = vertexBuffer.sizeInBytes;
	}

	return true;
}

bool R_ReleaseVertexBuffer(class VertexBuffer &vertexBuffer)
{
	if (vertexBuffer.vertexBuffer.vertexBuffer)
	{
		vertexBuffer.vertexBuffer.vertexBuffer->Release();
	}

	return true;
}

bool R_CreateIndexBuffer(class IndexBuffer &indexBuffer)
{
	D3DPOOL ibPool;
	DWORD	ibUsage;

	indexBuffer.indexBuffer.dynamicIBMemory = 0;
	indexBuffer.indexBuffer.dynamicIBMemorySize = 0;
	indexBuffer.indexBuffer.indexBuffer = 0;

	switch (indexBuffer.usage)
	{
		case USAGE_STATIC:
			ibUsage = 0;
			ibPool  = D3DPOOL_MANAGED;
			break;
		case USAGE_DYNAMIC:
			ibUsage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
			ibPool  = D3DPOOL_DEFAULT;
			break;
		default:
			Con_PrintError("Unknown index buffer usage type");
			return false;
			break;
	}

	if (USAGE_STATIC == indexBuffer.usage)
	{
		LastError = d3d.lpD3DDevice->CreateIndexBuffer(indexBuffer.sizeInBytes, ibUsage, D3DFMT_INDEX16, ibPool, &indexBuffer.indexBuffer.indexBuffer);
		if (FAILED(LastError))
		{
			Con_PrintError("Can't create index buffer");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
		}
	}
	else if (USAGE_DYNAMIC == indexBuffer.usage)
	{
		indexBuffer.indexBuffer.dynamicIBMemory = static_cast<uint8_t*>(GlobalAlloc(GMEM_FIXED, indexBuffer.sizeInBytes));
		if (indexBuffer.indexBuffer.dynamicIBMemory == NULL)
		{
			Con_PrintError("Can't create index buffer - GlobalAlloc() failed");
			return false;
		}
		indexBuffer.indexBuffer.dynamicIBMemorySize = indexBuffer.sizeInBytes;
	}

	return true;
}

bool R_ReleaseIndexBuffer(class IndexBuffer &indexBuffer)
{
	if (indexBuffer.indexBuffer.indexBuffer)
	{
		indexBuffer.indexBuffer.indexBuffer->Release();
	}

	return true;
}

uint32_t R_GetNumVideoModes()
{
	return d3d.Driver[d3d.CurrentDriver].NumModes;
}

void R_NextVideoMode()
{
	uint32_t numModes = R_GetNumVideoModes();

	if (++d3d.CurrentVideoMode >= numModes)
	{
		d3d.CurrentVideoMode = 0;
	}
}

void R_PreviousVideoMode()
{
	uint32_t numModes = R_GetNumVideoModes();

	if (--d3d.CurrentVideoMode < 0)
	{
		d3d.CurrentVideoMode = numModes - 1;
	}
}

std::string& R_GetVideoModeDescription()
{
	videoModeDescription = IntToString(d3d.Driver[d3d.CurrentDriver].DisplayMode[d3d.CurrentVideoMode].Width)
						   + "x" +
						   IntToString(d3d.Driver[d3d.CurrentDriver].DisplayMode[d3d.CurrentVideoMode].Height);

	return videoModeDescription;
}

char* R_GetDeviceName()
{
	return "Microsoft XBOX Direct3D8";
}

void R_SetCurrentVideoMode()
{
	uint32_t currentWidth  = d3d.Driver[d3d.CurrentDriver].DisplayMode[d3d.CurrentVideoMode].Width;
	uint32_t currentHeight = d3d.Driver[d3d.CurrentDriver].DisplayMode[d3d.CurrentVideoMode].Height;

	// set the new values in the config file
	Config_SetInt("[VideoMode]", "Width" , currentWidth);
	Config_SetInt("[VideoMode]", "Height", currentHeight);

	// save the changes to the config file
	Config_Save();

	// and actually change the resolution on the device
	R_ChangeResolution(currentWidth, currentHeight);
}

bool R_SetTexture(uint32_t stage, texID_t textureID)
{
	// check that the stage value is within range
	if (stage > kMaxTextureStages-1)
	{
		Con_PrintError("Invalid texture stage: " + IntToString(stage) + " set for texture: " + Tex_GetName(textureID));
		return false;
	}

	// check if the texture is already set
	if (setTextureArray[stage] == textureID)
	{
		// already set, just return
		return true;
	}

	// we need to set it
	LastError = d3d.lpD3DDevice->SetTexture(stage, Tex_GetTexture(textureID));
	if (FAILED(LastError))
	{
		Con_PrintError("Unable to set texture: " + Tex_GetName(textureID));
		return false;
	}
	else
	{
		// ok, update set texture array
		setTextureArray[stage] = textureID;
	}

	return true;
}

void R_UnsetTexture(texID_t textureID)
{
	// unbind this texture if it's set to any stage
	for (uint32_t i = 0; i < kMaxTextureStages; i++)
	{
		if (setTextureArray[i] == textureID)
		{
			setTextureArray[i] = NO_TEXTURE;
		}
	}
}

bool R_LockVertexBuffer(class VertexBuffer &vertexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, void **data, enum R_USAGE usage)
{
	DWORD vbFlags = 0;

	if (usage == USAGE_DYNAMIC)
	{
		(*data) = (void*)(vertexBuffer.vertexBuffer.dynamicVBMemory + offsetToLock);
	}
	else if (usage == USAGE_STATIC)
	{
		LastError = vertexBuffer.vertexBuffer.vertexBuffer->Lock(offsetToLock, sizeToLock, reinterpret_cast<byte**>(data), vbFlags);
		if (FAILED(LastError))
		{
			Con_PrintError("Can't vertex index buffer");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
		}
	}

	vertexBuffer.isLocked = true;

	return true;
}

static uint8_t *currentVBPointer = 0;
static uint8_t *currentIBPointer = 0;
static uint32_t currentVertexStride = 0;

/*
	numVerts - number of vertices in the VB (not how many we actually end up rendering due to indexing
*/
bool R_DrawIndexedPrimitive(uint32_t numVerts, uint32_t startIndex, uint32_t numPrimitives)
{
	if (!currentVBPointer && !currentIBPointer)
	{
		// draw from a VB and IB
		LastError = d3d.lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
			0,
			0,
//			numVerts,				// total num verts in VB
			startIndex,
			numPrimitives);
	}
	else
	{
		assert(currentVBPointer);
		assert(currentIBPointer);

		LastError = d3d.lpD3DDevice->DrawIndexedVerticesUP(D3DPT_TRIANGLELIST,//DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
			//	0,
			//	0,
			//	numPrimitives,
				numVerts,
				static_cast<void*>(currentIBPointer),
			//	D3DFMT_INDEX16,
				static_cast<void*>(currentVBPointer),
				currentVertexStride);
	}

	if (FAILED(LastError))
	{
		Con_PrintError("Can't draw indexed primitive");
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_LockIndexBuffer(class IndexBuffer &indexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, uint16_t **data, enum R_USAGE usage)
{
	DWORD ibFlags = 0;

	if (usage == USAGE_DYNAMIC)
	{
		(*data) = (uint16_t*)(indexBuffer.indexBuffer.dynamicIBMemory + offsetToLock);
	}
	else if (usage == USAGE_STATIC)
	{
		LastError = indexBuffer.indexBuffer.indexBuffer->Lock(offsetToLock, sizeToLock, (byte**)data, ibFlags);
		if (FAILED(LastError))
		{
			Con_PrintError("Can't lock index buffer");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
		}
	}

	indexBuffer.isLocked = true;

	return true;
}

bool R_SetVertexBuffer(class VertexBuffer &vertexBuffer)
{
	currentVBPointer = 0;

	if (vertexBuffer.vertexBuffer.dynamicVBMemory)
	{
		currentVBPointer = vertexBuffer.vertexBuffer.dynamicVBMemory;
		currentVertexStride = vertexBuffer.stride;
	}
	else if (vertexBuffer.vertexBuffer.vertexBuffer)
	{
		assert(vertexBuffer.stride);

		LastError = d3d.lpD3DDevice->SetStreamSource(0, vertexBuffer.vertexBuffer.vertexBuffer, vertexBuffer.stride);
		if (FAILED(LastError))
		{
			Con_PrintError("Can't set vertex buffer");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
		}
	}

	return true;
}

bool R_SetIndexBuffer(class IndexBuffer &indexBuffer)
{
	currentIBPointer = 0;

	if (indexBuffer.indexBuffer.dynamicIBMemory)
	{
		currentIBPointer = indexBuffer.indexBuffer.dynamicIBMemory;
	}
	else if (indexBuffer.indexBuffer.indexBuffer)
	{
		LastError = d3d.lpD3DDevice->SetIndices(indexBuffer.indexBuffer.indexBuffer, 0);
		if (FAILED(LastError))
		{
			Con_PrintError("Can't set index buffer");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
		}
	}

	return true;
}

bool R_UnlockVertexBuffer(class VertexBuffer &vertexBuffer)
{
	if (vertexBuffer.usage == USAGE_DYNAMIC)
	{
		// nothing to do here yet
		return true;
	}
	else if (vertexBuffer.usage == USAGE_STATIC)
	{
		LastError = vertexBuffer.vertexBuffer.vertexBuffer->Unlock();
		if (FAILED(LastError))
		{
			Con_PrintError("Can't unlock vertex buffer");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
		}
	}

	vertexBuffer.isLocked = false;

	return true;
}

bool R_UnlockIndexBuffer(class IndexBuffer &indexBuffer)
{
	if (indexBuffer.usage == USAGE_DYNAMIC)
	{
		// nothing to do here yet
		return true;
	}
	else if (indexBuffer.usage == USAGE_STATIC)
	{
		LastError = indexBuffer.indexBuffer.indexBuffer->Unlock();
		if (FAILED(LastError))
		{
			Con_PrintError("Can't unlock index buffer");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
		}
	}

	indexBuffer.isLocked = false;

	return true;
}

bool CreateVolatileResources()
{
	Tex_ReloadDynamicTextures();

	// main
	d3d.mainVB = new VertexBuffer;
	d3d.mainVB->Create(kMaxVertices*5, FVF_LVERTEX, USAGE_STATIC);

	d3d.mainIB = new IndexBuffer;
	d3d.mainIB->Create((kMaxIndices*5) * 3, USAGE_STATIC);

	// orthographic projected quads
	d3d.orthoVB = new VertexBuffer;
	d3d.orthoVB->Create(kMaxVertices, FVF_ORTHO, USAGE_STATIC);

	d3d.orthoIB = new IndexBuffer;
	d3d.orthoIB->Create(kMaxIndices * 3, USAGE_STATIC);

	// particle vertex buffer
	d3d.particleVB = new VertexBuffer;
	d3d.particleVB->Create(kMaxVertices*6, FVF_LVERTEX, USAGE_STATIC);

	d3d.particleIB = new IndexBuffer;
	d3d.particleIB->Create((kMaxIndices*6) * 3, USAGE_STATIC);

	SetRenderStateDefaults();

	// going to clear texture stages too
	for (int stage = 0; stage < kMaxTextureStages; stage++)
	{
		LastError = d3d.lpD3DDevice->SetTexture(stage, Tex_GetTexture(NO_TEXTURE));
		if (FAILED(LastError))
		{
			Con_PrintError("Unable to reset texture stage: " + stage);
			return false;
		}

		setTextureArray[stage] = NO_TEXTURE;
	}

	return true;
}

uint8_t *aaUnswizzled = 0;
uint32_t gPitch = 0;
uint8_t  *gPbits = 0;

bool R_LockTexture(Texture &texture, uint8_t **data, uint32_t *pitch, enum TextureLock lockType)
{
	D3DLOCKED_RECT lock;
	uint32_t lockFlag;

	switch (lockType)
	{
		case TextureLock_Normal:
			lockFlag = 0;
			break;
		case TextureLock_Discard:
			lockFlag = D3DLOCK_DISCARD;
			break;
		default:
			// error and return
			break;
	}

	LastError = texture.texture->LockRect(0, &lock, NULL, lockFlag);
	if (FAILED(LastError))
	{
		Con_PrintError("Unable to lock texture");
		*data = 0;
		*pitch = 0;
		return false;
	}
	else
	{
		// hack for aa_font :(
//		if (textureID == AVPMENUGFX_SMALL_FONT)
		if ((texture.height != 368) && (texture.height != 184))
		{
			aaUnswizzled = new uint8_t[texture.width*texture.height*4];

			XGUnswizzleRect(lock.pBits, texture.width, texture.height, NULL, aaUnswizzled, (texture.width*4), NULL, 4);

			gPbits = static_cast<uint8_t*>(lock.pBits);
			gPitch = lock.Pitch;

			*data = aaUnswizzled;
			*pitch = (texture.width*4);
		}
		else
		{
			*data = static_cast<uint8_t*>(lock.pBits);
			*pitch = lock.Pitch;
		}

		return true;
	}
}

bool R_UnlockTexture(Texture &texture)
{
	if (aaUnswizzled)
	{
		// reupload texture
		XGSwizzleRect(aaUnswizzled, (texture.width*4), NULL, gPbits, texture.width, texture.height, NULL, 4);

		delete[] aaUnswizzled;
		aaUnswizzled = 0;

		gPbits = 0;
		gPitch = 0;
	}

	LastError = texture.texture->UnlockRect(0);
	if (FAILED(LastError))
	{
		Con_PrintError("Unable to unlock texture");
		return false;
	}

	return true;
}

void ColourFillBackBuffer(int FillColour)
{
	d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, FillColour, 1.0f, 0);
}

// console command : set a new field of view value
void SetFov()
{
	if (Con_GetNumArguments() == 0)
	{
		Con_PrintMessage("usage: r_setfov 75");
		return;
	}

	d3d.fieldOfView = StringToInt(Con_GetArgument(0));

	SetTransforms();
}

extern void CheckWireFrameMode(int shouldBeOn)
{
	if (shouldBeOn)
		shouldBeOn = 1;

	if (CurrentRenderStates.WireFrameModeIsOn != shouldBeOn)
	{
		CurrentRenderStates.WireFrameModeIsOn = shouldBeOn;
		if (shouldBeOn)
		{
			d3d.lpD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		}
		else
		{
			d3d.lpD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		}
	}
}

void R_SetFov(uint32_t fov)
{
	d3d.fieldOfView = fov;
}

void FlushD3DZBuffer()
{
	d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

// console command : output all menu textures as .png files
void WriteMenuTextures()
{
#if 0
	char *filename = new char[strlen(GetSaveFolderPath()) + MAX_PATH];

	for (uint32_t i = 0; i < 54; i++) // 54 == MAX_NO_OF_AVPMENUGFXS
	{
		sprintf(filename, "%s%d.png", GetSaveFolderPath(), i);

		D3DXSaveTextureToFileA(filename, D3DXIFF_PNG, AvPMenuGfxStorage[i].menuTexture, NULL);
	}
#endif
}

// called from Scrshot.cpp
void CreateScreenShotImage()
{

	// TODO
}

/*
 * This function is responsible for creating the large sized font texture, as used in the game menu system. Originally this
 * texture was only handled by DirectDraw, and has a resolution of 30 x 7392, which cannot be loaded as a standard Direct3D texture.
 * The original image (IntroFont.RIM) is a bitmap font, containing one letter per row (width of 30px). The below function takes this
 * bitmap font and rearranges it into a square texture, now containing more letters per row (as a standard bitmap font would be)
 * which can be used by Direct3D without issue.
 */
bool R_CreateTallFontTexture(AVPTEXTURE &tex, enum TextureUsage usageType, Texture &texture)
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;
	LPDIRECT3DTEXTURE8 swizTexture = NULL;
	D3DLOCKED_RECT	   lock;

	// default colour format
	D3DFORMAT swizzledColourFormat = D3DFMT_A8R8G8B8;
	D3DFORMAT colourFormat = D3DFMT_A8R8G8B8;

	D3DPOOL texturePool;
	uint32_t textureUsage;

	// ensure this will be NULL unless we successfully create it
	texture.texture = 0;

	switch (usageType)
	{
		case TextureUsage_Normal:
		{
			texturePool = D3DPOOL_MANAGED;
			textureUsage = 0;
			break;
		}
		case TextureUsage_Dynamic:
		{
			texturePool = D3DPOOL_DEFAULT;
			textureUsage = 0; //D3DUSAGE_DYNAMIC; - not valid on xbox
			break;
		}
		default:
		{
			Con_PrintError("Invalid texture usageType value in R_CreateTallFontTexture");
			return false;
		}
	}

	uint32_t width  = 450;
	uint32_t height = 495;

	uint32_t padWidth  = 512;
	uint32_t padHeight = 512;

	uint32_t charWidth  = 30;
	uint32_t charHeight = 33;

	uint32_t numTotalChars = tex.height / charHeight;

	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, textureUsage, colourFormat, texturePool, &destTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if (FAILED(LastError))
	{
		destTexture->Release();
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	uint8_t *destPtr, *srcPtr;

	srcPtr = (uint8_t*)tex.buffer;

	D3DCOLOR padColour = D3DCOLOR_XRGB(0,0,0);

	// lets pad the whole thing black first
	for (uint32_t y = 0; y < padHeight; y++)
	{
		destPtr = (((uint8_t*)lock.pBits) + y*lock.Pitch);

		for (uint32_t x = 0; x < padWidth; x++)
		{
			// >> 3 for red and blue in a 16 bit texture, 2 for green
			*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(padColour, padColour, padColour, padColour);

			destPtr += sizeof(uint32_t); // move over an entire 4 byte pixel
		}
	}

	for (uint32_t i = 0; i < numTotalChars; i++)
	{
		uint32_t row = i / 15; // get row
		uint32_t column = i % 15; // get column from remainder value

		uint32_t offset = ((column * charWidth) * sizeof(uint32_t)) + ((row * charHeight) * lock.Pitch);

		destPtr = (((uint8_t*)lock.pBits + offset));

		for (uint32_t y = 0; y < charHeight; y++)
		{
			destPtr = (((uint8_t*)lock.pBits + offset) + (y*lock.Pitch));

			for (uint32_t x = 0; x < charWidth; x++)
			{
				if (srcPtr[0] == 0x00 && srcPtr[1] == 0x00 && srcPtr[2] == 0x00)
				{
					*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], 0x00);
				}
				else
				{
					*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], 0xff);
				}

				destPtr += sizeof(uint32_t); // move over an entire 4 byte pixel
				srcPtr += sizeof(uint32_t);
			}
		}
	}

	D3DLOCKED_RECT lock2;
	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, textureUsage, swizzledColourFormat, texturePool, &swizTexture);
	LastError = swizTexture->LockRect(0, &lock2, NULL, NULL );

	XGSwizzleRect(lock.pBits, lock.Pitch, NULL, lock2.pBits, padWidth, padHeight, NULL, sizeof(uint32_t));

	LastError = destTexture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	LastError = swizTexture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// we no longer need this
	destTexture->Release();

	// fill out newTexture struct
	texture.texture = swizTexture;
	texture.bitsPerPixel = 32;
	texture.width  = width;
	texture.height = height;
	texture.realWidth  = padWidth;
	texture.realHeight = padHeight;
	texture.usage = usageType;

	return true;
}

bool R_ReleaseVertexDeclaration(class VertexDeclaration &declaration)
{
/* TODO
	LastError = d3d.lpD3DDevice->DeleteVertexShader(declaration);
	if (FAILED(LastError))
	{
		Con_PrintError("Could not release vertex declaration");
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}
*/
	return true;
}

bool R_CreateVertexShader(const std::string &fileName, r_VertexShader &vertexShader, VertexDeclaration *vertexDeclaration)
{
	XGBuffer* pShaderData = NULL;
	XGBuffer* pErrors = NULL;

	std::string properPath = "d:\\" + fileName;

	// have to open the file ourselves and pass as string. XGAssembleShader won't do this for us
	std::ifstream shaderFile(properPath.c_str(), std::ifstream::in);

	if (!shaderFile.is_open())
	{
		LogErrorString("Error cannot open vertex shader file");
		return false;
	}

	std::stringstream buffer;
	buffer << shaderFile.rdbuf();

	// compile shader from file
	LastError = XGAssembleShader(properPath.c_str(), // filename (for errors only)
			buffer.str().c_str(),
			buffer.str().size(),
			0,
			NULL,
			&pShaderData,
			&pErrors,
			NULL,
			NULL,
			NULL,
			NULL);

	if (FAILED(LastError))
	{
		LogErrorString("XGAssembleShader failed for " + properPath, __LINE__, __FILE__);

		if (pErrors)
		{
			// shader didn't compile for some reason
			LogErrorString("Shader compile errors found for " + properPath, __LINE__, __FILE__);
			LogErrorString("\n" + std::string((const char*)pErrors->GetBufferPointer()));

			pErrors->Release();
		}

		return false;
	}

	LastError = d3d.lpD3DDevice->CreateVertexShader(&vertexDeclaration->d3dElement[0], (const DWORD*)pShaderData->pData, &vertexShader.shader, 0); // ??
	if (FAILED(LastError))
	{
		LogErrorString("CreateVertexShader failed for " + properPath, __LINE__, __FILE__);
		return false;
	}

	// no longer needed
	pShaderData->Release();

	return true;
}

bool R_CreatePixelShader(const std::string &fileName, r_PixelShader &pixelShader)
{
	XGBuffer* pShaderData = NULL;
	XGBuffer* pErrors = NULL;

	std::string properPath = "d:\\" + fileName;

	// have to open the file ourselves and pass as string. XGAssembleShader won't do this for us
	std::ifstream shaderFile(properPath.c_str(), std::ifstream::in);

	if (!shaderFile.is_open())
	{
		LogErrorString("Error cannot open pixel shader file");
		return false;
	}

	std::stringstream buffer;
	buffer << shaderFile.rdbuf();

	// compile shader from file
	LastError = XGAssembleShader(properPath.c_str(), // filename (for errors only)
			buffer.str().c_str(),
			buffer.str().size(),
			0,
			NULL,
			&pShaderData,
			&pErrors,
			NULL,
			NULL,
			NULL,
			NULL);

	if (FAILED(LastError))
	{
		LogErrorString("XGAssembleShader failed for " + properPath, __LINE__, __FILE__);

		if (pErrors)
		{
			// shader didn't compile for some reason
			LogErrorString("Shader compile errors found for " + properPath, __LINE__, __FILE__);
			LogErrorString("\n" + std::string((const char*)pErrors->GetBufferPointer()));

			pErrors->Release();
		}

		return false;
	}

	LastError = d3d.lpD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShaderData->GetBufferPointer(), &pixelShader.shader); // ??
	if (FAILED(LastError))
	{
		LogErrorString("CreatePixelShader failed for " + properPath, __LINE__, __FILE__);
		return false;
	}

	// no longer needed
	pShaderData->Release();

	return true;
}

static BYTE VD_USAGEtoD3DDECLUSAGE(VD_USAGE usage)
{
	BYTE d3dUsage = 0;
/*
	switch (usage)
	{
		case VDUSAGE_POSITION:
			d3dUsage = D3DDECLUSAGE_POSITION;
			break;
		case VDUSAGE_BLENDWEIGHT:
			d3dUsage = D3DDECLUSAGE_BLENDWEIGHT;
			break;
		case VDUSAGE_BLENDINDICES:
			d3dUsage = D3DDECLUSAGE_BLENDINDICES;
			break;
		case VDUSAGE_NORMAL:
			d3dUsage = D3DDECLUSAGE_NORMAL;
			break;
		case VDUSAGE_PSIZE:
			d3dUsage = D3DDECLUSAGE_PSIZE;
			break;
		case VDUSAGE_TEXCOORD:
			d3dUsage = D3DDECLUSAGE_TEXCOORD;
			break;
		case VDUSAGE_TANGENT:
			d3dUsage = D3DDECLUSAGE_TANGENT;
			break;
		case VDUSAGE_BINORMAL:
			d3dUsage = D3DDECLUSAGE_BINORMAL;
			break;
		case VDUSAGE_TESSFACTOR:
			d3dUsage = D3DDECLUSAGE_TESSFACTOR;
			break;
		case VDUSAGE_POSITIONT:
			d3dUsage = D3DDECLUSAGE_POSITIONT;
			break;
		case VDUSAGE_COLOR:
			d3dUsage = D3DDECLUSAGE_COLOR;
			break;
		case VDUSAGE_FOG:
			d3dUsage = D3DDECLUSAGE_FOG;
			break;
		case VDUSAGE_DEPTH:
			d3dUsage = D3DDECLUSAGE_DEPTH;
			break;
		case VDUSAGE_SAMPLE:
			d3dUsage = D3DDECLUSAGE_SAMPLE;
			break;
		default:
			assert (1==0);
			break;
	}
*/
	return d3dUsage;
}

static BYTE VD_TYPEtoD3DDECLTYPE(VD_TYPE type)
{
	BYTE d3dType;

	switch (type)
	{
		case VDTYPE_FLOAT1:
			d3dType = D3DVSDT_FLOAT1;
			break;
		case VDTYPE_FLOAT2:
			d3dType = D3DVSDT_FLOAT2;
			break;
		case VDTYPE_FLOAT3:
			d3dType = D3DVSDT_FLOAT3;
			break;
		case VDTYPE_FLOAT4:
			d3dType = D3DVSDT_FLOAT4;
			break;
		case VDTYPE_COLOR:
			d3dType = D3DVSDT_D3DCOLOR;
			break;
		default:
			assert (1==0);
			break;
	}

	return d3dType;
}

bool R_CreateVertexDeclaration(class VertexDeclaration *vertexDeclaration)
{
	// take a local copy of this to aid in keeping the code below a bit tidier..
	size_t numElements = vertexDeclaration->elements.size();

	vertexDeclaration->d3dElement.resize(numElements+2); // +2 for D3DVSD_STREAM and D3DVSD_END()

	vertexDeclaration->d3dElement[0] = D3DVSD_STREAM(0);

	for (uint32_t i = 1; i <= numElements; i++) // offset first and last values (D3DVSD_STREAM and D3DVSD_END())
	{
		vertexDeclaration->d3dElement[i] = D3DVSD_REG(i-1, VD_TYPEtoD3DDECLTYPE(vertexDeclaration->elements[i-1].type));
	}

	vertexDeclaration->d3dElement[numElements+1] = D3DVSD_END();

	// nothing else to do here, we'll create the vertex declaration in the create vertex shader function
	return true;
}

bool R_SetVertexDeclaration(class VertexDeclaration &declaration)
{
	// not needed?
	return true;
}

bool R_SetVertexShader(r_VertexShader &vertexShader)
{
	LastError = d3d.lpD3DDevice->SetVertexShader(vertexShader.shader);
	if (FAILED(LastError))
	{
		Con_PrintError("Can't set vertex shader " + vertexShader.shaderName);
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_UnsetVertexShader()
{
	LastError = d3d.lpD3DDevice->SetVertexShader(NULL);
	if (FAILED(LastError))
	{
		Con_PrintError("Can't set vertex shader to NULL");
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_SetVertexShaderConstant(r_VertexShader &vertexShader, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData)
{
	// notes: transpose matrixes
	// try SetVertexShaderConstantFast() if we can

	uint32_t numConstants = 0;

	XGMATRIX tempMatrix;

	switch (type)
	{
		case CONST_INT:
			numConstants = 1;
			break;

		case CONST_FLOAT:
			numConstants = 1;
			break;

		case CONST_MATRIX:
			numConstants = 4;

			// we need to transpose the matrix before sending to SetVertexShaderConstant
			memcpy(&tempMatrix, constantData, sizeof(XGMATRIX));
			XGMatrixTranspose(&tempMatrix, &tempMatrix);
			break;

		default:
			LogErrorString("Unknown shader constant type");
			return false;
			break;
	}

	if (type == CONST_MATRIX)
	{
		LastError = d3d.lpD3DDevice->SetVertexShaderConstant(registerIndex, &tempMatrix, numConstants);
	}
	else
	{
		LastError = d3d.lpD3DDevice->SetVertexShaderConstant(registerIndex, constantData, numConstants);
	}

	if (FAILED(LastError))
	{
		Con_PrintError("Can't SetVertexShaderConstant for vertex shader " + vertexShader.shaderName);
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

/*
bool R_SetVertexShaderMatrix(r_VertexShader &vertexShader, const char* constant, R_MATRIX &matrix)
{
	D3DXMATRIX tempMat;
	tempMat._11 = matrix._11;
	tempMat._12 = matrix._12;
	tempMat._13 = matrix._13;
	tempMat._14 = matrix._14;

	tempMat._21 = matrix._21;
	tempMat._22 = matrix._22;
	tempMat._23 = matrix._23;
	tempMat._24 = matrix._24;

	tempMat._31 = matrix._31;
	tempMat._32 = matrix._32;
	tempMat._33 = matrix._33;
	tempMat._34 = matrix._34;

	tempMat._41 = matrix._41;
	tempMat._42 = matrix._42;
	tempMat._43 = matrix._43;
	tempMat._44 = matrix._44;

	// TODO - constant register address lookup
	LastError = d3d.lpD3DDevice->SetVertexShaderConstant(0, &tempMat, 4);
	if (FAILED(LastError))
	{
		Con_PrintError("Can't set matrix for vertex shader " + vertexShader.shaderName);
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}
*/

bool R_SetPixelShader(r_PixelShader &pixelShader)
{
	LastError = d3d.lpD3DDevice->SetPixelShader(pixelShader.shader);
	if (FAILED(LastError))
	{
		Con_PrintError("Can't set pixel shader " + pixelShader.shaderName);
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_UnsetPixelShader()
{
	LastError = d3d.lpD3DDevice->SetPixelShader(NULL);
	if (FAILED(LastError))
	{
		Con_PrintError("Can't set pixel shader to NULL");
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

// removes pure red colour from a texture. used to remove red outline grid on small font texture.
// we remove the grid as it can sometimes bleed onto text when we use texture filtering. maybe add params for passing width/height?
void DeRedTexture(Texture &texture)
{
	uint8_t *srcPtr = NULL;
	uint8_t *destPtr = NULL;
	uint32_t pitch = 0;

	// lock texture
	if (!R_LockTexture(texture, &srcPtr, &pitch, TextureLock_Normal))
	{
		Con_PrintError("DeRedTexture call failed - can't lock texture");
		return;
	}

	// loop, setting all full red pixels to black
	for (uint32_t y = 0; y < texture.height; y++)
	{
		destPtr = (srcPtr + y*pitch);

		for (uint32_t x = 0; x < texture.width; x++)
		{
			if ((destPtr[BO_RED] == 255) && (destPtr[BO_BLUE] == 0) && (destPtr[BO_GREEN] == 0))
			{
				destPtr[BO_RED] = 0; // set to black
			}

			destPtr += sizeof(uint32_t); // move over an entire 4 byte pixel
		}
	}

	R_UnlockTexture(texture);
}

bool R_CreateTextureFromFile(const std::string &fileName, Texture &texture)
{
	D3DXIMAGE_INFO imageInfo;

	// ensure this will be NULL unless we successfully create it
	texture.texture = NULL;

	LastError = D3DXCreateTextureFromFileEx(d3d.lpD3DDevice,
		fileName.c_str(),
		D3DX_DEFAULT,			// width
		D3DX_DEFAULT,			// height
		1,						// mip levels
		0,						// usage
		D3DFMT_UNKNOWN,			// format
		D3DPOOL_MANAGED,
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&imageInfo,
		NULL,
		&texture.texture
		);

	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		texture.texture = NULL;
		return false;
	}

	texture.width  = imageInfo.Width;
	texture.height = imageInfo.Height;
	texture.bitsPerPixel = 32; // this should be ok?
	texture.usage = TextureUsage_Normal;

	return true;
}

bool R_CreateTextureFromAvPTexture(AVPTEXTURE &AvPTexture, enum TextureUsage usageType, Texture &texture)
{
	D3DPOOL texturePool;
	uint32_t textureUsage;
	LPDIRECT3DTEXTURE8 d3dTexture = NULL;
	D3DFORMAT textureFormat = D3DFMT_A8R8G8B8; // default format (swizzled)

	// ensure this will be NULL unless we successfully create it
	texture.texture = NULL;

	switch (usageType)
	{
		case TextureUsage_Normal:
		{
			texturePool  = D3DPOOL_MANAGED;
			textureUsage = 0;
			break;
		}
		case TextureUsage_Dynamic:
		{
			texturePool  = D3DPOOL_DEFAULT;
			textureUsage = 0; //D3DUSAGE_DYNAMIC; - not valid on xbox
			break;
		}
		default:
		{
			Con_PrintError("Invalid texture usageType value in R_CreateTextureFromAvPTexture()");
		}
	}

	// hack to ensure the font texture isn't swizzled (we need to lockRect and update it later. this'll make that easier)
	if (texture.name == "Graphics/Common/aa_font.rim") // file path
	{
		// change to a linear format
//		textureFormat = D3DFMT_LIN_A8R8G8B8;
	}

	// fill tga header
	TgaHeader.idlength = 0;
	TgaHeader.x_origin = AvPTexture.width;
	TgaHeader.y_origin = AvPTexture.height;
	TgaHeader.colourmapdepth	= 0;
	TgaHeader.colourmaplength	= 0;
	TgaHeader.colourmaporigin	= 0;
	TgaHeader.colourmaptype		= 0;
	TgaHeader.datatypecode		= 2;		// RGB
	TgaHeader.bitsperpixel		= 32;
	TgaHeader.imagedescriptor	= 0x20;		// set origin to top left
	TgaHeader.width  = AvPTexture.width;
	TgaHeader.height = AvPTexture.height;

	// size of raw image data
	uint32_t imageSize = AvPTexture.height * AvPTexture.width * sizeof(uint32_t);

	// create new buffer for header and image data
	uint8_t *buffer = new uint8_t[sizeof(TGA_HEADER) + imageSize];

	// copy header and image data to buffer
	memcpy(buffer, &TgaHeader, sizeof(TGA_HEADER));

	uint8_t *imageData = buffer + sizeof(TGA_HEADER);

	// loop, converting RGB to BGR for D3DX function
	for (uint32_t i = 0; i < imageSize; i+=4)
	{
		// ARGB
		// BGR
		imageData[i+2] = AvPTexture.buffer[i];
		imageData[i+1] = AvPTexture.buffer[i+1];
		imageData[i]   = AvPTexture.buffer[i+2];
		imageData[i+3] = AvPTexture.buffer[i+3];
	}

	if (FAILED(D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		AvPTexture.width,
		AvPTexture.height,
		1, // mips
		textureUsage,
		textureFormat,
		texturePool,
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		NULL,
		0,
		&d3dTexture)))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		delete[] buffer;
		texture.texture = NULL;
		return false;
	}

	// check to see if D3D resized our texture
	D3DSURFACE_DESC surfaceDescription;
	d3dTexture->GetLevelDesc(0, &surfaceDescription);

	texture.realWidth  = surfaceDescription.Width;
	texture.realHeight = surfaceDescription.Height;

	// set texture struct members
	texture.bitsPerPixel = 32; // set to 32 for now
	texture.width   = AvPTexture.width;
	texture.height  = AvPTexture.height;
	texture.usage   = usageType;
	texture.texture = d3dTexture;

	delete[] buffer;
	return true;
}

bool R_CreateTexture(uint32_t width, uint32_t height, uint32_t bitsPerPixel, enum TextureUsage usageType, Texture &texture)
{
	// TODO: find out if we should use XGSetTextureHeader() instead?

	D3DPOOL texturePool;
	D3DFORMAT textureFormat;
	uint32_t textureUsage;
	LPDIRECT3DTEXTURE8 d3dTexture = NULL;

	// ensure this will be NULL unless we successfully create it
	texture.texture = NULL;

	switch (usageType)
	{
		case TextureUsage_Normal:
		{
			texturePool  = D3DPOOL_MANAGED;
			textureUsage = 0;
			break;
		}
		case TextureUsage_Dynamic:
		{
			texturePool  = D3DPOOL_DEFAULT;
			textureUsage = D3DUSAGE_DYNAMIC;
			break;
		}
		default:
		{
			Con_PrintError("Invalid texture usageType value in R_CreateTexture()");
			return false;
		}
	}

	switch (bitsPerPixel)
	{
		case 32:
		{
			textureFormat = D3DFMT_LIN_A8R8G8B8;
			break;
		}
		case 16:
		{
			textureFormat = D3DFMT_LIN_A1R5G5B5;
			break;
		}
		case 8:
		{
			textureFormat = D3DFMT_LIN_L8;
			break;
		}
		default:
		{
			Con_PrintError("Invalid bitsPerPixel value in R_CreateTexture()");
			return false;
		}
	}

	// create the d3d9 texture
	LastError = d3d.lpD3DDevice->CreateTexture(width, height, 1, textureUsage, textureFormat, texturePool, &d3dTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	// set texture struct members
	texture.bitsPerPixel = bitsPerPixel;
	texture.width   = width;
	texture.height  = height;
	texture.usage   = usageType;
	texture.texture = d3dTexture;

	return true;
}

void R_ReleaseTexture(Texture &texture)
{
	if (texture.texture)
	{
		texture.texture->Release();
		texture.texture = NULL;
	}
}

void R_ReleaseVertexShader(r_VertexShader &vertexShader)
{
	if (vertexShader.shader)
	{
		d3d.lpD3DDevice->SetVertexShader(0);
		d3d.lpD3DDevice->DeleteVertexShader(vertexShader.shader);
	}
}

void R_ReleasePixelShader(r_PixelShader &pixelShader)
{
	if (pixelShader.shader)
	{
		d3d.lpD3DDevice->SetPixelShader(0);
		d3d.lpD3DDevice->DeletePixelShader(pixelShader.shader);
	}
}

bool R_ChangeResolution(uint32_t width, uint32_t height)
{
	// don't bother resetting device if we're already using the requested settings
	if ((width == d3d.d3dpp.BackBufferWidth) && (height == d3d.d3dpp.BackBufferHeight))
	{
		return true;
	}

	ThisFramesRenderingHasFinished();

	ReleaseVolatileResources();

	d3d.d3dpp.BackBufferWidth = width;
	d3d.d3dpp.BackBufferHeight = height;

	// try reset the device
	LastError = d3d.lpD3DDevice->Reset(&d3d.d3dpp);
	if (FAILED(LastError))
	{
		// log an error message
		std::stringstream sstream;
		sstream << "Can't set resolution " << width << " x " << height << ". Setting default safe values";
		Con_PrintError(sstream.str());

		// this'll occur if the resolution width and height passed aren't usable on this device
		if (D3DERR_INVALIDCALL == LastError)
		{
			// set some default, safe resolution?
			width  = 800;
			height = 600;

			d3d.d3dpp.BackBufferWidth  = width;
			d3d.d3dpp.BackBufferHeight = height;

			// try reset again, if it doesnt work, bail out
			LastError = d3d.lpD3DDevice->Reset(&d3d.d3dpp);
			if (FAILED(LastError))
			{
				LogDxError(LastError, __LINE__, __FILE__);
				return FALSE;
			}
		}
		else
		{
			LogDxError(LastError, __LINE__, __FILE__);
			return FALSE;
		}
	}

	d3d.D3DViewport.Width  = width;
	d3d.D3DViewport.Height = height;

	LastError = d3d.lpD3DDevice->SetViewport(&d3d.D3DViewport);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	ScreenDescriptorBlock.SDB_Width     = width;
	ScreenDescriptorBlock.SDB_Height    = height;
//	ScreenDescriptorBlock.SDB_Depth		= colourDepth;
	ScreenDescriptorBlock.SDB_Size      = width*height;
	ScreenDescriptorBlock.SDB_CentreX   = width/2;
	ScreenDescriptorBlock.SDB_CentreY   = height/2;

	ScreenDescriptorBlock.SDB_ProjX     = width/2;
	ScreenDescriptorBlock.SDB_ProjY     = height/2;

	ScreenDescriptorBlock.SDB_ClipLeft  = 0;
	ScreenDescriptorBlock.SDB_ClipRight = width;
	ScreenDescriptorBlock.SDB_ClipUp    = 0;
	ScreenDescriptorBlock.SDB_ClipDown  = height;

	CreateVolatileResources();

	SetTransforms();

	ThisFramesRenderingHasBegun();

	return true;
}

bool InitialiseDirect3D()
{
	// clear log file first, then write header text
	ClearLog();
	Con_PrintMessage("Starting to initialise Direct3D");

	uint32_t width = 640;
	uint32_t height = 480;
	uint32_t defaultDevice = D3DADAPTER_DEFAULT;
	uint32_t thisDevice = D3DADAPTER_DEFAULT;
	bool useTripleBuffering = Config_GetBool("[VideoMode]", "UseTripleBuffering", false);
	shaderPath = Config_GetString("[VideoMode]", "ShaderPath", "shaders\\");

	d3d.supportsShaders = false;

	//	Zero d3d structure
    memset(&d3d, 0, sizeof(D3DINFO));

	//  Set up Direct3D interface object
	d3d.lpD3D = Direct3DCreate8(D3D_SDK_VERSION);
	if (!d3d.lpD3D)
	{
		Con_PrintError("Could not create Direct3D8 object");
		return false;
	}

/*
	unsigned int modeCount = d3d.lpD3D->GetAdapterModeCount(defaultDevice);

	char buf[100];
	for(int i = 0; i < modeCount; i++)
	{
		d3d.lpD3D->EnumAdapterModes(0, i, &displayMode);
		sprintf(buf, "\n width: %d height: %d", displayMode.Width, displayMode.Height);
		OutputDebugString(buf);
	}
*/
	DWORD videoStandard = XGetVideoStandard();

	switch (videoStandard)
	{
		case XC_VIDEO_STANDARD_NTSC_M:
		{
			Con_PrintMessage("Using video standard XC_VIDEO_STANDARD_NTSC_M");
			break;
		}
		case XC_VIDEO_STANDARD_NTSC_J:
		{
			Con_PrintMessage("Using video standard XC_VIDEO_STANDARD_NTSC_J");
			break;
		}
		case XC_VIDEO_STANDARD_PAL_I:
		{
			Con_PrintMessage("Using video standard XC_VIDEO_STANDARD_PAL_I");
			break;
		}
		default:
		{
			Con_PrintError("Unknown video standard!");
			break;
		}
	}

	DWORD videoFlags = XGetVideoFlags();

	if (videoFlags & XC_VIDEO_FLAGS_WIDESCREEN)
	{
		Con_PrintMessage("Using video setting XC_VIDEO_FLAGS_WIDESCREEN");
	}
	else if (videoFlags & XC_VIDEO_FLAGS_HDTV_720p)
	{
		Con_PrintMessage("Using video setting XC_VIDEO_FLAGS_HDTV_720p");
	}
	else if (videoFlags & XC_VIDEO_FLAGS_HDTV_1080i)
	{
		Con_PrintMessage("Using video setting XC_VIDEO_FLAGS_HDTV_1080i");
	}
	else if (videoFlags & XC_VIDEO_FLAGS_HDTV_480p)
	{
		Con_PrintMessage("Using video setting XC_VIDEO_FLAGS_HDTV_480p");
	}
	else if (videoFlags & XC_VIDEO_FLAGS_LETTERBOX)
	{
		Con_PrintMessage("Using video setting XC_VIDEO_FLAGS_LETTERBOX");
	}
	else if (videoFlags & XC_VIDEO_FLAGS_PAL_60Hz)
	{
		Con_PrintMessage("Using video setting XC_VIDEO_FLAGS_PAL_60Hz");
	}
	else
	{
		Con_PrintError("Unknown video setting!");
	}

	DWORD AVPack = XGetAVPack();

	switch (AVPack)
	{
		case XC_AV_PACK_SCART:
		{
			Con_PrintMessage("Using av pack XC_AV_PACK_SCART");
			break;
		}
		case XC_AV_PACK_HDTV:
		{
			Con_PrintMessage("Using av pack XC_AV_PACK_HDTV");
			break;
		}
		case XC_AV_PACK_RFU:
		{
			Con_PrintMessage("Using av pack XC_AV_PACK_RFU");
			break;
		}
		case XC_AV_PACK_SVIDEO:
		{
			Con_PrintMessage("Using av pack XC_AV_PACK_SVIDEO");
			break;
		}
		case XC_AV_PACK_STANDARD:
		{
			Con_PrintMessage("Using av pack XC_AV_PACK_STANDARD");
			break;
		}
		case XC_AV_PACK_VGA: // this was in the header file but not the docs..
		{
			Con_PrintMessage("Using av pack XC_AV_PACK_VGA");
			break;
		}
	}

	uint32_t modeCount = d3d.lpD3D->GetAdapterModeCount(defaultDevice);
	D3DDISPLAYMODE tempMode;

	// create our list of supported resolutions for D3DFMT_LIN_X8R8G8B8 format
	for (uint32_t i = 0; i < modeCount; i++)
	{
		LastError = d3d.lpD3D->EnumAdapterModes(defaultDevice, i, &tempMode);
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
			continue;
		}

		// we only want D3DFMT_LIN_X8R8G8B8 format
		if (tempMode.Format == D3DFMT_X8R8G8B8) //D3DFMT_LIN_X8R8G8B8)
		{
			int j = 0;
			// check if the mode already exists
			for (; j < d3d.Driver[defaultDevice].NumModes; j++)
			{
				if ((d3d.Driver[defaultDevice].DisplayMode[j].Width  == tempMode.Width) &&
					(d3d.Driver[defaultDevice].DisplayMode[j].Height == tempMode.Height) &&
					(d3d.Driver[defaultDevice].DisplayMode[j].Format == tempMode.Format))
					break;
			}

			// we looped all the way through but didn't break early due to already existing item
			if (j == d3d.Driver[defaultDevice].NumModes)
			{
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[defaultDevice].NumModes].Width       = tempMode.Width;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[defaultDevice].NumModes].Height      = tempMode.Height;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[defaultDevice].NumModes].Format      = tempMode.Format;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[defaultDevice].NumModes].RefreshRate = 0;
				d3d.Driver[defaultDevice].NumModes++;
			}
		}
	}

	// set CurrentVideoMode variable to index display mode that matches user requested settings
	for (uint32_t i = 0; i < d3d.Driver[defaultDevice].NumModes; i++)
	{
		if ((width == d3d.Driver[defaultDevice].DisplayMode[i].Width)
			&&(height == d3d.Driver[defaultDevice].DisplayMode[i].Height))
		{
			d3d.CurrentVideoMode = i;
			break;
		}
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));

	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
//	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
//	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE_OR_IMMEDIATE;

	if (useTripleBuffering)
	{
		d3dpp.BackBufferCount = 2;
		d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
		Con_PrintMessage("Using triple buffering");
	}

	d3d.lpD3D->SetPushBufferSize(524288 * 4, 32768);

	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);

	if (FAILED(LastError))
	{
		LogErrorString("Could not create Direct3D device");
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// store the index of the driver we want to use
//	d3d.CurrentDriver = defaultDevice;

	D3DCAPS8 devCaps;
	d3d.lpD3DDevice->GetDeviceCaps(&devCaps);

	if (devCaps.TextureCaps & D3DPTEXTURECAPS_POW2)
	{
		OutputDebugString("D3DPTEXTURECAPS_POW2\n");
	}

	if (devCaps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		OutputDebugString("D3DPTEXTURECAPS_SQUAREONLY\n");
	}

	if (devCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
	{
		OutputDebugString("D3DPTEXTURECAPS_NONPOW2CONDITIONAL\n");
	}

	// Log resolution set
	Con_PrintMessage("\t Resolution set: " + IntToString(d3dpp.BackBufferWidth) + " x " + IntToString(d3dpp.BackBufferHeight));
/*
	// Log format set
	switch (d3dpp.BackBufferFormat)
	{
		case D3DFMT_A8R8G8B8:
			Con_PrintMessage("\t Format set: 32bit - D3DFMT_A8R8G8B8");
			break;
		case D3DFMT_X8R8G8B8:
			Con_PrintMessage("\t Format set: 32bit - D3DFMT_X8R8G8B8");
			break;
		case D3DFMT_R8G8B8:
			Con_PrintMessage("\t Format set: 32bit - D3DFMT_R8G8B8");
			break;
		case D3DFMT_A1R5G5B5:
			Con_PrintMessage("\t Format set: 16bit - D3DFMT_A1R5G5B5");
			break;
		case D3DFMT_R5G6B5:
			Con_PrintMessage("\t Format set: 16bit - D3DFMT_R5G6B5");
			break;
		case D3DFMT_X1R5G5B5:
			Con_PrintMessage("\t Format set: 16bit - D3DFMT_X1R5G5B5");
			break;
		default:
			Con_PrintMessage("\t Format set: Unknown");
			break;
	}

	// Log depth buffer format set
	switch (d3dpp.AutoDepthStencilFormat)
	{
		case D3DFMT_D24S8:
			Con_PrintMessage("\t Depth Format set: 24bit and 8bit stencil - D3DFMT_D24S8");
			break;
		case D3DFMT_D24X8:
			Con_PrintMessage("\t Depth Format set: 24bit and 0bit stencil - D3DFMT_D24X8");
			break;
		case D3DFMT_D24FS8:
			Con_PrintMessage("\t Depth Format set: 24bit and 8bit stencil - D3DFMT_D32");
			break;
		case D3DFMT_D32:
			Con_PrintMessage("\t Depth Format set: 32bit and 0bit stencil - D3DFMT_D32");
			break;
		default:
			Con_PrintMessage("\t Depth Format set: Unknown");
			break;
	}
*/
	ZeroMemory (&d3d.D3DViewport, sizeof(d3d.D3DViewport));
	d3d.D3DViewport.X = 0;
	d3d.D3DViewport.Y = 0;
	d3d.D3DViewport.Width = width;
	d3d.D3DViewport.Height = height;
	d3d.D3DViewport.MinZ = 0.0f;
	d3d.D3DViewport.MaxZ = 1.0f;

	LastError = d3d.lpD3DDevice->SetViewport(&d3d.D3DViewport);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	ScreenDescriptorBlock.SDB_Width     = width;
	ScreenDescriptorBlock.SDB_Height    = height;
//	ScreenDescriptorBlock.SDB_Depth		= colourDepth;
	ScreenDescriptorBlock.SDB_Size      = width*height;
	ScreenDescriptorBlock.SDB_CentreX   = width/2;
	ScreenDescriptorBlock.SDB_CentreY   = height/2;

	ScreenDescriptorBlock.SDB_ProjX     = width/2;
	ScreenDescriptorBlock.SDB_ProjY     = height/2;

	ScreenDescriptorBlock.SDB_ClipLeft  = 0;
	ScreenDescriptorBlock.SDB_ClipRight = width;
	ScreenDescriptorBlock.SDB_ClipUp    = 0;
	ScreenDescriptorBlock.SDB_ClipDown  = height;

	// use an offset for hud items to account for tv safe zones. just use width for now. 5%?
	uint32_t safeOffset = Config_GetInt("[VideoMode]", "SafeZoneOffset", 5);
	ScreenDescriptorBlock.SDB_SafeZoneWidthOffset  = (width / 100) * safeOffset;
	ScreenDescriptorBlock.SDB_SafeZoneHeightOffset = (height / 100) * safeOffset;

	// save a copy of the presentation parameters for use
	// later (device reset, resolution/depth change)
	d3d.d3dpp = d3dpp;

	// set field of view (this needs to be set differently for alien but 77 seems ok for marine and predator
	d3d.fieldOfView = 77;

	SetTransforms();

	// create vertex declarations
	d3d.mainDecl = new VertexDeclaration;
	d3d.mainDecl->Add(0, VDTYPE_FLOAT3, VDMETHOD_DEFAULT, VDUSAGE_POSITION, 0);
	d3d.mainDecl->Add(0, VDTYPE_COLOR,  VDMETHOD_DEFAULT, VDUSAGE_COLOR,    0);
	d3d.mainDecl->Add(0, VDTYPE_COLOR,  VDMETHOD_DEFAULT, VDUSAGE_COLOR,    1);
	d3d.mainDecl->Add(0, VDTYPE_FLOAT2, VDMETHOD_DEFAULT, VDUSAGE_TEXCOORD, 0);
	d3d.mainDecl->Create();

	d3d.orthoDecl = new VertexDeclaration;
	d3d.orthoDecl->Add(0, VDTYPE_FLOAT3, VDMETHOD_DEFAULT, VDUSAGE_POSITION, 0);
	d3d.orthoDecl->Add(0, VDTYPE_COLOR,  VDMETHOD_DEFAULT, VDUSAGE_COLOR,    0);
	d3d.orthoDecl->Add(0, VDTYPE_FLOAT2, VDMETHOD_DEFAULT, VDUSAGE_TEXCOORD, 0);
	d3d.orthoDecl->Create();

	d3d.fmvDecl = new VertexDeclaration;
	d3d.fmvDecl->Add(0, VDTYPE_FLOAT3, VDMETHOD_DEFAULT, VDUSAGE_POSITION, 0);
	d3d.fmvDecl->Add(0, VDTYPE_FLOAT2, VDMETHOD_DEFAULT, VDUSAGE_TEXCOORD, 0);
	d3d.fmvDecl->Add(0, VDTYPE_FLOAT2, VDMETHOD_DEFAULT, VDUSAGE_TEXCOORD, 1);
	d3d.fmvDecl->Add(0, VDTYPE_FLOAT2, VDMETHOD_DEFAULT, VDUSAGE_TEXCOORD, 2);
	d3d.fmvDecl->Create();

	d3d.tallFontText = new VertexDeclaration;
	d3d.tallFontText->Add(0, VDTYPE_FLOAT3, VDMETHOD_DEFAULT, VDUSAGE_POSITION, 0);
	d3d.tallFontText->Add(0, VDTYPE_COLOR,  VDMETHOD_DEFAULT, VDUSAGE_COLOR,    0);
	d3d.tallFontText->Add(0, VDTYPE_FLOAT2, VDMETHOD_DEFAULT, VDUSAGE_TEXCOORD, 0);
	d3d.tallFontText->Add(0, VDTYPE_FLOAT2, VDMETHOD_DEFAULT, VDUSAGE_TEXCOORD, 1);
	d3d.tallFontText->Create();

	r_Texture whiteTexture;
	r_Texture missingTexture;

	// create a hot pink texture to use when a texture resource can't be loaded
	{
		LastError = d3d.lpD3DDevice->CreateTexture(1, 1, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &missingTexture);
		if (FAILED(LastError))
		{
			Con_PrintError("Could not create missing texture signifying texture");
			LogDxError(LastError, __LINE__, __FILE__);
		}

		D3DLOCKED_RECT lock;
		LastError = missingTexture->LockRect(0, &lock, NULL, 0);
		if (FAILED(LastError))
		{
			Con_PrintError("Could not lock missing texture signifying texture");
			LogDxError(LastError, __LINE__, __FILE__);
		}
		else
		{
			// set pixel to hot pink
			(*(reinterpret_cast<uint32_t*>(lock.pBits))) = D3DCOLOR_XRGB(255, 0, 255);

			missingTexture->UnlockRect(0);

			// should we just add it even if it fails?
			MISSING_TEXTURE = Tex_AddTexture("missing", missingTexture, 1, 1, 32, TextureUsage_Normal);
		}
	}

	{
		// create a 1x1 resolution white texture to set to shader for sampling when we don't want to texture an object (eg what was NULL texture in fixed function pipeline)
		LastError = d3d.lpD3DDevice->CreateTexture(1, 1, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &whiteTexture);
		if (FAILED(LastError))
		{
			Con_PrintError("Could not create white texture for shader sampling");
			LogDxError(LastError, __LINE__, __FILE__);
		}

		D3DLOCKED_RECT lock;
		LastError = whiteTexture->LockRect(0, &lock, NULL, 0);
		if (FAILED(LastError))
		{
			Con_PrintError("Could not lock white texture for shader sampling");
			LogDxError(LastError, __LINE__, __FILE__);
		}
		else
		{
			// set pixel to white
			(*(reinterpret_cast<uint32_t*>(lock.pBits))) = D3DCOLOR_XRGB(255, 255, 255);

			whiteTexture->UnlockRect(0);

			// should we just add it even if it fails?
			NO_TEXTURE = Tex_AddTexture("white", whiteTexture, 1, 1, 32, TextureUsage_Normal);
		}
	}

	setTextureArray.resize(kMaxTextureStages);

/*
	// set all texture stages to sample the white texture
	for (uint32_t i = 0; i < kMaxTextureStages; i++)
	{
		setTextureArray[i] = NO_TEXTURE;
		d3d.lpD3DDevice->SetTexture(i, Tex_GetTexture(NO_TEXTURE));
	}
*/
	d3d.effectSystem = new EffectManager;

	d3d.mainEffect  = d3d.effectSystem->Add("main", "vertex_1_1.vsh", "pixel_1_1.psh", d3d.mainDecl);
	d3d.orthoEffect = d3d.effectSystem->Add("ortho", "orthoVertex_1_1.vsh", "pixel_1_1.psh", d3d.orthoDecl);
//	d3d.fmvEffect   = d3d.effectSystem->Add("fmv", "fmvVertex.vsh", "fmvPixel.psh");
//	d3d.cloudEffect = d3d.effectSystem->Add("cloud", "tallFontTextVertex.vsh", "tallFontTextPixel.psh");

	// create vertex and index buffers
	CreateVolatileResources();

	Con_Init();
	Net_Initialise();
	Font_Init();

	RenderListInit();

	Con_PrintMessage("Initialised Direct3D Xbox succesfully");

	return true;
}

void SetTransforms()
{
	// Setup orthographic projection matrix
	uint32_t standardWidth = 640;
	uint32_t wideScreenWidth = 852;

	// create an identity matrix
	D3DXMatrixIdentity(&matIdentity);

	D3DXMatrixIdentity(&matView);

	// set up orthographic projection matrix
	D3DXMatrixOrthoLH(&matOrtho, 2.0f, -2.0f, 1.0f, 10.0f);

	// set up projection matrix
	D3DXMatrixPerspectiveFovLH(&matProjection, D3DXToRadian(d3d.fieldOfView), (float)ScreenDescriptorBlock.SDB_Width / (float)ScreenDescriptorBlock.SDB_Height, 64.0f, 1000000.0f);

	// set up a viewport transform matrix
	matViewPort = matIdentity;

	matViewPort._11 = (float)(ScreenDescriptorBlock.SDB_Width / 2);
	matViewPort._22 = (float)((-ScreenDescriptorBlock.SDB_Height) / 2);
	matViewPort._33 = (1.0f - 0.0f);
	matViewPort._41 = (0 + matViewPort._11); // dwX + dwWidth / 2
	matViewPort._42 = (float)(ScreenDescriptorBlock.SDB_Height / 2) + 0;
	matViewPort._43 = 0.0f; // minZ
	matViewPort._44 = 1.0f;
}

void FlipBuffers()
{
	LastError = d3d.lpD3DDevice->Present(NULL, NULL, NULL, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}
}

void ReleaseDirect3D()
{
	Font_Release();

	Tex_DeInit();

	// release back-buffer copy surface, vertex buffer and index buffer
	ReleaseVolatileResources();

	// release vertex declarations
	delete d3d.mainDecl;
	delete d3d.orthoDecl;
	delete d3d.fmvDecl;
	delete d3d.tallFontText;

	// clean up render list classes
	RenderListDeInit();

	delete d3d.effectSystem;

	// release device
	SAFE_RELEASE(d3d.lpD3DDevice);
	LogString("Releasing Direct3D8 device...");

	// release object
	SAFE_RELEASE(d3d.lpD3D);
	LogString("Releasing Direct3D8 object...");

	// release Direct Input stuff
	ReleaseDirectKeyboard();
	ReleaseDirectMouse();
	ReleaseDirectInput();

	// find a better place to put this
	Config_Save();
}

void ReleaseAvPTexture(AVPTEXTURE *texture)
{
	delete[] texture->buffer;
	delete texture;
}

void ChangeTranslucencyMode(enum TRANSLUCENCY_TYPE translucencyRequired)
{
	if (CurrentRenderStates.TranslucencyMode == translucencyRequired)
		return;

	CurrentRenderStates.TranslucencyMode = translucencyRequired;

	switch (CurrentRenderStates.TranslucencyMode)
	{
	 	case TRANSLUCENCY_OFF:
		{
			if (TRIPTASTIC_CHEATMODE || MOTIONBLUR_CHEATMODE)
			{
				if (D3DAlphaBlendEnable != TRUE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					D3DAlphaBlendEnable = TRUE;
				}
				if (D3DSrcBlend != D3DBLEND_INVSRCALPHA)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVSRCALPHA);
					D3DSrcBlend = D3DBLEND_INVSRCALPHA;
				}
				if (D3DDestBlend != D3DBLEND_SRCALPHA)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
					D3DDestBlend = D3DBLEND_SRCALPHA;
				}
			}
			else
			{
				if (D3DAlphaBlendEnable != FALSE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
					D3DAlphaBlendEnable = FALSE;
				}
				if (D3DSrcBlend != D3DBLEND_ONE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
					D3DSrcBlend = D3DBLEND_ONE;
				}
				if (D3DDestBlend != D3DBLEND_ZERO)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
					D3DDestBlend = D3DBLEND_ZERO;
				}
			}
			break;
		}
	 	case TRANSLUCENCY_NORMAL:
		{
			if (D3DAlphaBlendEnable != TRUE)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3DAlphaBlendEnable = TRUE;
			}
			if (D3DSrcBlend != D3DBLEND_SRCALPHA)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				D3DSrcBlend = D3DBLEND_SRCALPHA;
			}
			if (D3DDestBlend != D3DBLEND_INVSRCALPHA)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
				D3DDestBlend = D3DBLEND_INVSRCALPHA;
			}
			break;
		}
	 	case TRANSLUCENCY_COLOUR:
		{
			if (D3DAlphaBlendEnable != TRUE)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3DAlphaBlendEnable = TRUE;
			}
			if (D3DSrcBlend != D3DBLEND_ZERO)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
				D3DSrcBlend = D3DBLEND_ZERO;
			}
			if (D3DDestBlend != D3DBLEND_SRCCOLOR)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR);
				D3DDestBlend = D3DBLEND_SRCCOLOR;
			}
			break;
		}
	 	case TRANSLUCENCY_INVCOLOUR:
		{
			if (D3DAlphaBlendEnable != TRUE)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3DAlphaBlendEnable = TRUE;
			}
			if (D3DSrcBlend != D3DBLEND_ZERO)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
				D3DSrcBlend = D3DBLEND_ZERO;
			}
			if (D3DDestBlend != D3DBLEND_INVSRCCOLOR)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCCOLOR);
				D3DDestBlend = D3DBLEND_INVSRCCOLOR;
			}
			break;
		}
  		case TRANSLUCENCY_GLOWING:
		{
			if (D3DAlphaBlendEnable != TRUE)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3DAlphaBlendEnable = TRUE;
			}
			if (D3DSrcBlend != D3DBLEND_SRCALPHA)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				D3DSrcBlend = D3DBLEND_SRCALPHA;
			}
			if (D3DDestBlend != D3DBLEND_ONE)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				D3DDestBlend = D3DBLEND_ONE;
			}
			break;
		}
  		case TRANSLUCENCY_DARKENINGCOLOUR:
		{
			if (D3DAlphaBlendEnable != TRUE)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3DAlphaBlendEnable = TRUE;
			}
			if (D3DSrcBlend != D3DBLEND_INVDESTCOLOR)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_INVDESTCOLOR);
				D3DSrcBlend = D3DBLEND_INVDESTCOLOR;
			}

			if (D3DDestBlend != D3DBLEND_ZERO)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
				D3DDestBlend = D3DBLEND_ZERO;
			}
			break;
		}
		case TRANSLUCENCY_JUSTSETZ:
		{
			if (D3DAlphaBlendEnable != TRUE)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
				D3DAlphaBlendEnable = TRUE;
			}
			if (D3DSrcBlend != D3DBLEND_ZERO)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
				D3DSrcBlend = D3DBLEND_ZERO;
			}
			if (D3DDestBlend != D3DBLEND_ONE)
			{
				d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				D3DDestBlend = D3DBLEND_ONE;
			}
		}
		default: break;
	}
}

void ChangeZWriteEnable(enum ZWRITE_ENABLE zWriteEnable)
{
	if (CurrentRenderStates.ZWriteEnable == zWriteEnable)
		return;

	if (zWriteEnable == ZWRITE_ENABLED)
	{
		LastError = d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DRS_ZWRITEENABLE D3DZB_TRUE failed\n");
		}
	}
	else if (zWriteEnable == ZWRITE_DISABLED)
	{
		LastError = d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DRS_ZWRITEENABLE D3DZB_FALSE failed\n");
			OutputDebugString("DISABLING Z WRITES\n");
		}
	}
	else
	{
		LOCALASSERT("Unrecognized ZWriteEnable mode"==0);
	}

	CurrentRenderStates.ZWriteEnable = zWriteEnable;
}

void ChangeTextureAddressMode(uint32_t samplerIndex, enum TEXTURE_ADDRESS_MODE textureAddressMode)
{
	if (CurrentRenderStates.TextureAddressMode == textureAddressMode)
		return;

	CurrentRenderStates.TextureAddressMode = textureAddressMode;

	if (textureAddressMode == TEXTURE_WRAP)
	{
		// wrap texture addresses
		LastError = d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSU Wrap fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSV Wrap fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSW Wrap fail");
		}
	}
	else if (textureAddressMode == TEXTURE_CLAMP)
	{
		// clamp texture addresses
		LastError = d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSU Clamp fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSV Clamp fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSW Clamp fail");
		}
	}
}

void ChangeFilteringMode(uint32_t samplerIndex, enum FILTERING_MODE_ID filteringRequired)
{
	if (CurrentRenderStates.FilteringMode == filteringRequired)
		return;

	CurrentRenderStates.FilteringMode = filteringRequired;

	switch (CurrentRenderStates.FilteringMode)
	{
		case FILTERING_BILINEAR_OFF:
		{
			d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_MAGFILTER, D3DTEXF_POINT);
			d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_MINFILTER, D3DTEXF_POINT);
			break;
		}
		case FILTERING_BILINEAR_ON:
		{
			d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
			d3d.lpD3DDevice->SetTextureStageState(samplerIndex, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
			break;
		}
		default:
		{
			LOCALASSERT("Unrecognized filtering mode"==0);
			OutputDebugString("Unrecognized filtering mode");
			break;
		}
	}
}

void ToggleWireframe()
{
	if (CurrentRenderStates.WireFrameModeIsOn)
	{
		CheckWireFrameMode(0);
	}
	else
	{
		CheckWireFrameMode(1);
	}
}

bool SetRenderStateDefaults()
{
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
//	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, 8);
/*
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP,	D3DTOP_MODULATE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1,	D3DTA_TEXTURE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2,	D3DTA_DIFFUSE);

	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP,	D3DTOP_MODULATE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2,	D3DTA_DIFFUSE);

	d3d.lpD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP,	D3DTOP_DISABLE);
	d3d.lpD3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP,	D3DTOP_DISABLE);

	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);
*/
//	float alphaRef = 0.5f;
//	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAREF,	 *((DWORD*)&alphaRef));//(DWORD)0.5);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAREF,	 (DWORD)0.5);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_OCCLUSIONCULLENABLE, FALSE);

	d3d.lpD3DDevice->SetRenderState(D3DRS_CULLMODE,			D3DCULL_NONE);
//	d3d.lpD3DDevice->SetRenderState(D3DRS_CLIPPING,			TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_LIGHTING,			FALSE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_SPECULARENABLE,	TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_DITHERENABLE,		FALSE);

	{
		// set transparency to TRANSLUCENCY_OFF
		d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		D3DAlphaBlendEnable = FALSE;

		d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
		D3DSrcBlend = D3DBLEND_ONE;

		d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
		D3DDestBlend = D3DBLEND_ZERO;

		// make sure render state tracking reflects above setting
		CurrentRenderStates.TranslucencyMode = TRANSLUCENCY_OFF;
	}

	{
		// enable bilinear filtering (FILTERING_BILINEAR_ON)
		d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
		d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);

		// make sure render state tracking reflects above setting
		CurrentRenderStates.FilteringMode = FILTERING_BILINEAR_ON;
	}

	{
		// set texture addressing mode to clamp (TEXTURE_CLAMP)
		d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
		d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
		d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);

		// make sure render state tracking reflects above setting
		CurrentRenderStates.TextureAddressMode = TEXTURE_CLAMP;
	}

	{
		// enable z-buffer
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

		// enable z writes (already on by default)
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

		// make sure render state tracking reflects above setting
		CurrentRenderStates.ZWriteEnable = ZWRITE_ENABLED;

		// set less + equal z buffer test
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		D3DZFunc = D3DCMP_LESSEQUAL;
	}

	return true;
}
