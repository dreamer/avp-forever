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


extern int NumberOfFMVTextures;
#define MAX_NO_FMVTEXTURES 10
extern FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];

D3DXMATRIX matOrtho;
D3DXMATRIX matOrtho2;
D3DXMATRIX matProjection;
D3DXMATRIX matView;
D3DXMATRIX matIdentity;
D3DXMATRIX matViewPort;

// vertex declarations
DWORD declMain[] =
{
	D3DVSD_STREAM(0),
		D3DVSD_REG(0, D3DVSDT_FLOAT3),    // position
		D3DVSD_REG(1, D3DVSDT_D3DCOLOR),  // diffuse color
		D3DVSD_REG(2, D3DVSDT_D3DCOLOR),  // specular color
		D3DVSD_REG(3, D3DVSDT_FLOAT2),    // texture coordiantes
	D3DVSD_END()
};

DWORD declOrtho[] =
{
	D3DVSD_STREAM(0),
		D3DVSD_REG(0, D3DVSDT_FLOAT3),    // position
		D3DVSD_REG(1, D3DVSDT_D3DCOLOR),  // diffuse color
		D3DVSD_REG(2, D3DVSDT_FLOAT2),    // texture coordiantes
	D3DVSD_END()
};

DWORD declFMV[] =
{
	D3DVSD_STREAM(0),
		D3DVSD_REG(0, D3DVSDT_FLOAT3),    // position
		D3DVSD_REG(1, D3DVSDT_FLOAT2),    // texture coordiantes 1
		D3DVSD_REG(2, D3DVSDT_FLOAT2),    // texture coordiantes 2
		D3DVSD_REG(3, D3DVSDT_FLOAT2),    // texture coordiantes 3
	D3DVSD_END()
};

DWORD declTallFontText[] =
{
	D3DVSD_STREAM(0),
		D3DVSD_REG(0, D3DVSDT_FLOAT3),    // position
		D3DVSD_REG(1, D3DVSDT_D3DCOLOR),  // colour
		D3DVSD_REG(2, D3DVSDT_FLOAT2),    // texture coordiantes 1
		D3DVSD_REG(3, D3DVSDT_FLOAT2),    // texture coordiantes 2
	D3DVSD_END()
};

extern void RenderListInit();
extern void RenderListDeInit();

// size of vertex and index buffers
const uint32_t MAX_VERTEXES = 4096;
const uint32_t MAX_INDICES = 9216;
uint32_t fov = 75;

static HRESULT LastError;
uint32_t NO_TEXTURE;

// keep track of set render states
static bool	D3DAlphaBlendEnable;
D3DBLEND D3DSrcBlend;
D3DBLEND D3DDestBlend;
RENDERSTATES CurrentRenderStates;
bool D3DAlphaTestEnable = FALSE;
static bool D3DStencilEnable;
D3DCMPFUNC D3DStencilFunc;
static D3DCMPFUNC D3DZFunc;
bool ZWritesEnabled = false;

int	VideoModeColourDepth;
int	NumAvailableVideoModes;
D3DINFO d3d;
bool usingStencil = false;

std::string shaderPath;

bool CreateVolatileResources();
bool ReleaseVolatileResources();
bool SetRenderStateDefaults();
void ToggleWireframe();

const int MAX_TEXTURE_STAGES = 8;
std::vector<uint32_t> setTextureArray;

std::string videoModeDescription;

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

