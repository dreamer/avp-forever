// Interface  functions (written in C++) for
// Direct3D immediate mode system

// Must link to C code in main engine system

extern "C" {

// Mysterious definition required by objbase.h
// (included via one of the include files below)
// to start definition of obscure unique in the
// universe IDs required  by Direct3D before it
// will deign to cough up with anything useful...

#define INITGUID

#include "3dc.h"
#include "awTexLd.h"
#include "module.h"
#include "d3_func.h"

#include "kshape.h"
#include "eax.h"
#include "vmanpset.h"

extern "C++" {
	#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
	#include "logString.h"
	#include <d3dx9.h>
	#include "console.h"

#ifdef _DEBUG
	#pragma comment(lib, "d3dx9d.lib")
#else
	#pragma comment(lib, "d3dx9.lib")
#endif
}

int image_num = 0;

BOOL BilinearTextureFilter = 1;

extern HWND hWndMain;
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern void DeleteRenderMemory();
extern int WindowMode;
extern int ZBufferMode;
extern int ScanDrawMode;
extern int VideoModeColourDepth;

HRESULT LastError;

D3DINFO d3d;
D3DTEXTURE consoleText;

int StartDriver = 0;
int StartFormat = 0;

/* TGA header structure */
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

TGA_HEADER TgaHeader = {0};

void ColourFillBackBuffer(int FillColour) 
{
	D3DCOLOR colour = FillColour;
	d3d.lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, colour, 1.0f, 0 );
}

char* GetDeviceName() 
{
	if (d3d.AdapterInfo.Description != NULL) 
	{
		return d3d.AdapterInfo.Description;
	}
	else return "Default Adapter";
}

// list of allowed display formats
const D3DFORMAT DisplayFormats[] =
{
	D3DFMT_A8R8G8B8,
	D3DFMT_X8R8G8B8,
	D3DFMT_R8G8B8,
	D3DFMT_A1R5G5B5,
	D3DFMT_R5G6B5,
	D3DFMT_X1R5G5B5
};

// list of allowed depth buffer formats
const D3DFORMAT StencilFormats[] =
{
	D3DFMT_D24S8,
	D3DFMT_D32,
	D3DFMT_D24FS8,
	D3DFMT_D16
};

D3DFORMAT SelectedDepthFormat = D3DFMT_D24S8;
D3DFORMAT SelectedAdapterFormat = D3DFMT_X8R8G8B8;
D3DFORMAT SelectedTextureFormat = D3DFMT_A8R8G8B8;

bool usingStencil = false;

void LoadConsoleFont()
{
	D3DXIMAGE_INFO		imageInfo;

	/* try find a png file at given path */
	LastError = D3DXCreateTextureFromFileEx(d3d.lpD3DDevice, 
		"aa_font.png", 
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
		&consoleText
		);	

	if(FAILED(LastError))
	{
		OutputDebugString("Couldn't load console font!\n");
		consoleText = NULL;
	}
	else
	{	
		OutputDebugString("loaded console font ok\n");
	}
}

