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
//#include "dxlog.h"
#include "module.h"
#include "d3_func.h"
#include "kshape.h"

extern "C++" {
	#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
	#include "logString.h"

	#include <xtl.h>
	#include <xgraphics.h>
	#include "console.h"
}

int image_num = 0;

// Set to Yes to make default texture filter bilinear averaging rather
// than nearest
BOOL BilinearTextureFilter = 1;

int 					VideoModeColourDepth;
int						NumAvailableVideoModes;
VIDEOMODEINFO			AvailableVideoModes[MaxAvailableVideoModes];

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern int WindowMode;
extern int VideoModeColourDepth;

static HRESULT LastError;

D3DINFO d3d;

//int StartDriver;
//int StartFormat;

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
	if (d3d.Driver[d3d.CurrentDriver].AdapterInfo.Description != NULL) 
	{
		return d3d.Driver[d3d.CurrentDriver].AdapterInfo.Description;
	}
	else return "Default Adapter";
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
	D3DLOCKED_RECT	   lock;

	// default colour format
	D3DFORMAT colourFormat = D3DFMT_R5G6B5;

	if (ScreenDescriptorBlock.SDB_Depth == 16) {
		colourFormat = D3DFMT_R5G6B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32) {
//		colour_format = D3DFMT_A8R8G8B8;
		colourFormat = D3DFMT_LIN_A8R8G8B8;
	}

	int height = 495;
	int width = 450;

	int padWidth = 512;
	int padHeight = 512;

	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, NULL, colourFormat, D3DPOOL_MANAGED, &destTexture);
	if(FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	LastError = destTexture->LockRect(0, &lock, NULL, NULL );
	if(FAILED(LastError)) 
	{
		destTexture->Release();
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	if (ScreenDescriptorBlock.SDB_Depth == 16) 
	{
		unsigned short *destPtr;
		unsigned char *srcPtr;

		srcPtr = (unsigned char *)tex->buffer;

		D3DCOLOR pad_colour = D3DCOLOR_XRGB(0,0,0);

		// lets pad the whole thing black first
		for (int y = 0; y < padHeight; y++)
		{
			destPtr = ((unsigned short *)(((unsigned char *)lock.pBits) + y*lock.Pitch));

			for (int x = 0; x < padWidth; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*destPtr = RGB16(pad_colour, pad_colour, pad_colour);
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
		for (int y = 0; y < padHeight; y++)
		{
			destPtr = (((unsigned char *)lock.pBits) + y*lock.Pitch);

			for (int x = 0; x < padWidth; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(pad_colour, pad_colour, pad_colour, pad_colour);
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

	D3DLOCKED_RECT lock2;
	LastError = d3d.lpD3DDevice->CreateTexture(padWidth, padHeight, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &swizTexture);
	LastError = swizTexture->LockRect(0, &lock2, NULL, NULL );

	XGSwizzleRect(lock.pBits, lock.Pitch, NULL, lock2.pBits, padWidth, padHeight, NULL, 4);

	LastError = destTexture->UnlockRect(0);
	if(FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
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

LPDIRECT3DTEXTURE8 CreateFmvTexture(int width, int height, int usage, int pool)
{
	LPDIRECT3DTEXTURE8 destTexture = NULL;

	LastError = d3d.lpD3DDevice->CreateTexture(width, height, 1, usage, D3DFMT_A8R8G8B8, (D3DPOOL)pool, &destTexture);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	return destTexture;
}

// use this to make textures from non power of two images
LPDIRECT3DTEXTURE8 CreateD3DTexturePadded(AvPTexture *tex, int *real_height, int *real_width) 
{
	int original_width = tex->width;
	int original_height = tex->height;
	int new_width = 0;
	int new_height = 0;

#if 0
	#define MB	(1024*1024)
	MEMORYSTATUS stat;
	char buf[100];

	// Get the memory status.
    GlobalMemoryStatus( &stat );

	sprintf(buf, "%4d  free MB of physical memory.\n", stat.dwAvailPhys / MB );
	OutputDebugString( buf );
#endif

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

	LPDIRECT3DTEXTURE8 destTexture = NULL;

	D3DXIMAGE_INFO image;
	image.Depth  = 32;
	image.Width  = tex->width;
	image.Height = tex->height;
	image.MipLevels = 1;
	image.Depth = D3DFMT_A8R8G8B8;

	D3DPOOL poolType = D3DPOOL_MANAGED;

	/* fill tga header */
	TgaHeader.idlength = 0;
	TgaHeader.x_origin = tex->width;
	TgaHeader.y_origin = tex->height;
	TgaHeader.colourmapdepth  = 0;
	TgaHeader.colourmaplength = 0;
	TgaHeader.colourmaporigin = 0;
	TgaHeader.colourmaptype	  = 0;
	TgaHeader.datatypecode    = 2;			// RGB
	TgaHeader.bitsperpixel    = 32;
	TgaHeader.imagedescriptor = 0x20;	// set origin to top left
	TgaHeader.height = tex->height;
	TgaHeader.width  = tex->width;

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
		imageData[i+2] = tex->buffer[i];
		imageData[i+1] = tex->buffer[i+1];
		imageData[i] = tex->buffer[i+2];
		imageData[i+3] = tex->buffer[i+3];
	}

	LastError = D3DXCreateTextureFromFileInMemoryEx(d3d.lpD3DDevice,
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

LPDIRECT3DTEXTURE8 CreateD3DTexture(AvPTexture *tex, unsigned char *buf, int usage, D3DPOOL poolType) 
{
	/* create our texture for returning */
	LPDIRECT3DTEXTURE8 destTexture = NULL;

	/* fill tga header */
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
//	ReleaseVolatileResources();

	d3d.d3dpp.BackBufferHeight = height;
	d3d.d3dpp.BackBufferWidth = width;

	LastError = d3d.lpD3DDevice->Reset(&d3d.d3dpp);

	if (!FAILED(LastError)) 
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

//		CreateVolatileResources();
	}
	else 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}
	return true;
}

BOOL InitialiseDirect3DImmediateMode()
{
	// clear log file first, then write header text
	ClearLog();
	LogString("Starting to initialise Direct3D");

//	int width = 720;
//	int height = 576;

	int width = 640;
	int height = 480;
	int depth = 32;
	int defaultDevice = D3DADAPTER_DEFAULT;

	//	Zero d3d structure
    memset(&d3d, 0, sizeof(D3DINFO));

	//  Set up Direct3D interface object
	d3d.lpD3D = Direct3DCreate8(D3D_SDK_VERSION);

	if (!d3d.lpD3D)
	{
		LogErrorString("Could not create Direct3D object \n");
		return FALSE;
	}
	
//	D3DDISPLAYMODE displayMode;
	unsigned int modeCount = d3d.lpD3D->GetAdapterModeCount(defaultDevice);
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
	if (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)	// PAL user
	{
		//get supported video flags

		DWORD videoFlags = XGetVideoFlags();

		if(videoFlags & XC_VIDEO_FLAGS_PAL_60Hz)		// PAL 60 user
		{
			d3dpp.FullScreen_RefreshRateInHz = 60;
			OutputDebugString("using refresh of 60\n");
		}
		else
		{
			d3dpp.FullScreen_RefreshRateInHz = 50;
			OutputDebugString("using refresh of 50\n");
		}
	}

	if(XGetVideoFlags() == XC_VIDEO_FLAGS_PAL_60Hz)
	{
		OutputDebugString("XC_VIDEO_FLAGS_PAL_60Hz set\n");
	}

	D3DDISPLAYMODE d3ddm;
	LastError = d3d.lpD3D->GetAdapterDisplayMode(defaultDevice, &d3ddm);

	D3DDISPLAYMODE tempMode;

	/* create our list of supported resolutions for D3DFMT_LIN_X8R8G8B8 format */
	for (int i = 0; i < modeCount; i++)
	{
		LastError = d3d.lpD3D->EnumAdapterModes( defaultDevice, i, &tempMode );
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
			continue;
		}

		/* we only want D3DFMT_LIN_X8R8G8B8 format */
		if (tempMode.Format == D3DFMT_LIN_X8R8G8B8)
		{
			if (tempMode.Height < 480 || tempMode.Width < 640) 
				continue;

			int j = 0;
			/* check if the more already exists */
			for (; j < d3d.NumModes; j++)
			{
				if ((d3d.Driver[defaultDevice].DisplayMode[j].Width == tempMode.Width) &&
					(d3d.Driver[defaultDevice].DisplayMode[j].Height == tempMode.Height) &&
					(d3d.Driver[defaultDevice].DisplayMode[j].Format == tempMode.Format))
					break;
			}
			
			/* we looped all the way through but didn't break early due to already existing item */
			if (j == d3d.NumModes)
			{
				d3d.Driver[defaultDevice].DisplayMode[d3d.NumModes].Width       = tempMode.Width;
				d3d.Driver[defaultDevice].DisplayMode[d3d.NumModes].Height      = tempMode.Height;
				d3d.Driver[defaultDevice].DisplayMode[d3d.NumModes].Format      = tempMode.Format;
				d3d.Driver[defaultDevice].DisplayMode[d3d.NumModes].RefreshRate = 0;
				d3d.NumModes++;
			}
		}
	}

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
//	d3dpp.Flags = D3DPRESENTFLAG_10X11PIXELASPECTRATIO;
//	UsingStencil = true;

//#endif

	LastError = d3d.lpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, NULL,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &d3d.lpD3DDevice);

	if (FAILED(LastError))
	{
		LogErrorString("Could not create Direct3D device");
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	// Log resolution set
	LogString("\t Resolution set: " + LogInteger(d3dpp.BackBufferWidth) + " x " + LogInteger(d3dpp.BackBufferHeight));
/*
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
		LogDxError(LastError, __LINE__, __FILE__);
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

	/* use an offset for hud items to account for tv safe zones. just use width for now. 5%?  */
	ScreenDescriptorBlock.SDB_SafeZoneWidthOffset = (width / 100) * 5;
	ScreenDescriptorBlock.SDB_SafeZoneHeightOffset = (height / 100) * 5;

	// save a copy of the presentation parameters for use
	// later (device reset, resolution/depth change)
	d3d.d3dpp = d3dpp;

	// create vertex and index buffers
	CreateVolatileResources();

	LogString("Initialised Direct3D succesfully");
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

	// release back-buffer copy surface, vertex buffer and index buffer
	ReleaseVolatileResources();

	SAFE_RELEASE(d3d.lpD3DDevice);
	LogString("Releasing Direct3D device...");

	SAFE_RELEASE(d3d.lpD3D);
	LogString("Releasing Direct3D object...");

	/* release Direct Input stuff */
	ReleaseDirectKeyboard();
	ReleaseDirectMouse();
	ReleaseDirectInput();
}

void ReleaseAvPTexture(AvPTexture* texture)
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

void ReleaseD3DTexture(D3DTEXTURE d3dTexture)
{
	/* release d3d texture */
	SAFE_RELEASE(d3dTexture);
}

void FlushD3DZBuffer()
{
	d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

// For extern "C"
};