bool ReleaseVolatileResources()
{
	Tex_ReleaseDynamicTextures();

	d3d.particleVB->Release();
	d3d.particleIB->Release();

	d3d.mainVB->Release();
	d3d.mainIB->Release();

	d3d.orthoVB->Release();
	d3d.orthoIB->Release();

	return true;
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

bool R_CreateVertexBuffer(uint32_t size, uint32_t usage, r_VertexBuffer **vertexBuffer)
{
	// TODO - how to handle dynamic VBs? thinking of new-ing some space and using DrawPrimitiveUP functions?

	D3DPOOL vbPool;
	DWORD	vbUsage;

	switch (usage)
	{
		case USAGE_STATIC:
			vbUsage = 0;
			vbPool = D3DPOOL_MANAGED;
			break;
		case USAGE_DYNAMIC:
			vbUsage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
			vbPool = D3DPOOL_DEFAULT;
			break;
		default:
			// error and return
			break;
	}

	if (USAGE_STATIC == usage)
	{

	}
	else if (USAGE_DYNAMIC == usage)
	{

	}
/*
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(size, vbUsage, 0, vbPool, vertexBuffer);
	if (FAILED(LastError))
	{
		Con_PrintError("Can't create vertex buffer");
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}
*/
	return true;
}

bool R_ReleaseVertexBuffer(r_VertexBuffer *vertexBuffer)
{
	return true;
}

bool R_CreateIndexBuffer(uint32_t size, uint32_t usage, r_IndexBuffer **indexBuffer)
{
	return true;
}

bool R_ReleaseIndexBuffer(r_IndexBuffer *indexBuffer)
{
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
	return d3d.Driver[d3d.CurrentDriver].AdapterInfo.Description;
}

void R_SetCurrentVideoMode()
{
	uint32_t currentWidth = d3d.Driver[d3d.CurrentDriver].DisplayMode[d3d.CurrentVideoMode].Width;
	uint32_t currentHeight = d3d.Driver[d3d.CurrentDriver].DisplayMode[d3d.CurrentVideoMode].Height;

	// set the new values in the config file
	Config_SetInt("[VideoMode]", "Width" , currentWidth);
	Config_SetInt("[VideoMode]", "Height", currentHeight);

	// save the changes to the config file
	Config_Save();

	// and actually change the resolution on the device
	R_ChangeResolution(currentWidth, currentHeight);
}

bool R_SetTexture(uint32_t stage, uint32_t textureID)
{
	// check that the stage value is within range
	if (stage > MAX_TEXTURE_STAGES-1)
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

bool R_DrawPrimitive(uint32_t numPrimitives)
{
	return true;
}

bool R_LockVertexBuffer(r_VertexBuffer *vertexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, void **data, enum R_USAGE usage)
{
	return true;
}

bool R_DrawIndexedPrimitive(uint32_t numVerts, uint32_t startIndex, uint32_t numPrimitives)
{
	return true;
}

bool R_LockIndexBuffer(r_IndexBuffer *indexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, uint16_t **data, enum R_USAGE usage)
{
	return true;
}

bool R_SetVertexBuffer(r_VertexBuffer *vertexBuffer, uint32_t FVFsize)
{
	return true;
}

bool R_SetIndexBuffer(r_IndexBuffer *indexBuffer)
{
	return true;
}

bool R_UnlockVertexBuffer(r_VertexBuffer *vertexBuffer)
{
	return true;
}

bool R_UnlockIndexBuffer(r_IndexBuffer *indexBuffer)
{
	return true;
}

bool CreateVolatileResources()
{
	Tex_ReloadDynamicTextures();

	// test vertex buffer
	d3d.particleVB = new VertexBuffer;
	d3d.particleVB->Create(MAX_VERTEXES, FVF_LVERTEX, USAGE_DYNAMIC);

	d3d.particleIB = new IndexBuffer;
	d3d.particleIB->Create(MAX_INDICES, USAGE_DYNAMIC);

	// test main
	d3d.mainVB = new VertexBuffer;
	d3d.mainVB->Create(MAX_VERTEXES*5, FVF_LVERTEX, USAGE_DYNAMIC);

	d3d.mainIB = new IndexBuffer;
	d3d.mainIB->Create(MAX_INDICES*5, USAGE_DYNAMIC);

	// ortho test
	d3d.orthoVB = new VertexBuffer;
	d3d.orthoIB = new IndexBuffer;

	d3d.orthoVB->Create(MAX_VERTEXES, FVF_ORTHO, USAGE_DYNAMIC);
	d3d.orthoIB->Create(MAX_INDICES, USAGE_DYNAMIC);

	SetRenderStateDefaults();

	return true;
}

bool R_LockTexture(r_Texture texture, uint8_t **data, uint32_t *pitch, enum TextureLock lockType)
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

	LastError = texture->LockRect(0, &lock, NULL, lockFlag);

	if (FAILED(LastError))
	{
		*data = 0;
		*pitch = 0;
		return false;
	}
	else
	{
		*data = static_cast<uint8_t*>(lock.pBits);
		*pitch = lock.Pitch;
		return true;
	}
}

bool R_UnlockTexture(r_Texture texture)
{
	LastError = texture->UnlockRect(0);

	if (FAILED(LastError))
	{
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

	fov = StringToInt(Con_GetArgument(0));

	SetTransforms();
}

extern "C"
{
	extern void ThisFramesRenderingHasBegun(void);
	extern void ThisFramesRenderingHasFinished(void);

	// Functions
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

	void FlushD3DZBuffer()
	{
		d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
	}
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

#if 0
// list of allowed display formats
const D3DFORMAT DisplayFormats[] =
{
	D3DFMT_X8R8G8B8,
	D3DFMT_A8R8G8B8,
	D3DFMT_R8G8B8/*,
	D3DFMT_A1R5G5B5,
	D3DFMT_X1R5G5B5,
	D3DFMT_R5G6B5*/
};

// 16 bit formats
const D3DFORMAT DisplayFormats16[] =
{
	D3DFMT_R8G8B8/*,
	D3DFMT_A1R5G5B5,
	D3DFMT_X1R5G5B5,
	D3DFMT_R5G6B5*/
};

// 32 bit formats
const D3DFORMAT DisplayFormats32[] =
{
	D3DFMT_X8R8G8B8,
	D3DFMT_A8R8G8B8
};

// list of allowed depth buffer formats
const D3DFORMAT DepthFormats[] =
{
	D3DFMT_D32,
	D3DFMT_D24S8,
	D3DFMT_D24X8,
	D3DFMT_D24FS8,
	D3DFMT_D16
};
#endif

// called from Scrshot.cpp
void CreateScreenShotImage()
{
}

/*
 * This function is responsible for creating the large sized font texture, as used in the game menu system. Originally this
 * texture was only handled by DirectDraw, and has a resolution of 30 x 7392, which cannot be loaded as a standard Direct3D texture.
 * The original image (IntroFont.RIM) is a bitmap font, containing one letter per row (width of 30px). The below function takes this
 * bitmap font and rearranges it into a square texture, now containing more letters per row (as a standard bitmap font would be)
 * which can be used by Direct3D without issue.
 */
r_Texture CreateD3DTallFontTexture(AVPTEXTURE *tex)
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;
	LPDIRECT3DTEXTURE8 swizTexture = NULL;
	D3DLOCKED_RECT	   lock;

	// default colour format
	D3DFORMAT colourFormat = D3DFMT_LIN_A8R8G8B8;

	uint32_t height = 495;
	uint32_t width = 450;

	uint32_t padWidth = 512;
	uint32_t padHeight = 512;

	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, NULL, colourFormat, D3DPOOL_MANAGED, &destTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if (FAILED(LastError))
	{
		destTexture->Release();
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	uint8_t *destPtr, *srcPtr;

	srcPtr = (uint8_t*)tex->buffer;

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

	uint32_t charWidth = 30;
	uint32_t charHeight = 33;

	for (uint32_t i = 0; i < 224; i++)
	{
		int row = i / 15; // get row
		int column = i % 15; // get column from remainder value

		int offset = ((column * charWidth) * sizeof(uint32_t)) + ((row * charHeight) * lock.Pitch);

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
	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &swizTexture);
	LastError = swizTexture->LockRect(0, &lock2, NULL, NULL );

	XGSwizzleRect(lock.pBits, lock.Pitch, NULL, lock2.pBits, padWidth, padHeight, NULL, sizeof(uint32_t));

	LastError = destTexture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	LastError = swizTexture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	destTexture->Release();

	//	return destTexture;
	return swizTexture;
}

bool R_CreateVertexShader(const std::string &fileName, vertexShader_t &vertexShader)
{
	return true;// CreateVertexShader(fileName, &vertexShader.vertexShader, &vertexShader.constantTable);
}

bool R_CreatePixelShader(const std::string &fileName, pixelShader_t &pixelShader)
{
	return true;// CreatePixelShader(fileName, &pixelShader.pixelShader);
}

bool R_SetVertexShader(vertexShader_t &vertexShader)
{
	return true;
}

bool R_SetPixelShader(pixelShader_t &pixelShader)
{
	return true;
}

// removes pure red colour from a texture. used to remove red outline grid on small font texture.
// we remove the grid as it can sometimes bleed onto text when we use texture filtering. maybe add params for passing width/height?
void DeRedTexture(r_Texture texture)
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
	for (uint32_t y = 0; y < 256; y++)
	{
		destPtr = (srcPtr + y*pitch);

		for (uint32_t x = 0; x < 256; x++)
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

// use this to make textures from non power of two images
r_Texture CreateD3DTexturePadded(AVPTEXTURE *tex, uint32_t *realWidth, uint32_t *realHeight)
{
	if (tex == NULL)
	{
		*realWidth = 0;
		*realHeight = 0;
		return NULL;
	}

	uint32_t originalWidth = tex->width;
	uint32_t originalHeight = tex->height;
	uint32_t newWidth = originalWidth;
	uint32_t newHeight = originalHeight;

	// check if passed value is already a power of 2
	if (!IsPowerOf2(tex->width))
	{
		newWidth = NearestSuperiorPow2(tex->width);
	}
	else { newWidth = originalWidth; }

	if (!IsPowerOf2(tex->height))
	{
		newHeight = NearestSuperiorPow2(tex->height);
	}
	else { newHeight = originalHeight; }

//	(*realHeight) = new_height;
//	(*realWidth) = new_width;

	r_Texture destTexture = NULL;

	D3DXIMAGE_INFO image;
	image.Depth = 32;
	image.Width = tex->width;
	image.Height = tex->height;
	image.MipLevels = 1;
	image.Depth = D3DFMT_A8R8G8B8;

	D3DPOOL poolType = D3DPOOL_MANAGED;

	// fill tga header
	TgaHeader.idlength = 0;
	TgaHeader.x_origin = tex->width;
	TgaHeader.y_origin = tex->height;
	TgaHeader.colourmapdepth  = 0;
	TgaHeader.colourmaplength = 0;
	TgaHeader.colourmaporigin = 0;
	TgaHeader.colourmaptype   = 0;
	TgaHeader.datatypecode    = 2;			// RGB
	TgaHeader.bitsperpixel    = 32;
	TgaHeader.imagedescriptor = 0x20;		// set origin to top left
	TgaHeader.height = tex->height;
	TgaHeader.width  = tex->width;

	// size of raw image data
	uint32_t imageSize = tex->height * tex->width * sizeof(uint32_t);

	// create new buffer for header and image data
	uint8_t *buffer = new uint8_t[sizeof(TGA_HEADER) + imageSize];

	// copy header and image data to buffer
	memcpy(buffer, &TgaHeader, sizeof(TGA_HEADER));

	uint8_t *imageData = buffer + sizeof(TGA_HEADER);

	// loop, converting RGB to BGR for D3DX function
	for (uint32_t i = 0; i < imageSize; i+=4)
	{
		// BGRA			 // RGBA
		imageData[i+2] = tex->buffer[i];
		imageData[i+1] = tex->buffer[i+1];
		imageData[i]   = tex->buffer[i+2];
		imageData[i+3] = tex->buffer[i+3];
	}

	if (FAILED(D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		D3DX_DEFAULT,//tex->width,
		D3DX_DEFAULT,//tex->height,
		1,
		0,
		D3DFMT_A8R8G8B8,
		poolType,
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&image,
		0,
		&destTexture)))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		delete[] buffer;
		return NULL;
	}

	delete[] buffer;

	return destTexture;
}

bool R_CreateTextureFromFile(const std::string &fileName, Texture &texture)
{
	return true;
}

bool R_CreateTextureFromAvPTexture(AVPTEXTURE &AvPTexture, enum TextureUsage usageType, Texture &texture)
{
	D3DPOOL texturePool;
	uint32_t textureUsage;
	LPDIRECT3DTEXTURE8 d3dTexture = NULL;

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
			textureUsage = D3DUSAGE_DYNAMIC;
			break;
		}
		default:
		{
			OutputDebugString("uh oh!\n");
		}
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
	TgaHeader.width = AvPTexture.width;
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

	D3DXIMAGE_INFO image;
	image.Depth = 32;
	image.Width = AvPTexture.width;
	image.Height = AvPTexture.height;
	image.MipLevels = 1;
	image.Depth = D3DFMT_A8R8G8B8;

	if (FAILED(D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		AvPTexture.width,
		AvPTexture.height,
		1, // mips
		textureUsage,
		D3DFMT_A8R8G8B8,
		texturePool,
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&image,
		0,
		&d3dTexture)))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		delete[] buffer;
		texture.texture = NULL;
		return false;
	}

	// set texture struct members
	texture.bitsPerPixel = 32; // set to 32 for now
	texture.width = AvPTexture.width;
	texture.height = AvPTexture.height;
	texture.usage = usageType;
	texture.texture = d3dTexture;

	delete[] buffer;
	return true;
}

