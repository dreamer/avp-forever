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

#include "dxlog.h"
#include "module.h"
#include "inline.h"

#include "d3_func.h"
#include "d3dmacs.h"

#include "string.h"

#include "kshape.h"
#include "eax.h"
#include "vmanpset.h"

extern "C++" {
	#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?

	#include <string>
	#include <fstream>
	#include <sstream>

	#include "logString.h"

	#ifdef WIN32
		#include <d3dx9.h>
		#pragma comment(lib, "d3dx9d.lib")
	#endif
	#ifdef _XBOX
		#include <xtl.h>
		#include <xgraphics.h>
	#endif
/*
	bool use_d3dx_tools = false;

	std::string LogInteger(int value);
	std::string logFilename = "dx_log.txt";
*/
}

int image_num = 0;

// Set to Yes to make default texture filter bilinear averaging rather
// than nearest
BOOL BilinearTextureFilter = 1;

//extern HWND hWndMain;

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int WindowMode;
extern int ZBufferMode;
extern int ScanDrawMode;
extern int VideoModeColourDepth;

HRESULT LastError;

D3DINFO d3d;
BOOL D3DHardwareAvailable;

int StartDriver;
int StartFormat;

/* TGA header structure */
#pragma pack(1)
struct TGA_HEADER {
	char idlength;
	char colourmaptype;
	char datatypecode;
	short int colourmaporigin;
	short int colourmaplength;
	char colourmapdepth;
	short int x_origin;
	short int y_origin;
	short width;
	short height;
	char bitsperpixel;
	char imagedescriptor;
};
#pragma pack()

void ColourFillBackBuffer(int FillColour) 
{
	d3d.lpD3DDevice->Clear( 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0 );
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
	D3DFMT_X8R8G8B8,
	D3DFMT_R5G6B5
};

// list of allowed depth buffer formats
const D3DFORMAT StencilFormats[] =
{
	D3DFMT_D24S8,
	D3DFMT_D16
};

D3DFORMAT SelectedDepthFormat = D3DFMT_D24S8;
D3DFORMAT SelectedAdapterFormat = D3DFMT_X8R8G8B8;
D3DFORMAT SelectedTextureFormat = D3DFMT_A8R8G8B8;

bool UsingStencil = false;