LPDIRECT3DTEXTURE9 CheckAndLoadUserTexture(const char *fileName, int *width, int *height)
{
	LPDIRECT3DTEXTURE9	tempTexture = NULL;
	D3DXIMAGE_INFO		imageInfo;

	std::string fullFileName(fileName);
	fullFileName += ".png";

	/* try find a png file at given path */
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

	if(FAILED(LastError))
	{
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

/* called from Scrshot.cpp */
void CreateScreenShotImage()
{
	SYSTEMTIME systemTime;
	LPDIRECT3DSURFACE9 frontBuffer = NULL;

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
	if(FAILED(d3d.lpD3DDevice->CreateOffscreenPlainSurface(ScreenDescriptorBlock.SDB_Width, ScreenDescriptorBlock.SDB_Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &frontBuffer, NULL)))
	{
		OutputDebugString("\n Couldn't create screenshot surface");
		return;
	}

	/* copy front buffer screen to surface */
	if(FAILED(d3d.lpD3DDevice->GetFrontBufferData(0, frontBuffer))) 
	{
		OutputDebugString("\n Couldn't get a copy of the front buffer");
		SAFE_RELEASE(frontBuffer);
		return;
	}

	/* save surface to image file */
	if(FAILED(D3DXSaveSurfaceToFile(fileName.str().c_str(), D3DXIFF_JPG, frontBuffer, NULL, NULL))) 
	{
		OutputDebugString("\n Save Surface to file failed!!!");
	}

	/* release surface */
	SAFE_RELEASE(frontBuffer);
}

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

LPDIRECT3DTEXTURE9 CreateD3DTallFontTexture(AvPTexture *tex) 
{
	LPDIRECT3DTEXTURE9 destTexture = NULL;

	// default colour format
	D3DFORMAT colourFormat = D3DFMT_R5G6B5;

	if (ScreenDescriptorBlock.SDB_Depth == 16) {
		colourFormat = D3DFMT_R5G6B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) {
		colourFormat = D3DFMT_A8R8G8B8;
	}

	int width = 450;
	int height = 495;

	int padWidth = 512;
	int padHeight = 512;
	

	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, NULL, colourFormat, D3DPOOL_MANAGED, &destTexture, NULL);
	if(FAILED(LastError)) {
//		LogDxError("Unable to create tall font texture", LastError);
		LogDxError(LastError);
		return NULL;
	}

	D3DLOCKED_RECT lock;

	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if(FAILED(LastError)) {
		destTexture->Release();
//		LogDxError("Unable to lock tall font texture for writing", LastError);
		LogDxError(LastError);
		return NULL;
	}

	if (ScreenDescriptorBlock.SDB_Depth == 16) 
	{
		unsigned short *destPtr;
		unsigned char *srcPtr;

		srcPtr = (unsigned char *)tex->buffer;

		D3DCOLOR padColour = D3DCOLOR_XRGB(0,0,0);

		// lets pad the whole thing black first
		for (int y = 0; y < padHeight; y++)
		{
			destPtr = ((unsigned short *)(((unsigned char *)lock.pBits) + y*lock.Pitch));

			for (int x = 0; x < padWidth; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*destPtr = RGB16(padColour, padColour, padColour);
/*
				*destPtr =	((pad_colour>>3)<<11) | // R
						((pad_colour>>2)<<5 ) | // G
						((pad_colour>>3)); // B
*/
				destPtr+=1;
			}
		}

		int charWidth = 30;
		int charHeight = 33;

		for (int i = 0; i < 224; i++) 
		{
			int row = i / 15; // get row
			int column = i % 15; // get column from remainder value

			int offset = ((column * charWidth) *2) + ((row * charHeight) * lock.Pitch);

			destPtr = ((unsigned short *)(((unsigned char *)lock.pBits + offset)));

			for (int y = 0; y < charHeight; y++) 
			{
				destPtr = ((unsigned short *)(((unsigned char *)lock.pBits + offset) + (y*lock.Pitch)));

				for (int x = 0; x < charWidth; x++) {
					*destPtr = RGB16(srcPtr[0], srcPtr[1], srcPtr[2]);
/*
					*destPtr =	((srcPtr[0]>>3)<<11) | // R
						((srcPtr[1]>>2)<<5 ) | // G
						((srcPtr[2]>>3)); // B
*/
					destPtr+=1;
					srcPtr+=4;
				}
			}
		}
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) 
	{
		unsigned char *destPtr, *srcPtr;

		srcPtr = (unsigned char *)tex->buffer;

		D3DCOLOR padColour = D3DCOLOR_XRGB(0,0,0);

		// lets pad the whole thing black first
		for (int y = 0; y < padHeight; y++)
		{
			destPtr = (((unsigned char *)lock.pBits) + y*lock.Pitch);

			for (int x = 0; x < padWidth; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(padColour, padColour, padColour, padColour);
//					*destPtr = pad_colour;

				destPtr+=4;
			}
		}

		int charWidth = 30;
		int charHeight = 33;

		for (int i = 0; i < 224; i++) 
		{
			int row = i / 15; // get row
			int column = i % 15; // get column from remainder value

			int offset = ((column * charWidth) *4) + ((row * charHeight) * lock.Pitch);

			destPtr = (((unsigned char *)lock.pBits + offset));

			for (int y = 0; y < charHeight; y++) 
			{
				destPtr = (((unsigned char *)lock.pBits + offset) + (y*lock.Pitch));

				for (int x = 0; x < charWidth; x++) 
				{
//					*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], srcPtr[3]);

					if (srcPtr[0] == 0x00 && srcPtr[1] == 0x00 && srcPtr[2] == 0x00) {
						*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], 0x00);
					}
//					if (srcPtr[0] == 0xff && srcPtr[1] == 0xff && srcPtr[2] == 0xff) {
//						*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], 0xff);
//					}
					else {
						*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], 0xff);
					}

					destPtr+=4;
					srcPtr+=4;
				}
			}
		}
	}

	LastError = destTexture->UnlockRect(0);
	if(FAILED(LastError)) {
//		LogDxError("Unable to unlock tall font texture", LastError);
		LogDxError(LastError);
		return NULL;
	}
/*
	if(FAILED(D3DXSaveTextureToFileA("tallfont.png", D3DXIFF_PNG, destTexture, NULL))) {
		OutputDebugString("\n couldnt save tex to file");
	}
*/
	return destTexture;
}

LPDIRECT3DTEXTURE9 CreateFmvTexture(int width, int height, int usage, int pool)
{
	LPDIRECT3DTEXTURE9 destTexture = NULL;

	if(FAILED(d3d.lpD3DDevice->CreateTexture(width, height, 1, usage, D3DFMT_A8R8G8B8, (D3DPOOL)pool, &destTexture, NULL)))
	{
		OutputDebugString("CreateFmvTexture failed\n");
		return NULL;
	}

	return destTexture;
}


/* TODO: Pre-order source image data into ARGB order? */