bool R_CreateTexture(uint32_t width, uint32_t height, uint32_t bpp, enum TextureUsage usageType, Texture &texture)
{
	D3DPOOL texturePool;
	D3DFORMAT textureFormat;
	uint32_t textureUsage;
	LPDIRECT3DTEXTURE8 d3dTexture = NULL;

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
			textureUsage = D3DUSAGE_DYNAMIC;
			break;
		}
		default:
		{
			OutputDebugString("uh oh!\n");
		}
	}

	switch (bpp)
	{
		case 32:
		{
			textureFormat = D3DFMT_A8R8G8B8;
			break;
		}
		case 16:
		{
			textureFormat = D3DFMT_A1R5G5B5;
			break;
		}
		case 8:
		{
			textureFormat = D3DFMT_L8;
			break;
		}
	}

	// create the d3d9 texture
	LastError = d3d.lpD3DDevice->CreateTexture(width, height, 1, textureUsage, textureFormat, texturePool, &d3dTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	// set texture struct members
	texture.bitsPerPixel = bpp;
	texture.width = width;
	texture.height = height;
	texture.usage = usageType;
	texture.texture = d3dTexture;

	return true;
}

void R_ReleaseTexture(r_Texture &texture)
{
	if (texture)
	{
		texture->Release();
		texture = NULL;
	}
}

void R_ReleaseVertexShader(r_VertexShader &vertexShader)
{
/*
	if (vertexShader)
	{
		vertexShader->Release();
		vertexShader = 0;
	}
*/
}

void R_ReleasePixelShader(r_PixelShader &pixelShader)
{
/*
	if (pixelShader)
	{
		pixelShader->Release();
		pixelShader = 0;
	}
*/
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
			width = 800;
			height = 600;

			d3d.d3dpp.BackBufferWidth = width;
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

	d3d.D3DViewport.Width = width;
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

//	D3DDISPLAYMODE d3ddm;
//	LastError = d3d.lpD3D->GetAdapterDisplayMode(defaultDevice, &d3ddm);

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
		if (tempMode.Format == D3DFMT_LIN_X8R8G8B8)
		{
			int j = 0;
			// check if the mode already exists
			for (; j < d3d.Driver[thisDevice].NumModes; j++)
			{
				if ((d3d.Driver[defaultDevice].DisplayMode[j].Width == tempMode.Width) &&
					(d3d.Driver[defaultDevice].DisplayMode[j].Height == tempMode.Height) &&
					(d3d.Driver[defaultDevice].DisplayMode[j].Format == tempMode.Format))
					break;
			}

			// we looped all the way through but didn't break early due to already existing item
			if (j == d3d.Driver[thisDevice].NumModes)
			{
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Width       = tempMode.Width;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Height      = tempMode.Height;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Format      = tempMode.Format;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].RefreshRate = 0;
				d3d.Driver[thisDevice].NumModes++;
			}
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

	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);

	if (FAILED(LastError))
	{
		LogErrorString("Could not create Direct3D device");
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

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
	int safeOffset = Config_GetInt("[VideoMode]", "SafeZoneOffset", 5);
	ScreenDescriptorBlock.SDB_SafeZoneWidthOffset = (width / 100) * safeOffset;
	ScreenDescriptorBlock.SDB_SafeZoneHeightOffset = (height / 100) * safeOffset;

	// save a copy of the presentation parameters for use
	// later (device reset, resolution/depth change)
	d3d.d3dpp = d3dpp;

	// create vertex and index buffers
	CreateVolatileResources();

//	mainVertex = new D3DLVERTEX[4096];
//	mainIndex = new WORD[9216 * 3];

	SetTransforms();

	CreateVertexShader("vertex_1_1.vsh", &d3d.vertexShader, mainDecl);
	CreateVertexShader("orthoVertex_1_1.vsh", &d3d.orthoVertexShader, orthoDecl);

	CreatePixelShader("pixel_1_1.psh", &d3d.pixelShader);

	r_Texture whiteTexture;

	// create a 1x1 resolution white texture to set to shader for sampling when we don't want to texture an object (eg what was NULL texture in fixed function pipeline)
	LastError = d3d.lpD3DDevice->CreateTexture(1, 1, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &whiteTexture);
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
		memset(lock.pBits, 255, lock.Pitch);

		whiteTexture->UnlockRect(0);

		// should we just add it even if it fails?
		NO_TEXTURE = Tex_AddTexture("white", whiteTexture, 1, 1);
	}

	setTextureArray.resize(MAX_TEXTURE_STAGES);

	// set all texture stages to sample the white texture
	for (uint32_t i = 0; i < MAX_TEXTURE_STAGES; i++)
	{
		setTextureArray[i] = NO_TEXTURE;
		d3d.lpD3DDevice->SetTexture(i, Tex_GetTexture(NO_TEXTURE));
	}

	Con_Init();
	Net_Initialise();
	Font_Init();

	RenderListInit();

	Con_PrintMessage("Initialised Direct3D Xbox succesfully");

	return true;
}