LPDIRECT3DSURFACE8 CreateD3DSurface(DDSurface *tex, int width, int height) {
#if 0
/*
	char buf[100];
	int size = sizeof(tex);

	sprintf(buf, "width: %d height: %d size: %d", width, height, size);
	OutputDebugString(buf);
*/
	LPDIRECT3DSURFACE8 tempSurface = NULL;

	if(FAILED(d3d.lpD3DDevice->CreateImageSurface(width, height, D3DFMT_R5G6B5, &tempSurface))) {
		OutputDebugString(" Couldn't create image surface");
		return NULL;
	}

/*
	if(FAILED(d3d.lpD3DDevice->CreateRenderTarget(width, height, D3DFMT_R5G6B5, D3DMULTISAMPLE_NONE, TRUE, &tempSurface))) {
		OutputDebugString(" Couldn't create render target surface");
		return NULL;
	}
*/
	D3DLOCKED_RECT lock;

	if(FAILED(tempSurface->LockRect(&lock, NULL, 0 ))){
		tempSurface->Release();
		OutputDebugString(" Couldn't LockRect for surface");
		return NULL;
	}

	unsigned short *destPtr;
	unsigned char *srcPtr;

	srcPtr = (unsigned char *)tex->buffer;

	for (int y = 0; y < height; y++)
	{
		destPtr = ((unsigned short *)(((unsigned char *)lock.pBits) + y*lock.Pitch));

		for (int x = 0; x < width; x++)
		{
			// >> 3 for red and blue in a 16 bit texture, 2 for green
			*destPtr =	((srcPtr[0]>>3)<<11) | // R
					((srcPtr[1]>>2)<<5 ) | // G
					((srcPtr[2]>>3)); // B

			destPtr+=1;
			srcPtr+=4;
		}
	}

	if(FAILED(tempSurface->UnlockRect())) {
		OutputDebugString(" Couldn't UnLockRect for Surface");
		return NULL;
	}

	return tempSurface;
#endif
	return NULL;
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

LPDIRECT3DTEXTURE8 CreateD3DTallFontTexture (AvPTexture *tex) 
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;
	LPDIRECT3DTEXTURE8 swizTexture = NULL;

	// default colour format
	D3DFORMAT colour_format = D3DFMT_R5G6B5;

	if (ScreenDescriptorBlock.SDB_Depth == 16) {
		colour_format = D3DFMT_R5G6B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) {
//		colour_format = D3DFMT_A8R8G8B8;
		colour_format = D3DFMT_LIN_A8R8G8B8;
	}

	int height = 495;
	int width = 450;

	int pad_height = 512;
	int pad_width = 512;

//	int size = 30 * 7392;
/*
	if(FAILED(d3d.lpD3DDevice->CreateTexture(pad_width, pad_height, 0, D3DUSAGE_DYNAMIC, D3DFMT_R5G6B5, D3DPOOL_SYSTEMMEM, &tempTexture))){
		return NULL;
	}

	if(FAILED(d3d.lpD3DDevice->CreateTexture(pad_width, pad_height, 0, D3DUSAGE_DYNAMIC, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &destTexture))){
		tempTexture->Release();
		return NULL;
	}
*/
	LastError = d3d.lpD3DDevice->CreateTexture(pad_width, pad_height, 1, NULL, colour_format, D3DPOOL_MANAGED, &destTexture);
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

		D3DCOLOR pad_colour = D3DCOLOR_XRGB(0,0,0);

		// lets pad the whole thing black first
		for (int y = 0; y < pad_height; y++)
		{
			destPtr = ((unsigned short *)(((unsigned char *)lock.pBits) + y*lock.Pitch));

			for (int x = 0; x < pad_width; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*destPtr = RGB16(pad_colour, pad_colour, pad_colour);
/*
				*destPtr =	((pad_colour>>3)<<11) | // R
						((pad_colour>>2)<<5 ) | // G
						((pad_colour>>3)); // B
*/
				destPtr+=1;
			}
		}

		int char_width = 30;
		int char_height = 33;

		for (int i = 0; i < 224; i++) 
		{
			int row = i / 15; // get row
			int column = i % 15; // get column from remainder value

			int offset = ((column * char_width) *2) + ((row * char_height) * lock.Pitch);

			destPtr = ((unsigned short *)(((unsigned char *)lock.pBits + offset)));

			for (int y = 0; y < char_height; y++) 
			{
				destPtr = ((unsigned short *)(((unsigned char *)lock.pBits + offset) + (y*lock.Pitch)));

				for (int x = 0; x < char_width; x++) {
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

		D3DCOLOR pad_colour = D3DCOLOR_XRGB(0,0,0);

		// lets pad the whole thing black first
		for (int y = 0; y < pad_height; y++)
		{
			destPtr = (((unsigned char *)lock.pBits) + y*lock.Pitch);

			for (int x = 0; x < pad_width; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(pad_colour, pad_colour, pad_colour, pad_colour);
//					*destPtr = pad_colour;

				destPtr+=4;
			}
		}

		int char_width = 30;
		int char_height = 33;

		for (int i = 0; i < 224; i++) 
		{
			int row = i / 15; // get row
			int column = i % 15; // get column from remainder value

			int offset = ((column * char_width) *4) + ((row * char_height) * lock.Pitch);

			destPtr = (((unsigned char *)lock.pBits + offset));

			for (int y = 0; y < char_height; y++) 
			{
				destPtr = (((unsigned char *)lock.pBits + offset) + (y*lock.Pitch));

				for (int x = 0; x < char_width; x++) 
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

	D3DLOCKED_RECT lock2;
	LastError = d3d.lpD3DDevice->CreateTexture(pad_width, pad_height, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &swizTexture);
	LastError = swizTexture->LockRect(0, &lock2, NULL, NULL );

	XGSwizzleRect(lock.pBits, lock.Pitch, NULL, lock2.pBits, pad_width, pad_height, NULL, 4);

	LastError = destTexture->UnlockRect(0);
	if(FAILED(LastError)) {
//		LogDxError("Unable to unlock tall font texture", LastError);
		LogDxError(LastError);
	}
	
	LastError = swizTexture->UnlockRect(0);
	destTexture->Release();

/*
	if(FAILED(D3DXSaveTextureToFileA("tallfont.png", D3DXIFF_PNG, destTexture, NULL))) {
		OutputDebugString("\n couldnt save tex to file");
	}
*/
	//	return destTexture;
	return swizTexture;
}

// use this to make textures from non power of two images
LPDIRECT3DTEXTURE8 CreateD3DTexturePadded(AvPTexture *tex,int *real_height, int *real_width) 
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

	if(new_height < 64) new_height = 64;
	if(new_width < 64) new_width = 64;

	// set passed in width and height values to be used later
	(*real_height) = new_height;
	(*real_width) = new_width;

//	char buf[100];
//	sprintf(buf,"Got in width: %d, height: %d and made a new tex, width: %d, height: %d",tex->w, tex->h, new_width, new_height);
//	OutputDebugString(buf);

//	LPDIRECT3DTEXTURE9 tempTexture = NULL;
	LPDIRECT3DTEXTURE8 destTexture = NULL;
	LPDIRECT3DTEXTURE8 swizTexture = NULL;
	
	// default colour format
	D3DFORMAT colour_format = D3DFMT_R5G6B5;

	if (ScreenDescriptorBlock.SDB_Depth == 16) {
		colour_format = D3DFMT_R5G6B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) {
//		colour_format = D3DFMT_A8R8G8B8;
		colour_format = D3DFMT_LIN_A8R8G8B8;
	}

	LastError = d3d.lpD3DDevice->CreateTexture(new_width, new_height, 1, NULL, colour_format, D3DPOOL_MANAGED, &destTexture);
	if(FAILED(LastError)) {
//		LogDxError("Unable to create menu texture", LastError);
		LogDxError(LastError);
//		OutputDebugString("\n Couldn't create padded texture");
		return NULL;
	}

	D3DLOCKED_RECT lock;
	
	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if(FAILED(LastError)) {
//		LogDxError("Unable to lock menu texture for writing", LastError);
		LogDxError(LastError);
		destTexture->Release();
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

			for (int x = 0; x < original_width; x++)
			{
				*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], 255);

				destPtr+=4;
				srcPtr+=4;
			}
		}
	}

	D3DLOCKED_RECT lock2;
	LastError = d3d.lpD3DDevice->CreateTexture(new_width, new_height, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &swizTexture);
	LastError = swizTexture->LockRect(0, &lock2, NULL, NULL );

	XGSwizzleRect(lock.pBits, lock.Pitch, NULL, lock2.pBits, new_width, new_height, NULL, 4);

	LastError = destTexture->UnlockRect(0);
	if(FAILED(LastError)) {
//		LogDxError("Unable to unlock menu texture", LastError);
		LogDxError(LastError);
	}

	LastError = swizTexture->UnlockRect(0);
	destTexture->Release();
/*
	if(FAILED(d3d.lpD3DDevice->UpdateTexture(tempTexture, destTexture))) {
		OutputDebugString("UpdateTexture failed \n");
	}
*/
//	char char_buf[100];
//	sprintf(char_buf, "image_%d.png", image_num);
//	if(FAILED(D3DXSaveTextureToFileA(char_buf, D3DXIFF_PNG, destTexture, NULL))) {
//		OutputDebugString("couldnt save tex to file");
//	}

//	image_num++;

/*
	tempTexture->Release();
	tempTexture = NULL;
*/
//	return destTexture;
	return swizTexture;
}

LPDIRECT3DTEXTURE8 CreateD3DTexture(AvPTexture *tex, unsigned char *buf) 
{
	/* create our texture for returning */
	LPDIRECT3DTEXTURE8 destTexture = NULL;

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
		D3DPOOL_DEFAULT, // not used for xbox
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&image,
		0,
		&destTexture)))
	{
		OutputDebugString("\n no, didn't work");
		delete TgaHeader;
		delete[] buffer;
		return NULL;
	}

	delete TgaHeader;
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
	SAFE_RELEASE(d3d.lpD3DBackSurface);
	SAFE_RELEASE(d3d.lpD3DIndexBuffer);
	SAFE_RELEASE(d3d.lpD3DVertexBuffer);

	return true;
}