// use this to make textures from non power of two images
LPDIRECT3DTEXTURE9 CreateD3DTexturePadded(AvPTexture *tex, int *real_height, int *real_width) 
{
	int original_width = tex->width;
	int original_height = tex->height;
	int new_width = 0;
	int new_height = 0;

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

	// set passed in width and height values to be used later
	(*real_height) = new_height;
	(*real_width) = new_width;

	LPDIRECT3DTEXTURE9 destTexture = NULL;
#if 1

	D3DXIMAGE_INFO image;
	image.Depth = 32;
	image.Width = tex->width;
	image.Height= tex->height;
	image.MipLevels = 1;
	image.Depth = D3DFMT_A8R8G8B8;

	D3DPOOL poolType = D3DPOOL_MANAGED;

	/* create and fill tga header */
	TGA_HEADER *TgaHeader = new TGA_HEADER;
	TgaHeader->idlength = 0;
	TgaHeader->x_origin = tex->width;
	TgaHeader->y_origin = tex->height;
	TgaHeader->colourmapdepth = 0;
	TgaHeader->colourmaplength = 0;
	TgaHeader->colourmaporigin = 0;
	TgaHeader->colourmaptype = 0;
	TgaHeader->datatypecode = 2;			// RGB
	TgaHeader->bitsperpixel = 32;
	TgaHeader->imagedescriptor = 0x20;		// set origin to top left
	TgaHeader->height = tex->height;
	TgaHeader->width = tex->width;

	/* size of raw image data */
	int imageSize = tex->height * tex->width * 4;

	/* create new buffer for header and image data */
	byte *buffer = new byte[sizeof(TGA_HEADER) + imageSize];

	/* copy header and image data to buffer */
	memcpy(buffer, TgaHeader, sizeof(TGA_HEADER));

	byte *imageData = buffer + sizeof(TGA_HEADER);

	/* loop, converting RGB to BGR for D3DX function */
	for (int i = 0; i < imageSize; i+=4)
	{
		// RGB
		// BGR
		imageData[i+2] = tex->buffer[i];
		imageData[i+1] = tex->buffer[i+1];
		imageData[i] = tex->buffer[i+2];
		imageData[i+3] = tex->buffer[i+3];
	}
#if 0
	/* create direct3d texture */
	if(FAILED(D3DXCreateTextureFromFileInMemory(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		&destTexture)))
	{
		OutputDebugString("\n no, didn't work");
		delete TgaHeader;
		delete[] buffer;
		return NULL;
	}
#endif

	if(FAILED(D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
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
		OutputDebugString("COULD NOT CREATE TEXTURE?\n");
		delete TgaHeader;
		delete[] buffer;
		return NULL;

	}

	delete TgaHeader;
	delete[] buffer;

	return destTexture;
#endif
#if 0	
	// default colour format
	D3DFORMAT colour_format = D3DFMT_R5G6B5;

	if (ScreenDescriptorBlock.SDB_Depth == 16) {
		colour_format = D3DFMT_R5G6B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) {
		colour_format = D3DFMT_A8R8G8B8;
	}

	LastError = d3d.lpD3DDevice->CreateTexture(new_width, new_height, 1, NULL, colour_format, D3DPOOL_MANAGED, &destTexture, NULL);
	if(FAILED(LastError)) {
//		LogDxError("Unable to create menu texture", LastError);
		LogDxError(LastError);
		OutputDebugString("\n Couldn't create padded texture");
		return NULL;
	}

	D3DLOCKED_RECT lock;
	
	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if(FAILED(LastError)) {
//		LogDxError("Unable to lock menu texture for writing", LastError);
		LogDxError(LastError);
		SAFE_RELEASE(destTexture);
//		destTexture->Release();
		return NULL;
	}

	if (ScreenDescriptorBlock.SDB_Depth == 16) {
		unsigned short *destPtr;
		unsigned char *srcPtr;
		
		srcPtr = (unsigned char *)tex->buffer;

		// lets pad the whole thing black first
		for (int y = 0; y < new_height; y++)
		{
			destPtr = ((unsigned short *)(((unsigned char *)lock.pBits) + y*lock.Pitch));

			for (int x = 0; x < new_width; x++)
			{
				*destPtr = RGB16(pad_colour, pad_colour, pad_colour);
				destPtr+=1;
			}
		}

		// then actual image data
		for (int y = 0; y < original_height; y++)
		{
			destPtr = ((unsigned short *)(((unsigned char *)lock.pBits) + y*lock.Pitch));

			for (int x = 0; x < original_width; x++)
			{
				*destPtr = RGB16(srcPtr[0], srcPtr[1], srcPtr[2]);

				destPtr+=1;
				srcPtr+=4;
			}
		}
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) { 
		unsigned char *srcPtr, *destPtr;
		
		srcPtr = (unsigned char *)tex->buffer;

		// lets pad the whole thing black first
		for (int y = 0; y < new_height; y++)
		{
			destPtr = (((unsigned char  *)lock.pBits) + y*lock.Pitch);

			for (int x = 0; x < new_width; x++)
			{
				*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(pad_colour,pad_colour,pad_colour,0xff);
				destPtr+=4;
			}
		}

		// then actual image data
		for (int y = 0; y < original_height; y++)
		{
			destPtr = (((unsigned char *)lock.pBits) + y*lock.Pitch);

//			memcpy(destPtr, srcPtr, original_width * 4);
//			srcPtr+=original_width * 4;

			for (int x = 0; x < original_width; x++)
			{
				*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], 255);

				destPtr+=4;
				srcPtr+=4;
			}
		}
	}

	LastError = destTexture->UnlockRect(0);
	if(FAILED(LastError)) {
		SAFE_RELEASE(destTexture);
//		LogDxError("Unable to unlock menu texture", LastError);
		LogDxError(LastError);
		return NULL;
	}