void R_UpdateViewMatrix(float *viewMat)
{

}

void SetTransforms()
{
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

	// release constant tables
//	SAFE_RELEASE(vertexConstantTable);
//	SAFE_RELEASE(orthoConstantTable);
//	SAFE_RELEASE(fmvConstantTable);
//	SAFE_RELEASE(cloudConstantTable);

	// release vertex declarations
//	SAFE_RELEASE(d3d.vertexDecl);
//	SAFE_RELEASE(d3d.orthoVertexDecl);
//	SAFE_RELEASE(d3d.fmvVertexDecl);
//	SAFE_RELEASE(d3d.cloudVertexDecl);

	// release pixel shaders
//	SAFE_RELEASE(d3d.pixelShader);
//	SAFE_RELEASE(d3d.fmvPixelShader);
//	SAFE_RELEASE(d3d.cloudPixelShader);

	d3d.lpD3DDevice->SetPixelShader(0);
	d3d.lpD3DDevice->DeletePixelShader(d3d.vertexShader);
	d3d.lpD3DDevice->DeletePixelShader(d3d.pixelShader);
	d3d.lpD3DDevice->DeletePixelShader(d3d.fmvPixelShader);
	d3d.lpD3DDevice->DeletePixelShader(d3d.cloudPixelShader);

	// release vertex shaders
	d3d.lpD3DDevice->SetVertexShader(0);
	d3d.lpD3DDevice->DeleteVertexShader(d3d.vertexShader);
	d3d.lpD3DDevice->DeleteVertexShader(d3d.fmvVertexShader);
	d3d.lpD3DDevice->DeleteVertexShader(d3d.orthoVertexShader);
	d3d.lpD3DDevice->DeleteVertexShader(d3d.cloudVertexShader);
//	SAFE_RELEASE(d3d.vertexShader);
//	SAFE_RELEASE(d3d.fmvVertexShader);
//	SAFE_RELEASE(d3d.orthoVertexShader);
//	SAFE_RELEASE(d3d.cloudVertexShader);

	// clean up render list classes
	RenderListDeInit();

	delete d3d.effectSystem;

	// release device
	SAFE_RELEASE(d3d.lpD3DDevice);
	LogString("Releasing Direct3D9 device...");

	// release object
	SAFE_RELEASE(d3d.lpD3D);
	LogString("Releasing Direct3D9 object...");

	// release Direct Input stuff
	ReleaseDirectKeyboard();
	ReleaseDirectMouse();
	ReleaseDirectInput();

	// find a better place to put this
	Config_Save();
}

void ReleaseAvPTexture(AVPTEXTURE *texture)
{
	if (texture->buffer)
	{
		free(texture->buffer);
	}

	if (texture)
	{
		free(texture);
	}
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
//bjd - fixme			if (TRIPTASTIC_CHEATMODE || MOTIONBLUR_CHEATMODE)
			if (0)
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
			if (D3DDestBlend != D3DBLEND_ONE) {
				d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
				D3DDestBlend = D3DBLEND_ONE;
			}
		}
		default: break;
	}
}

void ChangeTextureAddressMode(enum TEXTURE_ADDRESS_MODE textureAddressMode)
{
	if (CurrentRenderStates.TextureAddressMode == textureAddressMode)
		return;

	CurrentRenderStates.TextureAddressMode = textureAddressMode;

	if (textureAddressMode == TEXTURE_WRAP)
	{
		// wrap texture addresses
		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSU Wrap fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSV Wrap fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSW Wrap fail");
		}
	}
	else if (textureAddressMode == TEXTURE_CLAMP)
	{
		// clamp texture addresses
		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSU Clamp fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSV Clamp fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DTSS_ADDRESSW Clamp fail");
		}
	}
}

void ChangeFilteringMode(enum FILTERING_MODE_ID filteringRequired)
{
	if (CurrentRenderStates.FilteringMode == filteringRequired) return;

	CurrentRenderStates.FilteringMode = filteringRequired;

	switch(CurrentRenderStates.FilteringMode)
	{
		case FILTERING_BILINEAR_OFF:
		{
			d3d.lpD3DDevice->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTEXF_POINT);
			d3d.lpD3DDevice->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTEXF_POINT);
			break;
		}
		case FILTERING_BILINEAR_ON:
		{
			d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
			d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
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

void EnableZBufferWrites()
{
	if (!ZWritesEnabled)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3DZB_TRUE);
		ZWritesEnabled = true;
	}
}

void DisableZBufferWrites()
{
	if (ZWritesEnabled)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, D3DZB_FALSE);
		ZWritesEnabled = false;
	}
}

bool SetRenderStateDefaults()
{
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
//	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, 8);

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

//	float alphaRef = 0.5f;
//	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAREF,	 *((DWORD*)&alphaRef));//(DWORD)0.5);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAREF,	 (DWORD)0.5);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);

	d3d.lpD3DDevice->SetRenderState(D3DRS_OCCLUSIONCULLENABLE, FALSE);

//	ChangeFilteringMode(FILTERING_BILINEAR_OFF);
	ChangeTranslucencyMode(TRANSLUCENCY_OFF);

	d3d.lpD3DDevice->SetRenderState(D3DRS_CULLMODE,			D3DCULL_NONE);
//	d3d.lpD3DDevice->SetRenderState(D3DRS_CLIPPING,			TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_LIGHTING,			FALSE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_SPECULARENABLE,	TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_DITHERENABLE,		FALSE);

	// enable z-buffer
	d3d.lpD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

	// enable z writes (already on by default)
	d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	ZWritesEnabled = true;

	// set less + equal z buffer test
	d3d.lpD3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	D3DZFunc = D3DCMP_LESSEQUAL;

	return true;
}





#if 0