const int MAX_VERTEXES = 4096 * 2;
//const int MAX_INDICES = 9216 * 2;

BOOL CreateVolatileResources() 
{
/*
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(MAX_VERTEXES * sizeof(D3DTLVERTEX),D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFVF_TLVERTEX, D3DPOOL_DEFAULT, &d3d.lpD3DVertexBuffer);
	if(FAILED(LastError)) {
		LogDxError(LastError);
		return FALSE;
	}
*/
/*
	LastError = d3d.lpD3DDevice->CreateIndexBuffer(MAX_INDICES * 3 * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &d3d.lpD3DIndexBuffer);
	if(FAILED(LastError)) {
//		OutputDebugString("Couldn't create Index Buffer");
//		LogDxError("Unable to create index buffer", LastError);
		LogDxError(LastError);
		return FALSE;
	}
*/
/*
	LastError = d3d.lpD3DDevice->SetStreamSource( 0, d3d.lpD3DVertexBuffer, sizeof(D3DTLVERTEX));
	if (FAILED(LastError)){
//		OutputDebugString(" Couldn't set stream source");
//		LogDxError("Unable to set stream source", LastError);
		LogDxError(LastError);
		return FALSE;
	}
*/
	LastError = d3d.lpD3DDevice->SetVertexShader(D3DFVF_TLVERTEX);
	if (FAILED(LastError)){
		OutputDebugString(" Couldn't set vertex shader");
	}

//	LastError = d3d.lpD3DDevice->SetFVF(D3DFVF_TLVERTEX);
	if(FAILED(LastError)) {
//		LogDxError("Unable to set FVF", LastError);
		LogDxError(LastError);
		return FALSE;
	}

	SetExecuteBufferDefaults();

	return true;
}