//	char char_buf[100];
//	sprintf(char_buf, "image_%d.png", image_num);
//	if(FAILED(D3DXSaveTextureToFileA(char_buf, D3DXIFF_PNG, destTexture, NULL))) {
//		OutputDebugString("couldnt save tex to file");
//	}

//	image_num++;

	return destTexture;
#endif
}

LPDIRECT3DTEXTURE9 CreateD3DTexture(AvPTexture *tex, unsigned char *buf, D3DPOOL poolType) 
{
	/* create our texture for returning */
	LPDIRECT3DTEXTURE9 destTexture = NULL;

	/* fill tga header */
	TgaHeader.idlength = 0;
	TgaHeader.x_origin = tex->width;
	TgaHeader.y_origin = tex->height;
	TgaHeader.colourmapdepth = 0;
	TgaHeader.colourmaplength = 0;
	TgaHeader.colourmaporigin = 0;
	TgaHeader.colourmaptype = 0;
	TgaHeader.datatypecode = 2;			// RGB
	TgaHeader.bitsperpixel = 32;
	TgaHeader.imagedescriptor = 0x20;	// set origin to top left
	TgaHeader.height = tex->height;
	TgaHeader.width = tex->width;

	/* size of raw image data */
	int imageSize = tex->height * tex->width * 4;

	/* create new buffer for header and image data */
	byte *buffer = new byte[sizeof(TGA_HEADER) + imageSize];

	/* copy header and image data to buffer */
	memcpy(buffer, &TgaHeader, sizeof(TGA_HEADER));

	byte *imageData = buffer + sizeof(TGA_HEADER);

	/* loop, converting RGB to BGR for D3DX function */
	for (int i = 0; i < imageSize; i+=4)
	{
		// RGB
		// BGR
		imageData[i+2] = buf[i];
		imageData[i+1] = buf[i+1];
		imageData[i] = buf[i+2];
		imageData[i+3] = buf[i+3];
	}

#if 0
	/* create direct3d texture */
	if(FAILED(D3DXCreateTextureFromFileInMemory(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		&destTexture)))
	{
		OutputDebugString("\n no, didn't work");
		delete TgaHeader;
		delete[] buffer;
		return NULL;
	}
#endif

	D3DXIMAGE_INFO image;
	image.Depth = 32;
	image.Width = tex->width;
	image.Height= tex->height;
	image.MipLevels = 1;
	image.Depth = D3DFMT_A8R8G8B8;

	if(FAILED(D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
		buffer,
		sizeof(TGA_HEADER) + imageSize,
		tex->width,
		tex->height,
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
		OutputDebugString("COULD NOT CREATE TEXTURE?\n");
		delete[] buffer;
		return NULL;
	}

	delete[] buffer;
	return destTexture;
#if 0
//	char char_buf[100];
//	sprintf(char_buf, "image_%d.tga", image_num);
//	if(FAILED(D3DXSaveTextureToFileA(char_buf, D3DXIFF_PNG, destTexture, NULL))) {
//		OutputDebugString("couldnt save tex to file");
//	}
//	D3DXSaveTextureToFile(char_buf, D3DXIFF_TGA, destTexture, 0);

	image_num++;

	// default colour format
	D3DFORMAT colour_format = D3DFMT_A1R5G5B5;

	if (ScreenDescriptorBlock.SDB_Depth == 16) {
		colour_format = D3DFMT_A1R5G5B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) {
		colour_format = D3DFMT_A8R8G8B8;
	}

	LastError = d3d.lpD3DDevice->CreateTexture(tex->width, tex->height, 1, NULL, colour_format, D3DPOOL_MANAGED, &destTexture, NULL);
	if(FAILED(LastError)) {
//		LogDxError("Unable to create in game texture", LastError);
		LogDxError(LastError);
		return NULL;
	}

	D3DLOCKED_RECT lock;

	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if(FAILED(LastError)) {
//		LogDxError("Unable to lock in game texture for writing", LastError);
		LogDxError(LastError);
		destTexture->Release();
		return NULL;
	}

	if ( ScreenDescriptorBlock.SDB_Depth == 16 ) {
		unsigned short *destPtr;
		unsigned char *srcPtr;

		srcPtr = (unsigned char *)buf;

		for (int y = 0; y < tex->height; y++)
		{
			destPtr = ((unsigned short *)(((unsigned char *)lock.pBits) + y*lock.Pitch));

			for (int x = 0; x < tex->width; x++)
			{
				*destPtr = ARGB1555(srcPtr[3], srcPtr[0], srcPtr[1], srcPtr[2]);

				destPtr+=1;
				srcPtr+=4;
			}
		}
	}
	if ( ScreenDescriptorBlock.SDB_Depth == 32 ) {
		unsigned char *destPtr, *srcPtr;

		srcPtr = (unsigned char *)buf;

		for (int y = 0; y < tex->height; y++)
		{
			destPtr = (((unsigned char *)lock.pBits) + y*lock.Pitch);

			for (int x = 0; x < tex->width; x++)
			{
				*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], srcPtr[3]);

				destPtr+=4;
				srcPtr+=4;
			}
		}
	}
//#endif
	LastError = destTexture->UnlockRect(0);
	if(FAILED(LastError)) {
//		LogDxError("Unable to unlock in game texture", LastError);
		LogDxError(LastError);
	}