#include "3dc.h"
#include "awTexLd.h"
#include "module.h"
#include "d3_func.h"
#include "kshape.h"

#include "avp_menugfx.hpp"
extern AVPMENUGFX AvPMenuGfxStorage[];
extern void ReleaseAllFMVTextures(void);

extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);
extern BOOL SetExecuteBufferDefaults();

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int WindowMode;
int	VideoModeColourDepth;
static HRESULT LastError;

D3DINFO d3d;

uint32_t fov = 75;

// byte order macros for A8R8G8B8 d3d texture
enum
{
	BO_BLUE,
	BO_GREEN,
	BO_RED,
	BO_ALPHA
};

// console command : set a new field of view value
void SetFov()
{
	if (Con_GetNumArguments() == 0)
	{
		Con_PrintMessage("usage: r_setfov 75");
		return;
	}

	fov = atoi(Con_GetArgument(0).c_str());
}

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

void ColourFillBackBuffer(int FillColour)
{
	d3d.lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, FillColour, 1.0f, 0 );
}

void PrintD3DMatrix(const char* name, D3DXMATRIX &mat)
{
	char buf[300];
	sprintf(buf, "Printing D3D Matrix: - %s\n"
//	"\t 1 \t 2 \t 3 \t 4\n"
	"\t %f \t %f \t %f \t %f\n"
	"\t %f \t %f \t %f \t %f\n"
	"\t %f \t %f \t %f \t %f\n"
	"\t %f \t %f \t %f \t %f\n", name,
	mat._11, mat._12, mat._13, mat._14,
	mat._21, mat._22, mat._23, mat._24,
	mat._31, mat._32, mat._33, mat._34,
	mat._41, mat._42, mat._43, mat._44);

	OutputDebugString(buf);
}

char* GetDeviceName()
{
	if (d3d.Driver[d3d.CurrentDriver].AdapterInfo.Description != NULL)
	{
		return d3d.Driver[d3d.CurrentDriver].AdapterInfo.Description;
	}
	else return "Default Adapter";
}

LPDIRECT3DTEXTURE8 CreateD3DTallFontTexture (AVPTEXTURE *tex)
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;
	LPDIRECT3DTEXTURE8 swizTexture = NULL;
	D3DLOCKED_RECT	   lock;

	// default colour format
	D3DFORMAT colourFormat = D3DFMT_LIN_A8R8G8B8;

	uint32_t height = 495;
	uint32_t width = 450;

	uint32_t padWidth = 512;
	uint32_t padHeight = 512;

	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, NULL, colourFormat, D3DPOOL_MANAGED, &destTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if (FAILED(LastError))
	{
		destTexture->Release();
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	{
		uint8_t *destPtr, *srcPtr;

		srcPtr = (uint8_t*)tex->buffer;

		D3DCOLOR padColour = D3DCOLOR_XRGB(0,0,0);

		// lets pad the whole thing black first
		for (int y = 0; y < padHeight; y++)
		{
			destPtr = (((uint8_t*)lock.pBits) + y*lock.Pitch);

			for (int x = 0; x < padWidth; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(padColour, padColour, padColour, padColour);

				destPtr += sizeof(uint32_t); // move over an entire 4 byte pixel
			}
		}

		int charWidth = 30;
		int charHeight = 33;

		for (int i = 0; i < 224; i++)
		{
			int row = i / 15; // get row
			int column = i % 15; // get column from remainder value

			int offset = ((column * charWidth) * sizeof(uint32_t)) + ((row * charHeight) * lock.Pitch);

			destPtr = (((uint8_t*)lock.pBits + offset));

			for (int y = 0; y < charHeight; y++)
			{
				destPtr = (((uint8_t*)lock.pBits + offset) + (y*lock.Pitch));

				for (int x = 0; x < charWidth; x++)
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
	}

	D3DLOCKED_RECT lock2;
	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &swizTexture);
	LastError = swizTexture->LockRect(0, &lock2, NULL, NULL );

	XGSwizzleRect(lock.pBits, lock.Pitch, NULL, lock2.pBits, padWidth, padHeight, NULL, sizeof(uint32_t));

	LastError = destTexture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	LastError = swizTexture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	destTexture->Release();

/*
	if(FAILED(D3DXSaveTextureToFileA("tallfont.png", D3DXIFF_PNG, destTexture, NULL))) {
		OutputDebugString("\n couldnt save tex to file");
	}
*/
	//	return destTexture;
	return swizTexture;
}

void WriteTextureToFile(LPDIRECT3DTEXTURE8 srcTexture, const char* fileName)
{
/*
	int width = 0;
	int height = 0;

	// fill tga header
	TgaHeader.idlength = 0;
	TgaHeader.x_origin = width;
	TgaHeader.y_origin = height;
	TgaHeader.colourmapdepth  = 0;
	TgaHeader.colourmaplength = 0;
	TgaHeader.colourmaporigin = 0;
	TgaHeader.colourmaptype   = 0;
	TgaHeader.datatypecode    = 2;			// RGB
	TgaHeader.bitsperpixel    = 32;
	TgaHeader.imagedescriptor = 0x20;		// set origin to top left
	TgaHeader.height = height;
	TgaHeader.width  = width;

	// size of raw image data
	int imageSize = height * width * sizeof(uint32_t);

	// create new buffer for header and image data
	uint8_t *buffer = new uint8_t[sizeof(TGA_HEADER) + imageSize];

	// copy header and image data to buffer
	memcpy(buffer, &TgaHeader, sizeof(TGA_HEADER));

	uint8_t *imageData = buffer + sizeof(TGA_HEADER);

	// lock texture
	D3DLOCKED_RECT lock;

	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	// loop, converting RGB to BGR for D3DX function
	for (int i = 0; i < imageSize; i+=4)
	{

		// BGRA			 // RGBA
		imageData[i+2] = tex->buffer[i];
		imageData[i+1] = tex->buffer[i+1];
		imageData[i]   = tex->buffer[i+2];
		imageData[i+3] = tex->buffer[i+3];
	}
*/
}

LPDIRECT3DTEXTURE8 CreateFmvTexture(uint32_t *width, uint32_t *height, uint32_t usage, uint32_t pool)
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;

	int newWidth, newHeight;

	// check if passed value is already a power of 2
	if (!IsPowerOf2(*width))
	{
		newWidth = NearestSuperiorPow2(*width);
	}
	else { newWidth = *width; }

	if (!IsPowerOf2(*height))
	{
		newHeight = NearestSuperiorPow2(*height);
	}
	else { newHeight = *height; }

	LastError = d3d.lpD3DDevice->CreateTexture(newWidth, newHeight, 1, 0, D3DFMT_LIN_X8R8G8B8, 0, &destTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	if (XGIsSwizzledFormat(D3DFMT_LIN_A8R8G8B8))
	{
		OutputDebugString("swizzledeeded!\n");
	}

	// lets clear it to black
	D3DLOCKED_RECT lock;

	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}
	else
	{
		memset(lock.pBits, 0, lock.Pitch * newHeight);

		LastError = destTexture->UnlockRect(0);
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}
	}

	*width = newWidth;
	*height = newHeight;

	return destTexture;
}

