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

extern int NumberOfFMVTextures;
#include "fmvCutscenes.h"
#define MAX_NO_FMVTEXTURES 10
extern FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];

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

extern "C" {

#define INITGUID

#include "3dc.h"
#include "awTexLd.h"
#include "module.h"
#include "d3_func.h"
#include "avp_menus.h"

#include "kshape.h"
#include "eax.h"
#include "vmanpset.h"
#include "networking.h"

#include "avp_menugfx.hpp"
extern AVPMENUGFX AvPMenuGfxStorage[];
extern void ReleaseAllFMVTextures(void);

extern "C++"
{
	#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
	#include "logString.h"
	#include "configFile.h"
	#include <d3dx9.h>
	#include "console.h"
	extern void Font_Init();
	extern void Font_Release();
	D3DXMATRIX matOrtho;
	D3DXMATRIX matProjection;
	D3DXMATRIX matViewProjection;
	D3DXMATRIX matView; 
	D3DXMATRIX matIdentity;
}

extern HWND hWndMain;
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern void DeleteRenderMemory();
extern void ChangeWindowsSize(int width, int height);
extern int WindowMode;
int	VideoModeColourDepth;
int	NumAvailableVideoModes;
static HRESULT LastError;
D3DINFO d3d;

// byte order macros for A8R8G8B8 d3d texture
enum 
{
	BO_BLUE,
	BO_GREEN,
	BO_RED,
	BO_ALPHA
};

void WriteMenuTextures()
{
	char *filename = new char[strlen(GetSaveFolderPath()) + MAX_PATH];

	for (int i = 0; i < 54; i++)
	{
		sprintf(filename, "%s%d.png", GetSaveFolderPath(), i);

		D3DXSaveTextureToFileA(filename, D3DXIFF_PNG, AvPMenuGfxStorage[i].menuTexture, NULL);
	}
}

// TGA header structure
#pragma pack(1)
struct TGA_HEADER 
{
	char		idlength;
	char		colourmaptype;
	char		datatypecode;
	short int	colourmaporigin;
	short int	colourmaplength;
	char		colourmapdepth;
	short int	x_origin;
	short int	y_origin;
	short		width;
	short		height;
	char		bitsperpixel;
	char		imagedescriptor;
};
#pragma pack()

static TGA_HEADER TgaHeader = {0};

void ColourFillBackBuffer(int FillColour) 
{
	d3d.lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, FillColour, 1.0f, 0 );
}

char* GetDeviceName() 
{
	if (d3d.Driver[d3d.CurrentDriver].AdapterInfo.Description != NULL) 
	{
		return d3d.Driver[d3d.CurrentDriver].AdapterInfo.Description;
	}
	else return "Default Adapter";
}

// list of allowed display formats
const D3DFORMAT DisplayFormats[] =
{
	D3DFMT_X8R8G8B8,
	D3DFMT_A8R8G8B8,
	D3DFMT_R8G8B8,
	D3DFMT_A1R5G5B5,
	D3DFMT_X1R5G5B5,
	D3DFMT_R5G6B5
};

// 16 bit formats
const D3DFORMAT DisplayFormats16[] =
{
	D3DFMT_R8G8B8,
	D3DFMT_A1R5G5B5,
	D3DFMT_X1R5G5B5,
	D3DFMT_R5G6B5
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
	D3DFMT_D24S8,
	D3DFMT_D32,
	D3DFMT_D24X8,
	D3DFMT_D24FS8,
	D3DFMT_D16
};

bool usingStencil = false;

#if 0
LPDIRECT3DTEXTURE9 CheckAndLoadUserTexture(const char *fileName, int *width, int *height)
{
	LPDIRECT3DTEXTURE9	tempTexture = NULL;
	D3DXIMAGE_INFO		imageInfo;

	std::string fullFileName(fileName);
	fullFileName += ".png";

	// try find a png file at given path
	LastError = D3DXCreateTextureFromFileEx(d3d.lpD3DDevice, 
		fullFileName.c_str(), 
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
		&tempTexture
		);	

	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		*width = 0;
		*height = 0;
		return NULL;
	}

	OutputDebugString("created user texture: ");
	OutputDebugString(fullFileName.c_str());
	OutputDebugString("\n");
	
	*width = imageInfo.Width;
	*height = imageInfo.Height;
	return tempTexture;
}
#endif

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