//	char char_buf[100];
//	sprintf(char_buf, "image_%d.png", image_num);
//	if(FAILED(D3DXSaveTextureToFileA(char_buf, D3DXIFF_PNG, destTexture, NULL))) {
//		OutputDebugString("couldnt save tex to file");
//	}
	image_num++;

	delete TgaHeader;
	delete[] buffer;
	return destTexture;
#endif
}

/* defined in bink.c */
extern void ReleaseBinkTextures();
/* smacker.c */
extern void ReleaseAllFMVTextures(void);
extern int NumberOfFMVTextures;
//#include "smacker.h"
#include "fmv.h"
extern void SetupFMVTexture(FMVTEXTURE *ftPtr);
#define MAX_NO_FMVTEXTURES 10
extern FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];

extern int mainMenu;

/* size of vertex and index buffers */
const int MAX_VERTEXES = 4096;
const int MAX_INDICES = 9216;

BOOL ReleaseVolatileResources() 
{
	ReleaseAllFMVTexturesForDeviceReset();

	SAFE_RELEASE(d3d.lpD3DBackSurface);
	SAFE_RELEASE(d3d.lpD3DIndexBuffer);
	SAFE_RELEASE(d3d.lpD3DVertexBuffer);

	return TRUE;
}

BOOL CreateVolatileResources() 
{
	RecreateAllFMVTexturesAfterDeviceReset();

	/* create dynamic vertex buffer */
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(MAX_VERTEXES * sizeof(D3DTLVERTEX),D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_TLVERTEX, D3DPOOL_DEFAULT, &d3d.lpD3DVertexBuffer, NULL);
	if(FAILED(LastError)) {
		LogDxError(LastError);
		return FALSE;
	}

	/* create index buffer */
	LastError = d3d.lpD3DDevice->CreateIndexBuffer(MAX_INDICES * 3 * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &d3d.lpD3DIndexBuffer, NULL);
	if(FAILED(LastError)) {
//		OutputDebugString("Couldn't create Index Buffer");
//		LogDxError("Unable to create index buffer", LastError);
		LogDxError(LastError);
		return FALSE;
	}

	LastError = d3d.lpD3DDevice->SetStreamSource( 0, d3d.lpD3DVertexBuffer, 0, sizeof(D3DTLVERTEX));
	if (FAILED(LastError)){
//		OutputDebugString(" Couldn't set stream source");
//		LogDxError("Unable to set stream source", LastError);
		LogDxError(LastError);
		return FALSE;
	}

	LastError = d3d.lpD3DDevice->SetFVF(D3DFVF_TLVERTEX);
	if(FAILED(LastError)) {
//		LogDxError("Unable to set FVF", LastError);
		LogDxError(LastError);
		return FALSE;
	}

	SetExecuteBufferDefaults();

	return TRUE;
}

BOOL ChangeGameResolution(int width, int height, int colourDepth)
{
	ReleaseVolatileResources();

	/* location of this will need to be rechecked if we ever decide to add ingame res change.. */
//	NumberOfFMVTextures = 0;

	d3d.d3dpp.BackBufferHeight = height;
	d3d.d3dpp.BackBufferWidth = width;

	LastError = d3d.lpD3DDevice->Reset(&d3d.d3dpp);

	if(FAILED(LastError)) 
	{
		OutputDebugString("\n couldn't reset for res change");
		LogDxError(LastError);
		return FALSE;
	}
	else
	{
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
	}

	return TRUE;
}

/* need to redo all the enumeration code here, as it's not very good.. */
BOOL InitialiseDirect3DImmediateMode()
{
	/* clear log file first, then write header text */
	ClearLog();
	LogDxString("Starting to initialise Direct3D");

	int width = 640;
	int height = 480;
	int depth = 32;
	bool windowed = false;
	bool triple_buffer = false;

	if (WindowMode == WindowModeSubWindow) windowed = true;

	//	Zero d3d structure
    memset(&d3d, 0, sizeof(D3DINFO));

	/* Set up Direct3D interface object */
	d3d.lpD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d.lpD3D)
	{
		LogDxErrorString("Could not create Direct3D object\n");
		return FALSE;
	}

//#define D3DADAPTER_DEFAULT 1

	// Get the number of devices in the system
	d3d.NumDrivers = d3d.lpD3D->GetAdapterCount();

	LogDxString("\t Found " + LogInteger(d3d.NumDrivers) + " video adapter(s)");

	// Get adapter information
	LastError = d3d.lpD3D->GetAdapterIdentifier(D3DADAPTER_DEFAULT, D3DENUM_WHQL_LEVEL, &d3d.AdapterInfo);
	if(FAILED(LastError)) 
	{
		LogDxString("Could not get Adapter Identifier Information\n");
		return FALSE;
	}

	LogDxString("\t Using device: " + std::string(d3d.AdapterInfo.Description));

	/* taken from the DX SDK samples :) */