LPDIRECT3DTEXTURE8 CreateFmvTexture2(uint32_t *width, uint32_t *height)
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;
#if 0
	int newWidth, newHeight;

	// check if passed value is already a power of 2
	if (!IsPowerOf2(*width))
	{
		newWidth = NearestSuperiorPow2(*width);
	}
	else { newWidth = *width; }

	if (!IsPowerOf2(*height))
	{
		newHeight = NearestSuperiorPow2(*height);
	}
	else { newHeight = *height; }
#endif

	LastError = d3d.lpD3DDevice->CreateTexture(*width, *height, 1, D3DUSAGE_DYNAMIC, D3DFMT_LIN_L8, D3DPOOL_DEFAULT, &destTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

//	*width = newWidth;
//	*height = newHeight;

	return destTexture;
}

int32_t CreateVertexShaderFromMemory(const char *shader, DWORD &vertexShader)
{
	XGBuffer* pShaderData = NULL;
	XGBuffer* pErrors = NULL;

	// compile shader from file
	LastError = XGAssembleShader(NULL, // filename (for errors only)
			shader,
			strlen(shader),
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
		LogErrorString("Error cannot compile vertex shader file");
		return -1;
	}

	LastError = d3d.lpD3DDevice->CreateVertexShader(mainDecl, (const DWORD*)pShaderData->pData, &vertexShader, 0); // ??
	if (FAILED(LastError))
	{
		LogErrorString("Error create vertex shader");
		return -1;
	}

	pShaderData->Release();

	return 0;
}

int32_t CreateVertexShader(const std::string &fileName, DWORD *vertexShader, DWORD *vertexDeclaration)
{
	XGBuffer* pShaderData = NULL;
	XGBuffer* pErrors = NULL;

	std::string properPath = "d:\\" + fileName;

	// have to open the file ourselves and pass as string. XGAssembleShader won't do this for us
	std::ifstream shaderFile(properPath.c_str(), std::ifstream::in);

	if (!shaderFile.is_open())
	{
		LogErrorString("Error cannot open vertex shader file");
		return -1;
	}

	std::stringstream buffer;
	buffer << shaderFile.rdbuf();

	// compile shader from file
	LastError = XGAssembleShader(fileName.c_str(), // filename (for errors only)
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
		LogErrorString("Error cannot compile vertex shader file");
		return -1;
	}

	if (pErrors)
	{
//		LogErrorString("Errors!");
		OutputDebugString((const char*)pErrors->GetBufferPointer());
		pErrors->Release();
	}

	LastError = d3d.lpD3DDevice->CreateVertexShader(vertexDeclaration, (const DWORD*)pShaderData->pData, vertexShader, 0); // ??
	if (FAILED(LastError))
	{
		LogErrorString("Error create vertex shader");
		return -1;
	}

	pShaderData->Release();

	return 0;
}

int32_t CreatePixelShader(const std::string &fileName, DWORD *pixelShader)
{
	XGBuffer* pShaderData = NULL;
	XGBuffer* pErrors = NULL;

	std::string properPath = "d:\\" + fileName;

	// have to open the file ourselves and pass as string. XGAssembleShader won't do this for us
	std::ifstream shaderFile(properPath.c_str(), std::ifstream::in);

	if (!shaderFile.is_open())
	{
		LogErrorString("Error cannot open pixel shader file");
		return -1;
	}

	std::stringstream buffer;
	buffer << shaderFile.rdbuf();

	// compile shader from file
	LastError = XGAssembleShader(fileName.c_str(), // filename (for errors only)
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
		LogErrorString("Error cannot compile pixel shader file");
		return -1;
	}

	if (pErrors)
	{
//		LogErrorString("Errors!");
		OutputDebugString((const char*)pErrors->GetBufferPointer());
		pErrors->Release();
	}

	LastError = d3d.lpD3DDevice->CreatePixelShader((D3DPIXELSHADERDEF*)pShaderData->GetBufferPointer(), pixelShader); // ??
	if (FAILED(LastError))
	{
		LogErrorString("Error create pixel vertex shader");
		return -1;
	}

	pShaderData->Release();

	return 0;
}

// removes pure red colour from a texture. used to remove red outline grid on small font texture.
// we remove the grid as it can sometimes bleed onto text when we use texture filtering.
void DeRedTexture(LPDIRECT3DTEXTURE8 texture)
{
	D3DLOCKED_RECT lock;

	LastError = texture->LockRect(0, &lock, NULL, NULL );
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return;
	}

	uint8_t *destPtr = NULL;

	for (uint32_t y = 0; y < 256; y++)
	{
		destPtr = (((uint8_t*)lock.pBits) + y*lock.Pitch);

		for (uint32_t x = 0; x < 256; x++)
		{
			if ((destPtr[BO_RED] == 255) && (destPtr[BO_BLUE] == 0) && (destPtr[BO_GREEN] == 0))
			{
				destPtr[BO_RED] = 0;
			}

			destPtr += sizeof(uint32_t); // move over an entire 4 byte pixel
		}
	}

	LastError = texture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return;
	}
}

// use this to make textures from non power of two images
LPDIRECT3DTEXTURE8 CreateD3DTexturePadded(AVPTEXTURE *tex, uint32_t *realWidth, uint32_t *realHeight)
{
	if (tex == NULL)
	{
		*realWidth = 0;
		*realHeight = 0;
		return NULL;
	}

	uint32_t original_width = tex->width;
	uint32_t original_height = tex->height;
	uint32_t new_width = original_width;
	uint32_t new_height = original_height;

	D3DCOLOR pad_colour = D3DCOLOR_XRGB(0,0,0);

	// check if passed value is already a power of 2
	if (!IsPowerOf2(tex->width)) {
		new_width = NearestSuperiorPow2(tex->width);
	}
	else { new_width = original_width; }

	if (!IsPowerOf2(tex->height)) {
		new_height = NearestSuperiorPow2(tex->height);
	}
	else { new_height = original_height; }

	(*realHeight) = new_height;
	(*realWidth) = new_width;

	LPDIRECT3DTEXTURE8 destTexture = NULL;

	D3DXIMAGE_INFO image;
	image.Depth = 32;
	image.Width = tex->width;
	image.Height = tex->height;
	image.MipLevels = 1;
	image.Depth = D3DFMT_A8R8G8B8;

	D3DPOOL poolType = D3DPOOL_MANAGED;

	// fill tga header
	TgaHeader.idlength = 0;
	TgaHeader.x_origin = tex->width;
	TgaHeader.y_origin = tex->height;
	TgaHeader.colourmapdepth  = 0;
	TgaHeader.colourmaplength = 0;
	TgaHeader.colourmaporigin = 0;
	TgaHeader.colourmaptype   = 0;
	TgaHeader.datatypecode    = 2;			// RGB
	TgaHeader.bitsperpixel    = 32;
	TgaHeader.imagedescriptor = 0x20;		// set origin to top left
	TgaHeader.height = tex->height;
	TgaHeader.width  = tex->width;

	// size of raw image data
	uint32_t imageSize = tex->height * tex->width * sizeof(uint32_t);

	// create new buffer for header and image data
	uint8_t *buffer = new uint8_t[sizeof(TGA_HEADER) + imageSize];

	// copy header and image data to buffer
	memcpy(buffer, &TgaHeader, sizeof(TGA_HEADER));

	uint8_t *imageData = buffer + sizeof(TGA_HEADER);

	// loop, converting RGB to BGR for D3DX function
	for (int i = 0; i < imageSize; i+=4)
	{

		// BGRA			 // RGBA
		imageData[i+2] = tex->buffer[i];
		imageData[i+1] = tex->buffer[i+1];
		imageData[i]   = tex->buffer[i+2];
		imageData[i+3] = tex->buffer[i+3];
	}

	if (FAILED(D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		D3DX_DEFAULT,//tex->width,
		D3DX_DEFAULT,//tex->height,
		1,
		0,
		D3DFMT_A8R8G8B8,
		poolType,
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&image,
		0,
		&destTexture)))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		delete[] buffer;
		return NULL;
	}

	delete[] buffer;

	return destTexture;
}