BOOL ChangeGameResolution(int width, int height, int colour_depth)
{
#if 0
	ReleaseVolatileResources();

	d3d.d3dpp.BackBufferHeight = height;
	d3d.d3dpp.BackBufferWidth = width;

	LastError = d3d.lpD3DDevice->Reset(&d3d.d3dpp);

	if(!FAILED(LastError)) {
		ScreenDescriptorBlock.SDB_Width     = width;
		ScreenDescriptorBlock.SDB_Height    = height;
		ScreenDescriptorBlock.SDB_Depth		= colour_depth;
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
	else {
//		OutputDebugString("\n couldn't reset for res change");
		LogDxError(LastError);
	}
#endif
	return true;
}

BOOL InitialiseDirect3DImmediateMode()
{
	// clear log file first, then write header text
	ClearLog();
	LogDxString("Starting to initialise Direct3D");

	int width = 720;
	int height = 576;

//	int width = 640;
//	int height = 480;
	int depth = 32;

	D3DHardwareAvailable = Yes;

	//	Zero d3d structure
    memset(&d3d, 0, sizeof(D3DINFO));

	//  Set up Direct3D interface object
	d3d.lpD3D = Direct3DCreate8(D3D_SDK_VERSION);

	if (!d3d.lpD3D)
	{
		LogDxErrorString("Could not create Direct3D object \n");
		return FALSE;
	}
	
//	D3DDISPLAYMODE displayMode;
	unsigned int modeCount = d3d.lpD3D->GetAdapterModeCount(D3DADAPTER_DEFAULT);
/*
	char buf[100];
	for(int i = 0; i < modeCount; i++)
	{
		d3d.lpD3D->EnumAdapterModes(0, i, &displayMode);
		sprintf(buf, "\n width: %d height: %d", displayMode.Width, displayMode.Height);
		OutputDebugString(buf);
	}
*/
	DWORD standard = XGetVideoStandard();
	if(standard == XC_VIDEO_STANDARD_PAL_I)
	{
		OutputDebugString("\n using PAL");
	}
	DWORD flags = XGetVideoFlags();

//	D3DDISPLAYMODE d3ddm;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
/*
	d3dpp.BackBufferWidth        = width;
    d3dpp.BackBufferHeight       = height;
    d3dpp.BackBufferFormat       = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount        = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
*/
	/* from SDL_x */
	if(XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)	// PAL user
	{
		//get supported video flags

		DWORD videoFlags = XGetVideoFlags();

		if(videoFlags & XC_VIDEO_FLAGS_PAL_60Hz)		// PAL 60 user
		{
			//d3dpp.FullScreen_RefreshRateInHz = 60;
			OutputDebugString("using refresh of 60\n");
		}
		else
		{
			//d3dpp.FullScreen_RefreshRateInHz = 50;
			OutputDebugString("using refresh of 50\n");
		}
	}

	if(XGetVideoFlags() == XC_VIDEO_FLAGS_PAL_60Hz)
	{
		OutputDebugString("XC_VIDEO_FLAGS_PAL_60Hz set\n");
	}

	D3DDISPLAYMODE d3ddm;
	LastError = d3d.lpD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);

	char buf[100];
	sprintf(buf, "width: %d height: %d format: %d\n", d3ddm.Width, d3ddm.Height, d3ddm.Format);
	OutputDebugString(buf);

	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferCount = 1;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;//D3DSWAPEFFECT_DISCARD;
//	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE;
//	d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_ONE_OR_IMMEDIATE;
//	d3dpp.Flags = D3DPRESENTFLAG_10X11PIXELASPECTRATIO;
	UsingStencil = true;

//#endif

	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);

	if (FAILED(LastError))
	{
		LogDxErrorString("Could not create Direct3D device");
		return FALSE;
	}

	// Log resolution set
	LogDxString("\t Resolution set: " + LogInteger(d3dpp.BackBufferWidth) + " x " + LogInteger(d3dpp.BackBufferHeight));

	ZeroMemory(&d3d.lpD3DViewport, sizeof(d3d.lpD3DViewport));
	d3d.lpD3DViewport.X = 0;
	d3d.lpD3DViewport.Y = 0;
	d3d.lpD3DViewport.Width = width;
	d3d.lpD3DViewport.Height = height;
	d3d.lpD3DViewport.MinZ = 0.0f;
	d3d.lpD3DViewport.MaxZ = 1.0f;

	LastError = d3d.lpD3DDevice->SetViewport(&d3d.lpD3DViewport);
	if (FAILED(LastError))
	{
		LogDxErrorString("Could not set viewport");
	}

	LastError = d3d.lpD3DDevice->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &d3d.lpD3DBackSurface);

	if (FAILED(LastError))
	{
		LogDxErrorString("Could not get a copy of the back buffer");
		return FALSE;
	}

	LastError = d3d.lpD3DBackSurface->GetDesc(&d3d.BackSurface_desc);

	if (FAILED(LastError))
	{
		LogDxErrorString("Could not get backbuffer surface description");
		return FALSE;
	}

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

	// save a copy of the presentation parameters for use
	// later (device reset, resolution/depth change)
	d3d.d3dpp = d3dpp;

	// create vertex and index buffers
	CreateVolatileResources();

	LogDxString("Initialise Direct3D succesfully");
	return TRUE;
}