//	for (int i = 0; i < d3d.NumDrivers; i++) {
//		int num_adapter_modes = d3d.lpD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, D3DFMT_X8R8G8B8);
//	}

	// count number of display formats in our array
	int NumDisplayFormats = sizeof(DisplayFormats) / sizeof(DisplayFormats[0]);
	d3d.NumModes = 0;
	int num_fomats = 0;

	for (int CurrentDisplayFormat = 0; CurrentDisplayFormat < NumDisplayFormats; CurrentDisplayFormat++) 
	{
		// Get available display modes
		int num_adapter_modes = d3d.lpD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT, DisplayFormats[CurrentDisplayFormat]);

		for (int i = 0; i < num_adapter_modes; i++) 
		{
			D3DDISPLAYMODE DisplayMode;

			d3d.lpD3D->EnumAdapterModes( D3DADAPTER_DEFAULT, DisplayFormats[CurrentDisplayFormat], i, &DisplayMode );

			// Filter out low-resolution modes
			if( DisplayMode.Width  < 640 || DisplayMode.Height < 480 )
				continue;

			int j = 0;
			
			// Check if the mode already exists (to filter out refresh rates)
			for(; j < d3d.NumModes; j++ ) 
			{
				if( ( d3d.DisplayMode[j].Width  == DisplayMode.Width  ) &&
					( d3d.DisplayMode[j].Height == DisplayMode.Height ) &&
					( d3d.DisplayMode[j].Format == DisplayMode.Format ) )
						break;
			}

			// If we found a new mode, add it to the list of modes
			if( j == d3d.NumModes ) 
			{
				d3d.DisplayMode[d3d.NumModes].Width       = DisplayMode.Width;
				d3d.DisplayMode[d3d.NumModes].Height      = DisplayMode.Height;
				d3d.DisplayMode[d3d.NumModes].Format      = DisplayMode.Format;
				d3d.DisplayMode[d3d.NumModes].RefreshRate = 0;
				
				d3d.NumModes++;

				int f = 0;

				// Check if the mode's format already exists
				for(int f = 0; f < num_fomats; f++ ) 
				{
					if( DisplayMode.Format == d3d.Formats[f] )
						break;
				}

				// If the format is new, add it to the list
				if( f == num_fomats )
					d3d.Formats[num_fomats++] = DisplayMode.Format;
			}
		}
	}

	D3DDISPLAYMODE d3ddm;
	LastError = d3d.lpD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	if(FAILED(LastError)) 
	{
		LogDxErrorString("GetAdapterDisplayMode failed\n");
		return FALSE;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory( &d3dpp, sizeof(d3dpp) );
	d3dpp.hDeviceWindow = hWndMain;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	if(windowed) 
	{
		d3dpp.Windowed = TRUE;
		d3dpp.BackBufferWidth = 800;
		d3dpp.BackBufferHeight = 600;
		// setting this to interval one will cap the framerate to monitor refresh
		// the timer goes a bit mad if this isnt capped!
		d3dpp.PresentationInterval = 0;//D3DPRESENT_INTERVAL_IMMEDIATE;
		SetWindowPos( hWndMain, HWND_TOP, 0, 0, d3dpp.BackBufferWidth, d3dpp.BackBufferHeight, SWP_DRAWFRAME | SWP_FRAMECHANGED | SWP_SHOWWINDOW );
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
	}
	d3dpp.BackBufferCount = 1;

	if(triple_buffer)
	{
		d3dpp.BackBufferCount = 2;
		d3dpp.SwapEffect = D3DSWAPEFFECT_FLIP;
	}

	if (depth == 16)
	{
		d3dpp.BackBufferFormat = D3DFMT_R5G6B5; // 16 bit
	}
	if (depth == 32) 
	{
		d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8; // 32 bit
	}
	SelectedAdapterFormat  = d3dpp.BackBufferFormat;

//	d3dpp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	// setting this to interval one will cap the framerate to monitor refresh
	// the timer goes a bit mad if this isnt capped!
//	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;//D3DPRESENT_INTERVAL_IMMEDIATE;

	for( int i = 0; i < (sizeof(StencilFormats) / sizeof(StencilFormats[0])); i++) 
	{
		LastError = d3d.lpD3D->CheckDeviceFormat(D3DADAPTER_DEFAULT, 
									D3DDEVTYPE_HAL, 
									SelectedAdapterFormat, 
									D3DUSAGE_DEPTHSTENCIL, 
									D3DRTYPE_SURFACE,
									StencilFormats[i]);
		if(FAILED(LastError)) 
		{
			LogDxErrorString("CheckDeviceFormat failed\n");
			continue;
		}

		LastError = d3d.lpD3D->CheckDepthStencilMatch( D3DADAPTER_DEFAULT,
									D3DDEVTYPE_HAL,
									SelectedAdapterFormat,
									D3DFMT_X8R8G8B8,
									SelectedDepthFormat);

		if(FAILED(LastError))
		{
			LogDxErrorString("CheckDepthStencilMatch failed\n");
		}
		else
		{
			SelectedDepthFormat = StencilFormats[i];
			d3dpp.EnableAutoDepthStencil = true;
			break;
		}

		LogDxErrorString("couldn't get a usable depth buffer!\n");
		d3dpp.EnableAutoDepthStencil = false;
	}
	
/*	
	if(FAILED(LastError)) 
	{
		LogDxErrorString("Stencil and Depth format didn't match\n");
		d3dpp.EnableAutoDepthStencil = false;
	}
	else d3dpp.EnableAutoDepthStencil = true;
*/
	d3dpp.AutoDepthStencilFormat = SelectedDepthFormat;
//	if(SelectedDepthFormat == D3DFMT_D24S8) UsingStencil = true;

//	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;//SelectedDepthFormat;
//	if(SelectedDepthFormat == D3DFMT_D16) UsingStencil = true;

//#endif
#if 0 
	UINT adapter_to_use;

	for (UINT Adapter=0;Adapter<d3d.lpD3D->GetAdapterCount();Adapter++)
	{
		D3DADAPTER_IDENTIFIER9 Identifier;
		HRESULT Res;

	Res = d3d.lpD3D->GetAdapterIdentifier(Adapter, 0, &Identifier);
 
	if (strstr(Identifier.Description,"PerfHUD") != 0)
	{
		adapter_to_use = Adapter;
		//AdaptertoUse=Adapter;
		//DeviceType = D3DDEVTYPE_REF

		d3d.lpD3D->CreateDevice(Adapter, D3DDEVTYPE_REF, hWndMain,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	} 
	else {
		LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain,
		D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	}
}
#endif
/*
	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWndMain,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
*/	
	/* create D3DCREATE_PUREDEVICE */
	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &d3dpp, &d3d.lpD3DDevice);

	if(FAILED(LastError)) 
	{
		LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	}
	if(FAILED(LastError)) 
	{
		LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain,
			D3DCREATE_MIXED_VERTEXPROCESSING , &d3dpp, &d3d.lpD3DDevice);
	}
	if(FAILED(LastError)) 
	{
		LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);
	}

	if (FAILED(LastError)) 
	{
		LogDxErrorString("Could not create Direct3D device\n");
		return FALSE;
	}

	// Log resolution set
	LogDxString("\t Resolution set: " + LogInteger(d3dpp.BackBufferWidth) + " x " + LogInteger(d3dpp.BackBufferHeight));

	// Log format set
	switch(d3dpp.BackBufferFormat)
	{
		case D3DFMT_A8R8G8B8:
			LogDxString("\t Format set: 32bit - D3DFMT_A8R8G8B8");
			break;
		case D3DFMT_X8R8G8B8:
			LogDxString("\t Format set: 32bit - D3DFMT_X8R8G8B8");
			break;
		case D3DFMT_R8G8B8:
			LogDxString("\t Format set: 32bit - D3DFMT_R8G8B8");
			break;
		case D3DFMT_A1R5G5B5:
			LogDxString("\t Format set: 16bit - D3DFMT_A1R5G5B5");
			break;
		case D3DFMT_R5G6B5:
			LogDxString("\t Format set: 16bit - D3DFMT_R5G6B5");
			break;
		case D3DFMT_X1R5G5B5:
			LogDxString("\t Format set: 16bit - D3DFMT_X1R5G5B5");
			break;
		default:
			LogDxString("\t Format set: Unknown");
			break;
	}

	// Log format set
	switch(d3dpp.AutoDepthStencilFormat)
	{
		case D3DFMT_D24S8:
			LogDxString("\t Depth Format set: 24bit and 8bit stencil - D3DFMT_D24S8");
			break;
		case D3DFMT_D24X8:
			LogDxString("\t Depth Format set: 24bit and 0bit stencil - D3DFMT_D24X8");
			break;
		case D3DFMT_D32:
			LogDxString("\t Depth Format set: 32bit and 0bit stencil - D3DFMT_D32");
			break;
		case D3DFMT_D24FS8:
			LogDxString("\t Depth Format set: 24bit and 8bit stencil - D3DFMT_D32");
			break;
		default:
			LogDxString("\t Depth Format set: Unknown");
			break;
	}
	
	ZeroMemory( &d3d.lpD3DViewport, sizeof(d3d.lpD3DViewport) );
	d3d.lpD3DViewport.X = 0;
	d3d.lpD3DViewport.Y = 0;
	d3d.lpD3DViewport.Width = width;
	d3d.lpD3DViewport.Height = height;
	d3d.lpD3DViewport.MinZ = 0.0f;
	d3d.lpD3DViewport.MaxZ = 1.0f;

	LastError = d3d.lpD3DDevice->SetViewport(&d3d.lpD3DViewport);
	if (FAILED(LastError))
	{
		LogDxErrorString("Could not set viewport\n");
	}