uint32_t CreateD3DTextureFromFile(const char* fileName, Texture &texture)
{
//	D3DTEXTURE destTexture = NULL;
	D3DXIMAGE_INFO imageInfo;

	LastError = D3DXCreateTextureFromFileEx(d3d.lpD3DDevice,
		fileName,
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
		return -1;
	}

	texture.width = imageInfo.Width;
	texture.height = imageInfo.Height;

	return 0;
}

LPDIRECT3DTEXTURE8 CreateD3DTexture(AVPTEXTURE *tex, uint8_t *buf, uint32_t usage, D3DPOOL poolType)
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;

	// fill tga header
	TgaHeader.idlength = 0;
	TgaHeader.x_origin = tex->width;
	TgaHeader.y_origin = tex->height;
	TgaHeader.colourmapdepth  = 0;
	TgaHeader.colourmaplength = 0;
	TgaHeader.colourmaporigin = 0;
	TgaHeader.colourmaptype   = 0;
	TgaHeader.datatypecode    = 2;			// RGB
	TgaHeader.bitsperpixel    = 32;
	TgaHeader.imagedescriptor = 0x20;		// set origin to top left
	TgaHeader.height = tex->height;
	TgaHeader.width  = tex->width;

	// size of raw image data
	uint32_t imageSize = tex->height * tex->width * sizeof(uint32_t);

	// create new buffer for header and image data
	uint8_t *buffer = new uint8_t[sizeof(TGA_HEADER) + imageSize];

	// copy header and image data to buffer
	memcpy(buffer, &TgaHeader, sizeof(TGA_HEADER));

	uint8_t *imageData = buffer + sizeof(TGA_HEADER);

	// loop, converting RGB to BGR for D3DX function
	for (uint32_t i = 0; i < imageSize; i+=4)
	{
		// RGB
		// BGR
		imageData[i+2] = buf[i];
		imageData[i+1] = buf[i+1];
		imageData[i] = buf[i+2];
		imageData[i+3] = buf[i+3];
	}

	D3DXIMAGE_INFO image;
	image.Depth = 32;
	image.Width = tex->width;
	image.Height= tex->height;
	image.MipLevels = 1;
	image.Depth = D3DFMT_A8R8G8B8;

	D3DFORMAT textureFormat;

	textureFormat = D3DFMT_A8R8G8B8;

	LastError = D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		tex->width,
		tex->height,
		1,
		0,
		textureFormat,
//		D3DFMT_DXT3,
		D3DPOOL_DEFAULT, // not used for xbox
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&image,
		0,
		&destTexture);

	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		delete[] buffer;
		return NULL;
	}

	delete[] buffer;
	return destTexture;
}