void FlipBuffers()
{
	LastError = d3d.lpD3DDevice->Present(NULL, NULL, NULL, NULL);
	if (FAILED(LastError)) {
//		OutputDebugString(" Present failed ");
	}
}

void InGameFlipBuffers()
{
	FlipBuffers();
}

#if 1

// Note that error conditions have been removed
// on the grounds that an early exit will prevent
// EndScene being run if this function is used,
// which screws up all subsequent buffers

BOOL RenderD3DScene()
{
	return TRUE;
}

#else

int Time1, Time2, Time3, Time4;

BOOL RenderD3DScene()
{
    // Begin scene
	// My theory is that the functionality of this
	// thing must invoke a DirectDraw surface lock
	// on the back buffer without telling you.  However,
	// we shall see...

    Time1 = GetWindowsTickCount();

	LastError = d3d.lpD3DDevice->BeginScene();
	LOGDXERR(LastError);

	if (LastError != D3D_OK)
	  return FALSE;

    Time2 = GetWindowsTickCount();

	// Execute buffer
	#if 1
	LastError = d3d.lpD3DDevice->Execute(lpD3DExecCmdBuf,
	          d3d.lpD3DViewport, D3DEXECUTE_UNCLIPPED);
	LOGDXERR(LastError);
	#else
	LastError = d3d.lpD3DDevice->Execute(lpD3DExecCmdBuf,
	          d3d.lpD3DViewport, D3DEXECUTE_CLIPPED);
	LOGDXERR(LastError);
	#endif

	if (LastError != D3D_OK)
	  return FALSE;

    Time3 = GetWindowsTickCount();

	// End scene
    LastError = d3d.lpD3DDevice->EndScene();
	LOGDXERR(LastError);

	if (LastError != D3D_OK)
	  return FALSE;

    Time4 = GetWindowsTickCount();



	return TRUE;

#endif


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
	OutputDebugString("\n releasing direct3D!");
    DeallocateAllImages();

	// release back-buffer copy surface, vertex buffer and index buffer
	ReleaseVolatileResources();

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

	if (iheader->hBackup)
	{
		iheader->AvPTexture = AwCreateTexture("rf",AW_TLF_PREVSRC|AW_TLF_COMPRESS);
		return iheader->AvPTexture;
	}
	else return NULL;
}

int GetTextureHandle(IMAGEHEADER *imageHeaderPtr)
{
#if 0 // bjd
 	LPDIRECT3DTEXTURE Texture = (LPDIRECT3DTEXTURE) imageHeaderPtr->AvPTexture;


	LastError = Texture->GetHandle(d3d.lpD3DDevice, (D3DTEXTUREHANDLE*)&(imageHeaderPtr->D3DHandle));

	if (LastError != D3D_OK) return FALSE;

	imageHeaderPtr->D3DHandle = imageHeaderPtr->AvPTexture;
#endif
	return TRUE;
}



void ReleaseD3DTexture(void* texture)
{
	AvPTexture *TextureHandle = (AvPTexture *)texture;

	free(TextureHandle->buffer);
	free(TextureHandle);
}

void ReleaseD3DTexture8(LPDIRECT3DTEXTURE8 d3dTexture) 
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
	d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

void SecondFlushD3DZBuffer()
{
	FlushD3DZBuffer();
}

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

// For extern "C"

};

#if 0
std::string LogInteger(int value)
{
	// returns a string containing value
	std::ostringstream stream;
	stream << value;
	return stream.str();
}
#endif