/*
	LastError = d3d.lpD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &d3d.lpD3DBackSurface);

	if (FAILED(LastError))
	{
		LogDxErrorString("Could not get a copy of the back buffer\n");
		return FALSE;
	}

	LastError = d3d.lpD3DBackSurface->GetDesc(&d3d.backSurfaceDesc);

	if (FAILED(LastError))
	{
		LogDxErrorString("Could not get backbuffer surface description\n");
		return FALSE;
	}
*/
	ScreenDescriptorBlock.SDB_Width     = width;
	ScreenDescriptorBlock.SDB_Height    = height;
	ScreenDescriptorBlock.SDB_Depth		= depth;
	ScreenDescriptorBlock.SDB_Size      = width*height;
//	ScreenDescriptorBlock.SDB_DiagonalWidth = sqrt((float)width*width+height*height)+0.5;
	ScreenDescriptorBlock.SDB_CentreX   = width/2;
	ScreenDescriptorBlock.SDB_CentreY   = height/2;
	ScreenDescriptorBlock.SDB_ProjX     = width/2;
	ScreenDescriptorBlock.SDB_ProjY     = height/2;
	ScreenDescriptorBlock.SDB_ClipLeft  = 0;
	ScreenDescriptorBlock.SDB_ClipRight = width;
	ScreenDescriptorBlock.SDB_ClipUp    = 0;
	ScreenDescriptorBlock.SDB_ClipDown  = height;

	ScanDrawMode = ScanDrawD3DHardwareRGB;

	/* use an offset for hud items to account for tv safe zones. just use width for now. 15%?  */
	if(0)
	{
		ScreenDescriptorBlock.SDB_SafeZoneWidthOffset = (width / 100) * 15;
	}
	else ScreenDescriptorBlock.SDB_SafeZoneWidthOffset = 0;

	/* save a copy of the presentation parameters for use later (device reset, resolution/depth change) */
	d3d.d3dpp = d3dpp;

	/* create vertex and index buffers */
	CreateVolatileResources();