// called from Scrshot.cpp
void CreateScreenShotImage()
{
	SYSTEMTIME systemTime;
	LPDIRECT3DSURFACE9 frontBuffer = NULL;

	std::ostringstream fileName;
	
	// get current system time
	GetSystemTime(&systemTime);

	//	creates filename from date and time, adding a prefix '0' to seconds value
	//	otherwise 9 seconds appears as '9' instead of '09'
	{
		bool prefixSeconds = false;
		if (systemTime.wYear < 10) prefixSeconds = true;

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

	// create surface to copy screen to
	if (FAILED(d3d.lpD3DDevice->CreateOffscreenPlainSurface(ScreenDescriptorBlock.SDB_Width, ScreenDescriptorBlock.SDB_Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &frontBuffer, NULL)))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("Couldn't create screenshot surface\n");
		return;
	}

	// copy front buffer screen to surface
	if (FAILED(d3d.lpD3DDevice->GetFrontBufferData(0, frontBuffer))) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("Couldn't get a copy of the front buffer\n");
		SAFE_RELEASE(frontBuffer);
		return;
	}

	// save surface to image file
	if (FAILED(D3DXSaveSurfaceToFile(fileName.str().c_str(), D3DXIFF_JPG, frontBuffer, NULL, NULL))) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("Save Surface to file failed!!!\n");
	}

	// release surface
	SAFE_RELEASE(frontBuffer);
}

D3DTEXTURE CreateD3DTallFontTexture(AVPTEXTURE *tex) 
{
	D3DTEXTURE destTexture = NULL;
	D3DLOCKED_RECT lock;

	// default colour format
	D3DFORMAT colourFormat = D3DFMT_R5G6B5;

	if (ScreenDescriptorBlock.SDB_Depth == 16) 
	{
		colourFormat = D3DFMT_R5G6B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32)
	{
		colourFormat = D3DFMT_A8R8G8B8;
	}

	int width = 450;
	int height = 495;

	int padWidth = 512;
	int padHeight = 512;

	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, NULL, colourFormat, D3DPOOL_MANAGED, &destTexture, NULL);
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

	if (ScreenDescriptorBlock.SDB_Depth == 16) 
	{
		uint16_t *destPtr;
		uint8_t  *srcPtr;

		srcPtr = (uint8_t*)tex->buffer;

		D3DCOLOR padColour = D3DCOLOR_ARGB(255, 255, 0, 255);

		// lets pad the whole thing black first
		for (int y = 0; y < padHeight; y++)
		{
			destPtr = ((uint16_t*)(((uint8_t*)lock.pBits) + y*lock.Pitch));

			for (int x = 0; x < padWidth; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*destPtr = static_cast<uint16_t> (RGB16(padColour, padColour, padColour));
				destPtr += sizeof(uint16_t);
			}
		}

		int charWidth = 30;
		int charHeight = 33;

		for (int i = 0; i < 224; i++) 
		{
			int row = i / 15; // get row
			int column = i % 15; // get column from remainder value

			int offset = ((column * charWidth) * sizeof(uint16_t)) + ((row * charHeight) * lock.Pitch);

			destPtr = ((uint16_t*)(((uint8_t*)lock.pBits + offset)));

			for (int y = 0; y < charHeight; y++) 
			{
				destPtr = ((uint16_t*)(((uint8_t*)lock.pBits + offset) + (y*lock.Pitch)));

				for (int x = 0; x < charWidth; x++)
				{
					*destPtr = RGB16(srcPtr[0], srcPtr[1], srcPtr[2]);

					destPtr += sizeof(uint16_t);
					srcPtr += sizeof(uint32_t); // our source is 32 bit, move over an entire 4 byte pixel
				}
			}
		}
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) 
	{
		uint8_t *destPtr, *srcPtr;

		srcPtr = (uint8_t*)tex->buffer;

		D3DCOLOR padColour = D3DCOLOR_ARGB(255, 255, 0, 255);

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

	LastError = destTexture->UnlockRect(0);
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}
/*
	if (FAILED(D3DXSaveTextureToFileA("c:\\temp\\tallfont.png", D3DXIFF_PNG, destTexture, NULL)))
	{
		OutputDebugString("\n couldnt save tex to file");
	}
*/
	return destTexture;
}