// called from Scrshot.cpp
void CreateScreenShotImage()
{
#if 0 // can't be bothered implementing this for the xbox
	SYSTEMTIME systemTime;
	LPDIRECT3DSURFACE8 frontBuffer = NULL;

	std::ostringstream fileName;

	/* get current system time */
	GetSystemTime(&systemTime);

	//	creates filename from date and time, adding a prefix '0' to seconds value
	//	otherwise 9 seconds appears as '9' instead of '09'
	{
		bool prefixSeconds = false;
		if(systemTime.wYear < 10) prefixSeconds = true;

		fileName << "AvP_" << systemTime.wDay << "-" << systemTime.wMonth << "-" << systemTime.wYear << "_" << systemTime.wHour << "-" << systemTime.wMinute << "-";

		if (systemTime.wSecond < 10)
		{
			fileName << "0" << systemTime.wSecond;
		}
		else
		{
			fileName << systemTime.wSecond;
		}

		fileName << ".jpg";
	}

	/* create surface to copy screen to */
	LastError = (d3d.lpD3DDevice->CreateOffscreenPlainSurface(ScreenDescriptorBlock.SDB_Width, ScreenDescriptorBlock.SDB_Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &frontBuffer, NULL);
	if (FAILED(LastError)
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return;
	}

	/* copy front buffer screen to surface */
	LastError = d3d.lpD3DDevice->GetFrontBufferData(0, frontBuffer);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		SAFE_RELEASE(frontBuffer);
		return;
	}

	/* save surface to image file */
	LastError = D3DXSaveSurfaceToFile(fileName.str().c_str(), D3DXIFF_JPG, frontBuffer, NULL, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	/* release surface */
	SAFE_RELEASE(frontBuffer);
#endif
}

BOOL ReleaseVolatileResources()
{
/*
	SAFE_RELEASE(d3d.lpD3DBackSurface);
	SAFE_RELEASE(d3d.lpD3DIndexBuffer);
	SAFE_RELEASE(d3d.lpD3DVertexBuffer);
*/
	return true;
}

BOOL CreateVolatileResources()
{
/*
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(MAX_VERTEXES * sizeof(D3DTLVERTEX),D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_TLVERTEX, D3DPOOL_DEFAULT, &d3d.lpD3DVertexBuffer);
	if(FAILED(LastError))
	{
		LogDxError(LastError);
		return FALSE;
	}

	LastError = d3d.lpD3DDevice->CreateIndexBuffer(MAX_INDICES * 3 * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &d3d.lpD3DIndexBuffer);
	if(FAILED(LastError))
	{
		LogDxError(LastError);
		return FALSE;
	}

	LastError = d3d.lpD3DDevice->SetStreamSource( 0, d3d.lpD3DVertexBuffer, sizeof(D3DTLVERTEX));
	if (FAILED(LastError))
	{
		LogDxError(LastError);
		return FALSE;
	}
*/

	// create our 2D vertex buffer
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(4 * 2000 * sizeof(ORTHOVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, /*D3DFVF_ORTHOVERTEX*/0, D3DPOOL_DEFAULT, &d3d.lpD3DOrthoVertexBuffer);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	// create our 2D index buffer
	LastError = d3d.lpD3DDevice->CreateIndexBuffer(MAX_INDICES * 3 * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &d3d.lpD3DOrthoIndexBuffer);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}
/*
	LastError = d3d.lpD3DDevice->SetVertexShader(D3DFVF_LVERTEX);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}
*/
/*
	LastError = d3d.lpD3DDevice->SetFVF(D3DFVF_TLVERTEX);
	if(FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}
*/
	SetExecuteBufferDefaults();

	return true;
}

BOOL ChangeGameResolution(uint32_t width, uint32_t height)
{
	// don't bother resetting device if we're already using the requested settings
	if ((width == d3d.d3dpp.BackBufferWidth) && (height == d3d.d3dpp.BackBufferHeight))
	{
		return TRUE;
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
/*
		OutputDebugString(DXGetErrorString8(LastError));
		OutputDebugString(DXGetErrorDescription8(LastError));
		OutputDebugString("\n");
*/
		// this'll occur if the resolution width and height passed aren't usable on this device
		if (D3DERR_INVALIDCALL == LastError)
		{
			// set some default, safe resolution?
			width = 800;
			height = 600;

//			ChangeWindowsSize(width, height);

			d3d.d3dpp.BackBufferWidth = width;
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

	d3d.D3DViewport.Width = width;
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

	return TRUE;
}

BOOL InitialiseDirect3D()
{
	// clear log file first, then write header text
	ClearLog();
	Con_PrintMessage("Starting to initialise Direct3D");

	uint32_t width = 640;
	uint32_t height = 480;
//	uint32_t colourDepth = 32;
	uint32_t defaultDevice = D3DADAPTER_DEFAULT;
	uint32_t thisDevice = D3DADAPTER_DEFAULT;
	bool useTripleBuffering = Config_GetBool("[VideoMode]", "UseTripleBuffering", false);
	shaderPath = Config_GetString("[VideoMode]", "ShaderPath", "shaders\\");

	//	Zero d3d structure
    memset(&d3d, 0, sizeof(D3DINFO));

	//  Set up Direct3D interface object
	d3d.lpD3D = Direct3DCreate8(D3D_SDK_VERSION);
	if (!d3d.lpD3D)
	{
		Con_PrintError("Could not create Direct3D8 object");
		return FALSE;
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

//	D3DDISPLAYMODE d3ddm;
//	LastError = d3d.lpD3D->GetAdapterDisplayMode(defaultDevice, &d3ddm);

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
		if (tempMode.Format == D3DFMT_LIN_X8R8G8B8)
		{
			int j = 0;
			// check if the mode already exists
			for (; j < d3d.Driver[thisDevice].NumModes; j++)
			{
				if ((d3d.Driver[defaultDevice].DisplayMode[j].Width == tempMode.Width) &&
					(d3d.Driver[defaultDevice].DisplayMode[j].Height == tempMode.Height) &&
					(d3d.Driver[defaultDevice].DisplayMode[j].Format == tempMode.Format))
					break;
			}

			// we looped all the way through but didn't break early due to already existing item
			if (j == d3d.Driver[thisDevice].NumModes)
			{
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Width       = tempMode.Width;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Height      = tempMode.Height;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Format      = tempMode.Format;
				d3d.Driver[defaultDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].RefreshRate = 0;
				d3d.Driver[thisDevice].NumModes++;
			}
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

	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);

	if (FAILED(LastError))
	{
		LogErrorString("Could not create Direct3D device");
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

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
	Con_PrintMessage("\t Resolution set: " + LogInteger(d3dpp.BackBufferWidth) + " x " + LogInteger(d3dpp.BackBufferHeight));

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
	int safeOffset = Config_GetInt("[VideoMode]", "SafeZoneOffset", 5);
	ScreenDescriptorBlock.SDB_SafeZoneWidthOffset = (width / 100) * safeOffset;
	ScreenDescriptorBlock.SDB_SafeZoneHeightOffset = (height / 100) * safeOffset;

	// save a copy of the presentation parameters for use
	// later (device reset, resolution/depth change)
	d3d.d3dpp = d3dpp;

	// create vertex and index buffers
	CreateVolatileResources();

	mainVertex = new D3DLVERTEX[4096];
	mainIndex = new WORD[9216 * 3];

	SetTransforms();

	Con_Init();
	Net_Initialise();
	Font_Init();

	CreateVertexShader("vertex_1_1.vsh", &d3d.vertexShader, mainDecl);
	CreateVertexShader("orthoVertex_1_1.vsh", &d3d.orthoVertexShader, orthoDecl);

	CreatePixelShader("pixel_1_1.psh", &d3d.pixelShader);

	d3d.lpD3DDevice->CreateTexture(1, 1, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &blankTexture);

	D3DLOCKED_RECT lock;

	blankTexture->LockRect(0, &lock, NULL, 0);

	memset(lock.pBits, 255, lock.Pitch);

	blankTexture->UnlockRect(0);

	Con_PrintMessage("Initialised Direct3D8 succesfully");
	return TRUE;
}

void SetTransforms()
{
	// Setup orthographic projection matrix
	uint32_t standardWidth = 640;
	uint32_t wideScreenWidth = 852;

	// setup view matrix
	D3DXMatrixIdentity(&matView);

	// set up orthographic projection matrix
	D3DXMatrixOrthoLH(&matOrtho, 2.0f, -2.0f, 1.0f, 10.0f);

	// set up projection matrix
	D3DXMatrixPerspectiveFovLH(&matProjection, D3DXToRadian(75), (float)ScreenDescriptorBlock.SDB_Width / (float)ScreenDescriptorBlock.SDB_Height, 64.0f, 1000000.0f);

	D3DXMatrixIdentity(&matIdentity);
	d3d.lpD3DDevice->SetTransform(D3DTS_WORLD,		&matIdentity);
	d3d.lpD3DDevice->SetTransform(D3DTS_VIEW,		&matIdentity);
	d3d.lpD3DDevice->SetTransform(D3DTS_PROJECTION, &matOrtho);
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
    DeallocateAllImages();

    Font_Release();

	// release back-buffer copy surface, vertex buffer and index buffer
	ReleaseVolatileResources();

	SAFE_RELEASE(d3d.lpD3DDevice);
	LogString("Releasing Direct3D device...");

	SAFE_RELEASE(d3d.lpD3D);
	LogString("Releasing Direct3D object...");

	// release Direct Input stuff
	ReleaseDirectKeyboard();
	ReleaseDirectMouse();
	ReleaseDirectInput();

	// find a better place to put this
	Config_Save();
}

void ReleaseAvPTexture(AVPTEXTURE *texture)
{
	if (texture->buffer)
	{
		free(texture->buffer);
	}
	if (texture)
	{
		free(texture);
	}
}

void ReleaseD3DTexture(LPDIRECT3DTEXTURE8 *d3dTexture)
{
	// release d3d texture
	SAFE_RELEASE(*d3dTexture);
}

void FlushD3DZBuffer()
{
	d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

// For extern "C"
};

#endif