//	Con_Init();

	LogDxString("Initialise Direct3D succesfully");
	return TRUE;
}

void FlipBuffers()
{
	LastError = d3d.lpD3DDevice->Present(NULL, NULL, NULL, NULL);
	if (FAILED(LastError)) {
		OutputDebugString(" Present failed ");
	}
}

void InGameFlipBuffers()
{
	FlipBuffers();
}

// With a bit of luck this should automatically
// release all the Direct3D and  DirectDraw
// objects using their own functionality.
// A separate call to finiObjects
// is not required.
// NOTE!!! This depends on Microsoft macros
// in d3dmacs.h, which is in the win95 directory
// and must be upgraded from sdk upgrades!!!

void ReleaseDirect3D()
{
    DeallocateAllImages();

	// release back-buffer copy surface, vertex buffer and index buffer
	ReleaseVolatileResources();

	/* delete up any new()-ed memory in d3d_render.cpp */
	DeleteRenderMemory();

	SAFE_RELEASE(d3d.lpD3DDevice);
	SAFE_RELEASE(d3d.lpD3D);

	/* release Direct Input stuff */
	ReleaseDirectKeyboard();
	ReleaseDirectMouse();
	ReleaseDirectInput();
}

// reload D3D image -- assumes a base pointer points to the image loaded
// from disc, in a suitable format

void ReloadImageIntoD3DImmediateSurface(IMAGEHEADER* iheader)
{
    void *reloadedTexturePtr = ReloadImageIntoD3DTexture(iheader);
	LOCALASSERT(reloadedTexturePtr != NULL);

	int gotTextureHandle = GetTextureHandle(iheader);
	LOCALASSERT(gotTextureHandle == TRUE);
}

void* ReloadImageIntoD3DTexture(IMAGEHEADER* iheader)
{
	// NOTE FIXME BUG HACK
	// what if the image was a DD surface ??
#if 0
	if (iheader->hBackup)
	{
		iheader->AvPTexture = AwCreateTexture("rf",AW_TLF_PREVSRC|AW_TLF_COMPRESS);
		return iheader->AvPTexture;
	}
	else return NULL;
#endif

	return NULL;
}

int GetTextureHandle(IMAGEHEADER *imageHeaderPtr)
{
#if 0 // bjd
 	LPDIRECT3DTEXTURE Texture = (LPDIRECT3DTEXTURE) imageHeaderPtr->D3DTexture;


	LastError = Texture->GetHandle(d3d.lpD3DDevice, (D3DTEXTUREHANDLE*)&(imageHeaderPtr->D3DHandle));

	if (LastError != D3D_OK) return FALSE;

	imageHeaderPtr->D3DHandle = imageHeaderPtr->D3DTexture;
#endif
	return TRUE;
}

void ReleaseD3DTexture(void* texture)
{
	AvPTexture *TextureHandle = (AvPTexture *)texture;
	
	free(TextureHandle->buffer);
	free(TextureHandle);
}

void ReleaseD3DTexture8(LPDIRECT3DTEXTURE9 d3dTexture) 
{
	/* release d3d texture */
	SAFE_RELEASE(d3dTexture);
}

BOOL CreateD3DZBuffer()
{
	return TRUE;
}

#define ZFlushVal 0xffffffff

// At present we are using my z flush function, with the
// (undocumented) addition of a fill colour for the
// actual depth fill, since it seems to work and at least
// I know what it does.  If it starts failing we'll probably
// have to go back to invoking the viewport clear through
// Direct3D.

void FlushD3DZBuffer()
{
//	OutputDebugString("\n clear first buffer");
	d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

void SecondFlushD3DZBuffer()
{
//	OutputDebugString("\n clear second buffer");
	d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

/* bjd - commented out
void FlushZB()
{
	OutputDebugString(" FlushZB called ");
    HRESULT hRes;
    D3DRECT d3dRect;

    d3dRect.x1 = 0;
    d3dRect.x2 = 640;
    d3dRect.x1 = 0;
    d3dRect.x2 = 480;
//    hRes = d3d.lpD3DViewport->Clear(1, &d3dRect, D3DCLEAR_ZBUFFER);
	hRes = d3d.lpD3DDevice->Clear(1, &d3dRect, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255,255,255),1.0f, 0);
	if(FAILED(hRes)) {
		OutputDebugString("Couldn't FlushZB");
	}
}
*/

// For extern "C"

};