D3DTEXTURE CreateFmvTexture(int *width, int *height, int usage, int pool)
{
	D3DTEXTURE destTexture = NULL;

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

//	D3DXCheckTextureRequirements(d3d.lpD3DDevice, (UINT*)width, (UINT*)height, NULL, usage, NULL, (D3DPOOL)pool);

//	LastError = d3d.lpD3DDevice->CreateTexture(*width, *height, 1, usage, D3DFMT_X8R8G8B8, (D3DPOOL)pool, &destTexture, NULL);
	LastError = d3d.lpD3DDevice->CreateTexture(newWidth, newHeight, 1, usage, D3DFMT_X8R8G8B8, (D3DPOOL)pool, &destTexture, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	*width = newWidth;
	*height = newHeight;

	return destTexture;
}

int imageNum = 0;


// removes pure red colour from a texture. used to remove red outline grid on small font texture.
// we remove the grid as it can sometimes bleed onto text when we use texture filtering.
void DeRedTexture(D3DTEXTURE texture)
{
	D3DLOCKED_RECT	lock;

	LastError = texture->LockRect(0, &lock, NULL, NULL );
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return;
	}

	uint8_t *destPtr = NULL;

	for (int y = 0; y < 256; y++)
	{
		destPtr = (((uint8_t*)lock.pBits) + y*lock.Pitch);

		for (int x = 0; x < 256; x++)
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
D3DTEXTURE CreateD3DTexturePadded(AVPTEXTURE *tex, int *realWidth, int *realHeight) 
{
	int original_width = tex->width;
	int original_height = tex->height;
	int new_width = original_width;
	int new_height = original_height;

	D3DCOLOR pad_colour = D3DCOLOR_ARGB(255, 255, 0, 255);

	// check if passed value is already a power of 2
	if (!IsPowerOf2(tex->width)) 
	{
		new_width = NearestSuperiorPow2(tex->width);
	}
	else { new_width = original_width; }

	if (!IsPowerOf2(tex->height)) 
	{
		new_height = NearestSuperiorPow2(tex->height);
	}
	else { new_height = original_height; }

	// set passed in width and height values to be used later
//	(*real_height) = new_height;
//	(*real_width) = new_width;

//	D3DXCheckTextureRequirements(d3d.lpD3DDevice, (UINT*)&new_width, (UINT*)&new_height, NULL, 0, NULL, D3DPOOL_MANAGED);

	(*realHeight) = new_height;
	(*realWidth) = new_width;

	D3DTEXTURE destTexture = NULL;

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
	int imageSize = tex->height * tex->width * sizeof(uint32_t);

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

D3DTEXTURE CreateD3DTexture(AVPTEXTURE *tex, uint8_t *buf, int usage, D3DPOOL poolType) 
{
	D3DTEXTURE destTexture = NULL;

	// fill tga header
	TgaHeader.idlength = 0;
	TgaHeader.x_origin = tex->width;
	TgaHeader.y_origin = tex->height;
	TgaHeader.colourmapdepth	= 0;
	TgaHeader.colourmaplength	= 0;
	TgaHeader.colourmaporigin	= 0;
	TgaHeader.colourmaptype		= 0;
	TgaHeader.datatypecode		= 2;		// RGB
	TgaHeader.bitsperpixel		= 32;
	TgaHeader.imagedescriptor	= 0x20;		// set origin to top left
	TgaHeader.height = tex->height;
	TgaHeader.width = tex->width;

	// size of raw image data
	int imageSize = tex->height * tex->width * sizeof(uint32_t);

	// create new buffer for header and image data
	uint8_t *buffer = new uint8_t[sizeof(TGA_HEADER) + imageSize];

	// copy header and image data to buffer
	memcpy(buffer, &TgaHeader, sizeof(TGA_HEADER));

	uint8_t *imageData = buffer + sizeof(TGA_HEADER);

	// loop, converting RGB to BGR for D3DX function
	for (int i = 0; i < imageSize; i+=4)
	{
		// ARGB
		// BGR
		imageData[i+2] = buf[i];
		imageData[i+1] = buf[i+1];
		imageData[i]   = buf[i+2];
		imageData[i+3] = buf[i+3];
	}

	D3DXIMAGE_INFO image;
	image.Depth = 32;
	image.Width = tex->width;
	image.Height = tex->height;
	image.MipLevels = 1;
	image.Depth = D3DFMT_A8R8G8B8;

	if (FAILED(D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		tex->width,
		tex->height,
		1,
		usage,
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
		OutputDebugString("COULD NOT CREATE TEXTURE?\n");
		delete[] buffer;
		return NULL;
	}

	delete[] buffer;
	return destTexture;
}

// size of vertex and index buffers
const int MAX_VERTEXES = 4096;
const int MAX_INDICES = 9216;

BOOL ReleaseVolatileResources() 
{
	ReleaseAllFMVTexturesForDeviceReset();

	SAFE_RELEASE(d3d.lpD3DIndexBuffer);
	SAFE_RELEASE(d3d.lpD3DVertexBuffer);
	SAFE_RELEASE(d3d.lpD3DOrthoVertexBuffer);

	return TRUE;
}

BOOL CreateVolatileResources() 
{
	RecreateAllFMVTexturesAfterDeviceReset();

	// create dynamic vertex buffer
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(MAX_VERTEXES * sizeof(D3DTLVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_TLVERTEX, D3DPOOL_DEFAULT, &d3d.lpD3DVertexBuffer, NULL);
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	// create index buffer
	LastError = d3d.lpD3DDevice->CreateIndexBuffer(MAX_INDICES * 3 * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &d3d.lpD3DIndexBuffer, NULL);
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	// create our 2D vertex buffer
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(4 * 20 * sizeof(ORTHOVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_ORTHOVERTEX, D3DPOOL_DEFAULT, &d3d.lpD3DOrthoVertexBuffer, NULL);
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	LastError = d3d.lpD3DDevice->SetStreamSource(0, d3d.lpD3DVertexBuffer, 0, sizeof(D3DTLVERTEX));
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	LastError = d3d.lpD3DDevice->SetFVF(D3DFVF_TLVERTEX);
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	SetExecuteBufferDefaults();

	return TRUE;
}

BOOL ChangeGameResolution(int width, int height, int colourDepth)
{
	// don't bother resetting device if we're already using the requested settings
	if ((width == d3d.d3dpp.BackBufferWidth) && (height == d3d.d3dpp.BackBufferHeight))
	{
		return TRUE;
	}

	ReleaseVolatileResources();

	d3d.d3dpp.BackBufferWidth = width;
	d3d.d3dpp.BackBufferHeight = height;

	// change the window size
	ChangeWindowsSize(width, height);

	// try reset the device
	LastError = d3d.lpD3DDevice->Reset(&d3d.d3dpp);
	if (FAILED(LastError))
	{
		// log an error message
		std::stringstream sstream;
		sstream << "Can't set resolution " << width << " x " << height << ". Setting default safe values";
		Con_PrintError(sstream.str());

		OutputDebugString(DXGetErrorString(LastError));
		OutputDebugString(DXGetErrorDescription(LastError));
		OutputDebugString("\n");

		// this'll occur if the resolution width and height passed aren't usable on this device
		if (D3DERR_INVALIDCALL == LastError)
		{
			// set some default, safe resolution?
			width = 800;
			height = 600;

			ChangeWindowsSize(width, height);

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
	ScreenDescriptorBlock.SDB_Depth		= colourDepth;
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
//	SetExecuteBufferDefaults();

	// set up projection matrix
	D3DXMatrixPerspectiveFovLH( &matProjection, width / height, D3DX_PI / 2, 1.0f, 100.0f);

	d3d.lpD3DDevice->SetTransform( D3DTS_PROJECTION, &matOrtho );
	d3d.lpD3DDevice->SetTransform( D3DTS_WORLD, &matIdentity );
	d3d.lpD3DDevice->SetTransform( D3DTS_VIEW, &matIdentity );

	return TRUE;
}

// need to redo all the enumeration code here, as it's not very good..
BOOL InitialiseDirect3D()
{
	// clear log file first, then write header text
	ClearLog();
	Con_PrintMessage("Starting to initialise Direct3D9");

	// grab the users selected resolution from the config file
	int width = Config_GetInt("[VideoMode]", "Width", 800);
	int height = Config_GetInt("[VideoMode]", "Height", 600);
	int colourDepth = Config_GetInt("[VideoMode]", "ColourDepth", 32);
	bool useTripleBuffering = Config_GetBool("[VideoMode]", "UseTripleBuffering", false);

	// set some defaults
	int defaultDevice = D3DADAPTER_DEFAULT;
	bool windowed = false;
	D3DFORMAT SelectedDepthFormat = D3DFMT_D24S8;
	D3DFORMAT SelectedAdapterFormat = D3DFMT_X8R8G8B8;
	D3DFORMAT SelectedBackbufferFormat = D3DFMT_X8R8G8B8; // back buffer format

	if (WindowMode == WindowModeSubWindow) 
		windowed = true;

	//	Zero d3d structure
    ZeroMemory(&d3d, sizeof(D3DINFO));

	// Set up Direct3D interface object
	d3d.lpD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!d3d.lpD3D)
	{
		Con_PrintError("Could not create Direct3D9 object!");
		return FALSE;
	}

	// Get the number of devices/video cards in the system
	d3d.NumDrivers = d3d.lpD3D->GetAdapterCount();

	Con_PrintMessage("\t Found " + LogInteger(d3d.NumDrivers) + " video adapter(s)");

	// Get adapter information for all available devices (vid card name, etc)
	for (int i = 0; i < d3d.NumDrivers; i++)
	{
		LastError = d3d.lpD3D->GetAdapterIdentifier(i, /*D3DENUM_WHQL_LEVEL*/0, &d3d.Driver[i].AdapterInfo);
		if (FAILED(LastError)) 
		{
			LogDxError(LastError, __LINE__, __FILE__);
			return FALSE;
		}
	}

	Con_PrintMessage("\t Using device: " + std::string(d3d.Driver[defaultDevice].AdapterInfo.Description) + " on display " + std::string(d3d.Driver[defaultDevice].AdapterInfo.DeviceName));

	// chose an adapter format. take from desktop if windowed.
	if (windowed)
	{
		D3DDISPLAYMODE d3ddm;
		LastError = d3d.lpD3D->GetAdapterDisplayMode(defaultDevice, &d3ddm);
		if (FAILED(LastError))
		{
			Con_PrintError("GetAdapterDisplayMode call failed!");
			LogDxError(LastError, __LINE__, __FILE__);
			return FALSE;
		}
		else 
		{
			SelectedAdapterFormat = d3ddm.Format;
		}
	}
	else
	{
		// i assume we shouldn't be guessing this?
		SelectedAdapterFormat = D3DFMT_X8R8G8B8;
	}

	// count number of display formats in our array
	int numDisplayFormats = sizeof(DisplayFormats) / sizeof(DisplayFormats[0]);

	int numFomats = 0;

	// loop through all the devices, getting the list of formats available for each
	for (int thisDevice = 0; thisDevice < d3d.NumDrivers; thisDevice++)
	{
		d3d.Driver[thisDevice].NumModes = 0;

		for (int displayFormatIndex = 0; displayFormatIndex < numDisplayFormats; displayFormatIndex++) 
		{
			// get the number of display moves available on this adapter for this particular format
			int numAdapterModes = d3d.lpD3D->GetAdapterModeCount(thisDevice, DisplayFormats[displayFormatIndex]);

			for (int modeIndex = 0; modeIndex < numAdapterModes; modeIndex++) 
			{
				D3DDISPLAYMODE DisplayMode;

				// does this adapter support the requested format?
				d3d.lpD3D->EnumAdapterModes(thisDevice, DisplayFormats[displayFormatIndex], modeIndex, &DisplayMode);

				// Filter out low-resolution modes
				if (DisplayMode.Width < 640 || DisplayMode.Height < 480)
					continue;

				int j = 0;
				
				// Check if the mode already exists (to filter out refresh rates)
				for(; j < d3d.Driver[thisDevice].NumModes; j++ ) 
				{
					if (( d3d.Driver[thisDevice].DisplayMode[j].Width  == DisplayMode.Width ) &&
						( d3d.Driver[thisDevice].DisplayMode[j].Height == DisplayMode.Height) &&
						( d3d.Driver[thisDevice].DisplayMode[j].Format == DisplayMode.Format))
							break;
				}

				// If we found a new mode, add it to the list of modes
				if (j == d3d.Driver[thisDevice].NumModes) // we made it all the way throught the list and didn't find a match
				{
					assert (DisplayMode.Width != 0);
					assert (DisplayMode.Height != 0);

					d3d.Driver[thisDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Width       = DisplayMode.Width;
					d3d.Driver[thisDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Height      = DisplayMode.Height;
					d3d.Driver[thisDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].Format      = DisplayMode.Format;
					d3d.Driver[thisDevice].DisplayMode[d3d.Driver[thisDevice].NumModes].RefreshRate = DisplayMode.RefreshRate;

					d3d.Driver[thisDevice].NumModes++;

					int f = 0;

					// Check if the mode's format already exists
					for (int f = 0; f < numFomats; f++ ) 
					{
						if (DisplayMode.Format == d3d.Driver[thisDevice].Formats[f])
							break;
					}

					// If the format is new, add it to the list
					if (f == numFomats)
						d3d.Driver[thisDevice].Formats[numFomats++] = DisplayMode.Format;
				}
			}
		}
	}
	
	// check that the resolution and colour depth the user wants is supported
	bool gotOne = false;
	bool gotValidFormats = false;

	for (int i = 0; i < (sizeof(DisplayFormats) / sizeof(DisplayFormats[0])); i++)
	{
		for (int j = 0; i < d3d.Driver[defaultDevice].NumModes; j++)
		{
			// found a usable mode
			if ((d3d.Driver[defaultDevice].DisplayMode[j].Width == width) && (d3d.Driver[defaultDevice].DisplayMode[j].Height == height))
			{
				// try find a matching depth buffer format
				for (int d = 0; d < (sizeof(DepthFormats) / sizeof(DepthFormats[0])); d++)
				{
					LastError = d3d.lpD3D->CheckDeviceFormat( d3d.CurrentDriver, 
												D3DDEVTYPE_HAL, 
												SelectedAdapterFormat, 
												D3DUSAGE_DEPTHSTENCIL, 
												D3DRTYPE_SURFACE,
												DepthFormats[d]);

					// if the format wont work with this depth buffer, try another format
					if (FAILED(LastError)) 
						continue;

					LastError = d3d.lpD3D->CheckDepthStencilMatch( d3d.CurrentDriver,
											D3DDEVTYPE_HAL,
											SelectedAdapterFormat,
											DisplayFormats[i],
											DepthFormats[d]);

					// we got valid formats
					if (!FAILED(LastError))
					{
						SelectedDepthFormat = DepthFormats[d];
						SelectedBackbufferFormat = DisplayFormats[i];
						gotValidFormats = true;
						gotOne = true;
						//break;
						goto blah;
					}
				}
			}
		}
	}

	blah:

#if 0

		// check for a match on height and width
		if ((d3d.Driver[defaultDevice].DisplayMode[i].Width == width) && (d3d.Driver[defaultDevice].DisplayMode[i].Height == height))
		{
			// we found something that matches the users settings
			if (colourDepth == 32)
			{
				SelectedDisplayFormat = DisplayFormats32[0]; // use the first in the list

				// try find a matching depth buffer format
				for (int j = 0; j < (sizeof(DepthFormats) / sizeof(DepthFormats[0])); j++)
				{
					LastError = d3d.lpD3D->CheckDeviceFormat( d3d.CurrentDriver, 
												D3DDEVTYPE_HAL, 
												SelectedAdapterFormat, 
												D3DUSAGE_DEPTHSTENCIL, 
												D3DRTYPE_SURFACE,
												DepthFormats[j]);

					// if the format wont work with this depth buffer, try another format
					if (FAILED(LastError)) 
						continue;

					LastError = d3d.lpD3D->CheckDepthStencilMatch( d3d.CurrentDriver,
											D3DDEVTYPE_HAL,
											SelectedAdapterFormat,
											SelectedDisplayFormat,
											DepthFormats[j]);

					// we got valid formats
					if (!FAILED(LastError))
					{
						SelectedDepthFormat = DepthFormats[j];
						//SelectedDisplayFormat = DisplayFormats[i];
						gotValidFormats = true;
						break;
					}
				}
			}
			else if (colourDepth == 16)
			{
				SelectedDisplayFormat = DisplayFormats16[0]; // use the first in the list
			}
			gotOne = true;
			break;
		}
	}
#endif
	if (!gotOne)
	{
		Con_PrintError("Couldn't find match for user requested resolution!");

		// set some default values?
		width = 800;
		height = 600;
		SelectedBackbufferFormat = DisplayFormats32[0]; // use the first in the list
	}

	// set up the presentation parameters
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory (&d3dpp, sizeof(d3dpp));
	d3dpp.hDeviceWindow = hWndMain;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = SelectedDepthFormat;
	d3dpp.BackBufferFormat = SelectedBackbufferFormat;

	if (windowed) 
	{
		d3dpp.Windowed = TRUE;
		d3dpp.BackBufferWidth = width;
		d3dpp.BackBufferHeight = height;
		// setting this to interval one will cap the framerate to monitor refresh
		// the timer goes a bit mad if this isnt capped!
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		ChangeWindowsSize(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight);
	}
	else 
	{
		d3dpp.Windowed = FALSE;
		d3dpp.BackBufferWidth = width;
		d3dpp.BackBufferHeight = height;
		// setting this to interval one will cap the framerate to monitor refresh
		// the timer goes a bit mad if this isnt capped!
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
//		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
//		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		ChangeWindowsSize(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight);
	}

	d3dpp.BackBufferCount = 1;

	if (useTripleBuffering)
	{
		d3dpp.BackBufferCount = 2;
		d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
		Con_PrintMessage("Using triple buffering");
	}

	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	// store the index of the driver we want to use
	d3d.CurrentDriver = defaultDevice;

#if 0
	// try to get a depth format that'll work with the selected display format
	for (int i = 0; i < (sizeof(DisplayFormats) / sizeof(DisplayFormats[0])); i++)
	{
		// if we're found a usable backbuffer and depth buffer format, break out of the two loops
		if (gotValidFormats) 
			break;

		for (int j = 0; j < (sizeof(DepthFormats) / sizeof(DepthFormats[0])); j++)
		{
			LastError = d3d.lpD3D->CheckDeviceFormat( d3d.CurrentDriver, 
										D3DDEVTYPE_HAL, 
										SelectedDepthFormat, 
										D3DUSAGE_DEPTHSTENCIL, 
										D3DRTYPE_SURFACE,
										DepthFormats[j]);

			// if the format wont work with this depth buffer, try another format
			if (FAILED(LastError)) 
				continue;

			LastError = d3d.lpD3D->CheckDepthStencilMatch( d3d.CurrentDriver,
									D3DDEVTYPE_HAL,
									SelectedDepthFormat,
									DisplayFormats[i],
									DepthFormats[j]);

			// we got valid formats
			if (!FAILED(LastError))
			{
				SelectedDepthFormat = DepthFormats[j];
				SelectedDisplayFormat = DisplayFormats[i];
				gotValidFormats = true;
				break;
			}
		}
	}

	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = SelectedDepthFormat;
	d3dpp.BackBufferFormat = SelectedDisplayFormat;
#endif

#if 0 // NVidia PerfHUD
	UINT adapter_to_use;

	for (UINT Adapter = 0; Adapter < d3d.lpD3D->GetAdapterCount(); Adapter++)
	{
		D3DADAPTER_IDENTIFIER9 Identifier;
		HRESULT Res;

	Res = d3d.lpD3D->GetAdapterIdentifier(Adapter, 0, &Identifier);
 
	if (strstr(Identifier.Description, "PerfHUD") != 0)
	{
		adapter_to_use = Adapter;

		d3d.lpD3D->CreateDevice(Adapter, D3DDEVTYPE_REF, hWndMain,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	} 
	else 
	{
		LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	}
}
#endif

//#define USEREFDEVICE

#ifdef USEREFDEVICE
	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWndMain,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
#else

	// create D3DCREATE_PUREDEVICE
/*
	LastError = d3d.lpD3D->CreateDevice(defaultDevice, D3DDEVTYPE_HAL, hWndMain,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &d3dpp, &d3d.lpD3DDevice);

	if (FAILED(LastError))
*/
	{
		LastError = d3d.lpD3D->CreateDevice(defaultDevice, D3DDEVTYPE_HAL, hWndMain,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	}
	if (FAILED(LastError)) 
	{
		LastError = d3d.lpD3D->CreateDevice(defaultDevice, D3DDEVTYPE_HAL, hWndMain,
			D3DCREATE_MIXED_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	}
	if (FAILED(LastError)) 
	{
		LastError = d3d.lpD3D->CreateDevice(defaultDevice, D3DDEVTYPE_HAL, hWndMain,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	}
#endif
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	// get device capabilities
	D3DCAPS9 d3dCaps;
	d3d.lpD3DDevice->GetDeviceCaps(&d3dCaps);

	// check and remember if we have dynamic texture support
	if (d3dCaps.Caps2 & D3DCAPS2_DYNAMICTEXTURES)
	{
		d3d.supportsDynamicTextures = TRUE;
	}
	else 
	{
		d3d.supportsDynamicTextures = FALSE;
		Con_PrintError("Device can't use D3DUSAGE_DYNAMIC");
	}

	// Log resolution set
	Con_PrintMessage("\t Resolution set: " + LogInteger(d3dpp.BackBufferWidth) + " x " + LogInteger(d3dpp.BackBufferHeight));

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

	// Log stencil buffer format set
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
	ScreenDescriptorBlock.SDB_Depth		= colourDepth;
	ScreenDescriptorBlock.SDB_Size      = width*height;
	ScreenDescriptorBlock.SDB_CentreX   = width/2;
	ScreenDescriptorBlock.SDB_CentreY   = height/2;

	ScreenDescriptorBlock.SDB_ProjX     = width/2;
	ScreenDescriptorBlock.SDB_ProjY     = height/2;

	ScreenDescriptorBlock.SDB_ClipLeft  = 0;
	ScreenDescriptorBlock.SDB_ClipRight = width;
	ScreenDescriptorBlock.SDB_ClipUp    = 0;
	ScreenDescriptorBlock.SDB_ClipDown  = height;

	// use an offset for hud items to account for tv safe zones. just use width for now. 15%?
	if (0)
	{
		ScreenDescriptorBlock.SDB_SafeZoneWidthOffset = (width / 100) * 15;
		ScreenDescriptorBlock.SDB_SafeZoneHeightOffset = (height / 100) * 15;
	}
	else 
	{
		ScreenDescriptorBlock.SDB_SafeZoneWidthOffset = 0;
		ScreenDescriptorBlock.SDB_SafeZoneHeightOffset = 0;
	}

	// save a copy of the presentation parameters for use later (device reset, resolution/depth change)
	d3d.d3dpp = d3dpp;

	// create vertex and index buffers
	CreateVolatileResources();

	// Setup orthographic projection matrix
//	int standardWidth = 640;
//	int wideScreenWidth = 852;

	// setup view matrix
	D3DXMatrixIdentity( &matView );
	D3DXVECTOR3 position = D3DXVECTOR3(-64.0f, 280.0f, -1088.0f);
	D3DXVECTOR3 lookAt = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	D3DXMatrixLookAtLH( &matView, &position, &lookAt, &up );
	PrintD3DMatrix("View", matView);

	// set up orthographic projection matrix
//	D3DXMatrixOrthoOffCenterLH( &matOrtho, 0.0f, wideScreenWidth, 0.0f, 480, 1.0f, 10.0f);
	D3DXMatrixOrthoLH( &matOrtho, 2.0f, -2.0f, 1.0f, 10.0f);
	// set up projection matrix
	D3DXMatrixPerspectiveFovLH( &matProjection, width / height, D3DX_PI / 2, 1.0f, 100.0f);

	// print projection matrix?
	PrintD3DMatrix("Projection", matProjection);

	// multiply view and projection
	D3DXMatrixMultiply( &matViewProjection, &matView, &matProjection);
	PrintD3DMatrix("View and Projection", matViewProjection);

	D3DXMatrixIdentity( &matIdentity );
	d3d.lpD3DDevice->SetTransform( D3DTS_PROJECTION, &matOrtho );
	d3d.lpD3DDevice->SetTransform( D3DTS_WORLD, &matIdentity );
	d3d.lpD3DDevice->SetTransform( D3DTS_VIEW, &matIdentity );

	Con_Init();
	Net_Initialise();
	Font_Init();

	Con_AddCommand("dumptex", WriteMenuTextures);

	Con_PrintMessage("Initialised Direct3D9 succesfully");
	return TRUE;
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

	// delete up any new()-ed memory in d3d_render.cpp
	DeleteRenderMemory();

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

void ReleaseD3DTexture(D3DTEXTURE *d3dTexture) 
{
	// release d3d texture
	SAFE_RELEASE(*d3dTexture);
}

void FlushD3DZBuffer()
{
	d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

} // For extern "C"