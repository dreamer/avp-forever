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
#include "kshape.h"
#include "networking.h"

#include "avp_menugfx.hpp"
extern AVPMENUGFX AvPMenuGfxStorage[];
extern void ReleaseAllFMVTextures(void);

extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);

extern "C++" {
	#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
	#include "logString.h"
	#include "configFile.h"
	#include <xtl.h>
	#include <xgraphics.h>
	#include "console.h"
	extern void Font_Init();
	extern void Font_Release();
	D3DXMATRIX matOrtho;
	D3DXMATRIX matProjection;
	D3DXMATRIX matViewProjection;
	D3DXMATRIX matView;
	D3DXMATRIX matIdentity;
}

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int WindowMode;
extern int VideoModeColourDepth;

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

D3DTEXTURE CreateD3DTallFontTexture (AVPTEXTURE *tex)
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;
	LPDIRECT3DTEXTURE8 swizTexture = NULL;
	D3DLOCKED_RECT	   lock;

	// default colour format
	D3DFORMAT colourFormat = D3DFMT_R5G6B5;

	if (ScreenDescriptorBlock.SDB_Depth == 16)
	{
		colourFormat = D3DFMT_LIN_R5G6B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32)
	{
		colourFormat = D3DFMT_LIN_A8R8G8B8;
	}

	int height = 495;
	int width = 450;

	int padWidth = 512;
	int padHeight = 512;

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

	if (ScreenDescriptorBlock.SDB_Depth == 16)
	{
		uint16_t *destPtr;
		uint8_t  *srcPtr;

		srcPtr = (uint8_t*)tex->buffer;

		D3DCOLOR padColour = D3DCOLOR_XRGB(0,0,0);

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

		D3DCOLOR pad_colour = D3DCOLOR_XRGB(0,0,0);

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

D3DTEXTURE CreateFmvTexture(int *width, int *height, int usage, int pool)
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

//	D3DXCheckTextureRequirements(d3d.lpD3DDevice, (UINT*)width, (UINT*)height, NULL, 0, NULL, (D3DPOOL)pool);

	LastError = d3d.lpD3DDevice->CreateTexture(newWidth, newHeight, 1, usage, D3DFMT_LIN_X8R8G8B8, 0, &destTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	*width = newWidth;
	*height = newHeight;

	return destTexture;
}

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

D3DTEXTURE CreateD3DTexture(AVPTEXTURE *tex, unsigned char *buf, int usage, D3DPOOL poolType)
{
	D3DTEXTURE destTexture = NULL;

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

	LastError = D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		tex->width,
		tex->height,
		1,
		0,
		D3DFMT_A8R8G8B8,
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

/* called from Scrshot.cpp */
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

LPDIRECT3DTEXTURE8 CheckAndLoadUserTexture(const char *fileName, int *width, int *height)
{
	LPDIRECT3DTEXTURE8	tempTexture = NULL;
	D3DXIMAGE_INFO		imageInfo;

	std::string fullFileName(fileName);
	fullFileName += ".png";

	/* try find a png file at given path */
	LastError = D3DXCreateTextureFromFileEx(d3d.lpD3DDevice,
		fullFileName.c_str(),
		D3DX_DEFAULT,	// width
		D3DX_DEFAULT,	// height
		1,	// mip levels
		0,				// usage
		D3DFMT_UNKNOWN,	// format
		D3DPOOL_MANAGED,
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&imageInfo,
		NULL,
		&tempTexture
		);

	if(FAILED(LastError))
	{
//		OutputDebugString("\n couldn't create texture");
		*width = 0;
		*height = 0;
		return NULL;
	}

	OutputDebugString("\n made texture: ");
	OutputDebugString(fullFileName.c_str());

	*width = imageInfo.Width;
	*height = imageInfo.Height;
	return tempTexture;
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

const int MAX_VERTEXES = 4096 * 2;
//const int MAX_INDICES = 9216 * 2;

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
	LastError = d3d.lpD3DDevice->SetVertexShader(D3DFVF_TLVERTEX);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

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

BOOL ChangeGameResolution(int width, int height, int colourDepth)
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

		OutputDebugString(DXGetErrorString8(LastError));
		OutputDebugString(DXGetErrorDescription8(LastError));
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

	ThisFramesRenderingHasBegun();

	// set up projection matrix
	D3DXMatrixPerspectiveFovLH( &matProjection, width / height, D3DX_PI / 2, 1.0f, 100.0f);

	d3d.lpD3DDevice->SetTransform( D3DTS_PROJECTION, &matOrtho );
	d3d.lpD3DDevice->SetTransform( D3DTS_WORLD, &matIdentity );
	d3d.lpD3DDevice->SetTransform( D3DTS_VIEW, &matIdentity );

	return TRUE;
}

BOOL InitialiseDirect3D()
{
	// clear log file first, then write header text
	ClearLog();
	Con_PrintMessage("Starting to initialise Direct3D");

	int width = 640;
	int height = 480;
	int depth = 32;
	int defaultDevice = D3DADAPTER_DEFAULT;
	int thisDevice = D3DADAPTER_DEFAULT;

	//	Zero d3d structure
    memset(&d3d, 0, sizeof(D3DINFO));

	//  Set up Direct3D interface object
	d3d.lpD3D = Direct3DCreate8(D3D_SDK_VERSION);
	if (!d3d.lpD3D)
	{
		LogErrorString("Could not create Direct3D object \n");
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

	unsigned int modeCount = d3d.lpD3D->GetAdapterModeCount(defaultDevice);
	D3DDISPLAYMODE tempMode;

	// create our list of supported resolutions for D3DFMT_LIN_X8R8G8B8 format
	for (int i = 0; i < modeCount; i++)
	{
		LastError = d3d.lpD3D->EnumAdapterModes( defaultDevice, i, &tempMode );
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
	d3dpp.BackBufferFormat = D3DFMT_LIN_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
//	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
//	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE_OR_IMMEDIATE;

	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);

	if (FAILED(LastError))
	{
		LogErrorString("Could not create Direct3D device");
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
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
	ScreenDescriptorBlock.SDB_Depth		= depth;
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

	// Setup orthographic projection matrix
	int standardWidth = 640;
	int wideScreenWidth = 852;

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

// For extern "C"
};