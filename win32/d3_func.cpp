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
#include <d3dx9.h>
#include "fmvCutscenes.h"
#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
#include "logString.h"
#include "configFile.h"
#include "console.h"
#include "vertexBuffer.h"
#include "networking.h"
#include "font2.h"

extern "C"
{
	extern HWND hWndMain;
}

//test
/*
class D3D9Renderer : public Renderer
{
	private:
		IDirect3D9				*D3D;
		IDirect3DDevice9		*D3DDevice;
		D3DVIEWPORT9			D3DViewport;
		D3DPRESENT_PARAMETERS	D3DPresentationParams;

		D3DXMATRIX	matIdentity;
		D3DXMATRIX	matView;
		D3DXMATRIX	matProjection;
		D3DXMATRIX	matOrtho;
		D3DXMATRIX	matViewPort;

		HRESULT					LastError;

		std::string				deviceName;

	public:
		D3D9Renderer():
			D3D(0),
			D3DDevice(0)
		{
			memset(&D3DPresentationParams, 0, sizeof(D3DPRESENT_PARAMETERS));
		}
		void Initialise();
		void BeginFrame();
		void EndFrame();
};


bool D3D9Renderer::Initialise()
{
	// create Direct3D9 interface object
	D3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!D3D)
	{
		Con_PrintError("Could not create Direct3D9 object");
		return false;
	}

	// fill out presentation parameters
	D3DPresentationParams.hDeviceWindow = hWndMain;
	D3DPresentationParams.SwapEffect = D3DSWAPEFFECT_DISCARD;

	D3DPresentationParams.Windowed = TRUE;
	D3DPresentationParams.BackBufferWidth = 800;
	D3DPresentationParams.BackBufferHeight = 600;
	D3DPresentationParams.PresentationInterval = 0;

	D3DPresentationParams.BackBufferCount = 1;
	D3DPresentationParams.AutoDepthStencilFormat = D3DFMT_D24S8;
	D3DPresentationParams.EnableAutoDepthStencil = TRUE;

	D3DPresentationParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	// create the Direct3D9 device
	LastError = D3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWndMain,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &D3DPresentationParams, &D3DDevice);

	if (FAILED(LastError))
	{
		Con_PrintError("Could not create Direct3D device");
		return false;
	}

	// set up viewport
	D3DViewport.X = 0;
	D3DViewport.Y = 0;
	D3DViewport.Width = 800;
	D3DViewport.Height = 600;
	D3DViewport.MinZ = 0.0f;
	D3DViewport.MaxZ = 1.0f;

	LastError = D3DDevice->SetViewport(&D3DViewport);
	if (FAILED(LastError))
	{
		Con_PrintError("Could not set Direct3D viewport");
		return false;
	}

	// create identity matrix
	D3DXMatrixIdentity(&matIdentity);

	return true;
}

D3D9Renderer::~D3D9Renderer()
{
	SAFE_RELEASE(D3D);
	SAFE_RELEASE(D3DDevice);
}

void D3D9Renderer::BeginFrame()
{
	// check for a lost device
	LastError = D3DDevice->TestCooperativeLevel();

	if (FAILED(LastError))
	{
		// release vertex + index buffers, and dynamic textures
//		ReleaseVolatileResources();

		// disable XInput
//		XInputEnable(false);

		while (1)
		{
			CheckForWindowsMessages();

			if (D3DERR_DEVICENOTRESET == LastError)
			{
				OutputDebugString("Releasing resources for a device reset..\n");

				if (FAILED(D3DDevice->Reset(&d3d.d3dpp)))
				{
					OutputDebugString("Couldn't reset device\n");
				}
				else
				{
					OutputDebugString("We have reset the device. recreating resources..\n");
//					CreateVolatileResources();

					SetTransforms();

					// re-enable XInput
//					XInputEnable(true);
					break;
				}
			}
			else if (D3DERR_DEVICELOST == LastError)
			{
				OutputDebugString("D3D device lost\n");
			}
			else if (D3DERR_DRIVERINTERNALERROR == LastError)
			{
				// handle this a lot better (exit the game etc)
				Con_PrintError("need to close avp as a display adapter error occured");
//				return false;
			}
			Sleep(50);
		}
	}

	LastError = D3DDevice->BeginScene();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
//		return false;
	}

	LastError = D3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
//		return false;
	}
}

void D3D9Renderer::EndFrame()
{
	LastError = D3DDevice->EndScene();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}
}

bool D3D9Renderer::CreateVertexBuffer(uint32_t length, uint32_t usage, r_VertexBuffer **vertexBuffer)
{
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

	LastError = D3DDevice->CreateVertexBuffer(length, vbUsage, 0, vbPool, vertexBuffer, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}
*/

extern int NumberOfFMVTextures;
#define MAX_NO_FMVTEXTURES 10
extern FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];

D3DXMATRIX matOrtho;
D3DXMATRIX matOrtho2;
D3DXMATRIX matProjection;
D3DXMATRIX matView;
D3DXMATRIX matIdentity;
D3DXMATRIX matViewPort;

struct D3D_SHADER
{
	LPDIRECT3DVERTEXDECLARATION9	vertexDeclaration;
	LPDIRECT3DVERTEXSHADER9			vertexShader;
	LPDIRECT3DPIXELSHADER9			pixelShader;
	LPD3DXCONSTANTTABLE				constantTable;
};

extern D3DVERTEXELEMENT9 declMain[];
extern D3DVERTEXELEMENT9 declOrtho[];
extern D3DVERTEXELEMENT9 declFMV[];
extern D3DVERTEXELEMENT9 declTallFontText[];

extern LPD3DXCONSTANTTABLE	vertexConstantTable;
extern LPD3DXCONSTANTTABLE	orthoConstantTable;
extern LPD3DXCONSTANTTABLE	fmvConstantTable;
extern LPD3DXCONSTANTTABLE	cloudConstantTable;

extern void Init();
extern void ReleaseAllFMVTextures(void);

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
	Tex_ReloadDynamicTextures();

	ReleaseAllFMVTexturesForDeviceReset();

	SAFE_RELEASE(d3d.lpD3DIndexBuffer);
	SAFE_RELEASE(d3d.lpD3DVertexBuffer);
	SAFE_RELEASE(d3d.lpD3DOrthoVertexBuffer);
	SAFE_RELEASE(d3d.lpD3DOrthoIndexBuffer);
	SAFE_RELEASE(d3d.lpD3DParticleVertexBuffer);
	SAFE_RELEASE(d3d.lpD3DParticleIndexBuffer);

	return true;
}

bool R_CreateVertexBuffer(uint32_t length, uint32_t usage, r_VertexBuffer **vertexBuffer)
{
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

	LastError = d3d.lpD3DDevice->CreateVertexBuffer(length, vbUsage, 0, vbPool, vertexBuffer, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_DrawPrimitive(uint32_t numPrimitives)
{
	LastError = d3d.lpD3DDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, numPrimitives);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_LockVertexBuffer(r_VertexBuffer *vertexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, void **data, enum R_USAGE usage)
{
	DWORD vbFlags = 0;
	if (usage == USAGE_DYNAMIC)
	{
		vbFlags = D3DLOCK_DISCARD;
	}

	LastError = vertexBuffer->Lock(offsetToLock, sizeToLock, data, vbFlags);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_SetVertexBuffer(r_VertexBuffer *vertexBuffer, uint32_t FVFsize)
{
	LastError = d3d.lpD3DDevice->SetStreamSource(0, vertexBuffer, 0, FVFsize);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool R_UnlockVertexBuffer(r_VertexBuffer *vertexBuffer)
{
	LastError = vertexBuffer->Unlock();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool CreateVolatileResources()
{
	RecreateAllFMVTexturesAfterDeviceReset();

	// create dynamic vertex buffer
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(MAX_VERTEXES * sizeof(D3DLVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, /*D3DFVF_LVERTEX*/0, D3DPOOL_DEFAULT, &d3d.lpD3DVertexBuffer, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// create index buffer
	LastError = d3d.lpD3DDevice->CreateIndexBuffer(MAX_INDICES * 3 * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &d3d.lpD3DIndexBuffer, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// create our 2D vertex buffer
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(4 * 2000 * sizeof(ORTHOVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, /*D3DFVF_ORTHOVERTEX*/0, D3DPOOL_DEFAULT, &d3d.lpD3DOrthoVertexBuffer, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// create our 2D index buffer
	LastError = d3d.lpD3DDevice->CreateIndexBuffer(MAX_INDICES * 3 * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &d3d.lpD3DOrthoIndexBuffer, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// create particle vertex buffer
	LastError = d3d.lpD3DDevice->CreateVertexBuffer(MAX_VERTEXES * sizeof(D3DLVERTEX), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &d3d.lpD3DParticleVertexBuffer, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// create particle index buffer
	LastError = d3d.lpD3DDevice->CreateIndexBuffer(MAX_INDICES * 3 * sizeof(WORD), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &d3d.lpD3DParticleIndexBuffer, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	LastError = d3d.lpD3DDevice->SetStreamSource(0, d3d.lpD3DVertexBuffer, 0, sizeof(D3DLVERTEX));
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	LastError = d3d.lpD3DDevice->SetFVF(D3DFVF_LVERTEX);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	SetRenderStateDefaults();

	return true;
}

bool R_LockTexture(r_Texture texture, uint8_t **data, uint32_t *pitch)
{
	D3DLOCKED_RECT lock;

	LastError = texture->LockRect(0, &lock, NULL, D3DLOCK_DISCARD);

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
		return false;
	else
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

	fov = atoi(Con_GetArgument(0).c_str());

	SetTransforms();
}

extern "C"
{
	extern void ThisFramesRenderingHasBegun(void);
	extern void ThisFramesRenderingHasFinished(void);
	extern HWND hWndMain;
	extern int WindowMode;
	extern void ChangeWindowsSize(uint32_t width, uint32_t height);

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

	std::ostringstream fileName(GetSaveFolderPath());

	// get current system time
	GetSystemTime(&systemTime);

	//	creates filename from date and time, adding a prefix '0' to seconds value
	//	otherwise 9 seconds appears as '9' instead of '09'
	{
		bool prefixSeconds = false;
		if (systemTime.wYear < 10)
			prefixSeconds = true;

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
		OutputDebugString("Save Surface to file failed\n");
	}

	// release surface
	SAFE_RELEASE(frontBuffer);
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
	r_Texture destTexture = NULL;
	D3DLOCKED_RECT lock;

	// default colour format
	D3DFORMAT colourFormat = D3DFMT_A8R8G8B8;//D3DFMT_R5G6B5;
/*
	if (ScreenDescriptorBlock.SDB_Depth == 16)
	{
		colourFormat = D3DFMT_R5G6B5;
	}
	if (ScreenDescriptorBlock.SDB_Depth == 32)
	{
		colourFormat = D3DFMT_A8R8G8B8;
	}
*/

	uint32_t width = 450;
	uint32_t height = 495;

	uint32_t padWidth = 512;
	uint32_t padHeight = 512;

	uint32_t charWidth = 30;
	uint32_t charHeight = 33;

	uint32_t numTotalChars = tex->height / charHeight;

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
/*
	if (ScreenDescriptorBlock.SDB_Depth == 16)
	{
		uint16_t *destPtr;
		uint8_t  *srcPtr;

		srcPtr = (uint8_t*)tex->buffer;

		D3DCOLOR padColour = D3DCOLOR_ARGB(255, 255, 0, 255);

		// lets pad the whole thing black first
		for (uint32_t y = 0; y < padHeight; y++)
		{
			destPtr = ((uint16_t*)(((uint8_t*)lock.pBits) + y*lock.Pitch));

			for (uint32_t x = 0; x < padWidth; x++)
			{
				// >> 3 for red and blue in a 16 bit texture, 2 for green
				*destPtr = static_cast<uint16_t> (RGB16(padColour, padColour, padColour));
				destPtr += sizeof(uint16_t);
			}
		}

		for (uint32_t i = 0; i < numTotalChars; i++)
		{
			uint32_t row = i / 15; // get row
			uint32_t column = i % 15; // get column from remainder value

			uint32_t offset = ((column * charWidth) * sizeof(uint16_t)) + ((row * charHeight) * lock.Pitch);

			destPtr = ((uint16_t*)(((uint8_t*)lock.pBits + offset)));

			for (uint32_t y = 0; y < charHeight; y++)
			{
				destPtr = ((uint16_t*)(((uint8_t*)lock.pBits + offset) + (y*lock.Pitch)));

				for (uint32_t x = 0; x < charWidth; x++)
				{
					*destPtr = RGB16(srcPtr[0], srcPtr[1], srcPtr[2]);

					destPtr += sizeof(uint16_t);
					srcPtr += sizeof(uint32_t); // our source is 32 bit, move over an entire 4 byte pixel
				}
			}
		}
	}
*/
//	if (ScreenDescriptorBlock.SDB_Depth == 32)
	{
		uint8_t *destPtr, *srcPtr;

		srcPtr = (uint8_t*)tex->buffer;

		D3DCOLOR padColour = D3DCOLOR_ARGB(255, 255, 0, 255);

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
	}

	LastError = destTexture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	return destTexture;
}

/*
r_Texture CreateFmvTexture2(uint32_t &width, uint32_t &height)
{
	// TODO - Add support for rendering FMVs on GPUs that can't use non power of 2 textures

	r_Texture destTexture = NULL;
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
	LastError = d3d.lpD3DDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_L8, D3DPOOL_DEFAULT, &destTexture, NULL);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return NULL;
	}

	return destTexture;
}
*/

bool CreateVertexShader(const std::string &fileName, LPDIRECT3DVERTEXSHADER9 *vertexShader, LPD3DXCONSTANTTABLE *constantTable)
{
	LPD3DXBUFFER pErrors = NULL;
	LPD3DXBUFFER pCode = NULL;
	std::string actualPath = shaderPath + fileName;

	// test that the path to the file is valid first (d3dx doesn't give a specific error message for this)
	std::ifstream fileOpenTest(actualPath.c_str(), std::ifstream::in | std::ifstream::binary);
	if (!fileOpenTest.good())
	{
		LogErrorString("Can't open vertex shader file " + actualPath, __LINE__, __FILE__);
		return false;
	}
	// close the file
	fileOpenTest.close();

	// set up vertex shader
	LastError = D3DXCompileShaderFromFile(actualPath.c_str(), //filepath
						NULL,            // macro's
						NULL,            // includes
						"vs_main",       // main function
						"vs_2_0",        // shader profile
						0,               // flags
						&pCode,          // compiled operations
						&pErrors,        // errors
						constantTable);  // constants

	if (FAILED(LastError))
	{
		OutputDebugString(DXGetErrorString(LastError));
		OutputDebugString(DXGetErrorDescription(LastError));

		if (pErrors)
		{
			// shader didn't compile for some reason
			OutputDebugString((const char*)pErrors->GetBufferPointer());
			pErrors->Release();
		}

		return false;
	}

	d3d.lpD3DDevice->CreateVertexShader((DWORD*)pCode->GetBufferPointer(), vertexShader);
	pCode->Release();

	return true;
}

bool CreatePixelShader(const std::string &fileName, LPDIRECT3DPIXELSHADER9 *pixelShader)
{
	LPD3DXBUFFER pErrors = NULL;
	LPD3DXBUFFER pCode = NULL;
	std::string actualPath = shaderPath + fileName;

	// test that the path to the file is valid first (d3dx doesn't give a specific error message for this)
	std::ifstream fileOpenTest(actualPath.c_str(), std::ifstream::in | std::ifstream::binary);
	if (!fileOpenTest.good())
	{
		LogErrorString("Can't open pixel shader file " + actualPath, __LINE__, __FILE__);
		return false;
	}
	// close the file
	fileOpenTest.close();

	// set up pixel shader
	LastError = D3DXCompileShaderFromFile(actualPath.c_str(), //filepath
						NULL,            // macro's
						NULL,            // includes
						"ps_main",       // main function
						"ps_2_0",        // shader profile
						0,               // flags
						&pCode,          // compiled operations
						&pErrors,        // errors
						NULL);			 // constants

	if (FAILED(LastError))
	{
		OutputDebugString(DXGetErrorString(LastError));
		OutputDebugString(DXGetErrorDescription(LastError));

		if (pErrors)
		{
			// shader didn't compile for some reason
			OutputDebugString((const char*)pErrors->GetBufferPointer());
			pErrors->Release();
		}

		return false;
	}

	d3d.lpD3DDevice->CreatePixelShader((DWORD*)pCode->GetBufferPointer(), pixelShader);
	pCode->Release();

	return true;
}

// removes pure red colour from a texture. used to remove red outline grid on small font texture.
// we remove the grid as it can sometimes bleed onto text when we use texture filtering. maybe add params for passing width/height?
void DeRedTexture(r_Texture texture)
{
	// lock texture

	uint8_t *srcPtr = NULL;
	uint8_t *destPtr = NULL;
	uint32_t pitch = 0;

	R_LockTexture(texture, &srcPtr, &pitch);

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

	// set passed in width and height values to be used later
//	(*real_height) = new_height;
//	(*real_width) = new_width;

//	D3DXCheckTextureRequirements(d3d.lpD3DDevice, (UINT*)&new_width, (UINT*)&new_height, NULL, 0, NULL, D3DPOOL_MANAGED);

	(*realHeight) = newHeight;
	(*realWidth) = newWidth;

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

bool CreateD3DTextureFromFile(const char* fileName, Texture &texture)
{
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
		return false;
	}

	texture.width = imageInfo.Width;
	texture.height = imageInfo.Height;
	texture.usage = TextureUsage_Normal;

	return true;
}

bool R_CreateTextureFromAvPTexture(AVPTEXTURE &AvPTexture, enum TextureUsage usageType, Texture &texture)
{
	D3DPOOL texturePool;
	D3DFORMAT textureFormat;
	uint32_t textureUsage;
	LPDIRECT3DTEXTURE9 d3dTexture = NULL;

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
	LPDIRECT3DTEXTURE9 d3dTexture = NULL;

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
	LastError = d3d.lpD3DDevice->CreateTexture(width, height, 1, textureUsage, textureFormat, texturePool, &d3dTexture, NULL);
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

r_Texture CreateD3DTexture(AVPTEXTURE *tex, uint32_t usage, D3DPOOL poolType)
{
	r_Texture destTexture = NULL;

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
	uint32_t imageSize = tex->height * tex->width * sizeof(uint32_t);

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
		imageData[i+2] = tex->buffer[i];
		imageData[i+1] = tex->buffer[i+1];
		imageData[i]   = tex->buffer[i+2];
		imageData[i+3] = tex->buffer[i+3];
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
		1, // mips
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
		delete[] buffer;
		return NULL;
	}

	delete[] buffer;
	return destTexture;
}

void R_ReleaseTexture(r_Texture &texture)
{
	if (texture)
	{
		texture->Release();
		texture = NULL;
	}
}

bool ChangeGameResolution(uint32_t width, uint32_t height/*, uint32_t colourDepth*/)
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
				return false;
			}
		}
		else
		{
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
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

// need to redo all the enumeration code here, as it's not very good..
bool InitialiseDirect3D()
{
	// clear log file first, then write header text
	ClearLog();
	Con_PrintMessage("Starting to initialise Direct3D9");

	// grab the users selected resolution from the config file
	uint32_t width       = Config_GetInt("[VideoMode]", "Width", 800);
	uint32_t height      = Config_GetInt("[VideoMode]", "Height", 600);
//	uint32_t colourDepth = Config_GetInt("[VideoMode]", "ColourDepth", 32);
	bool useTripleBuffering = Config_GetBool("[VideoMode]", "UseTripleBuffering", false);
	bool useVSync = Config_GetBool("[VideoMode]", "UseVSync", true);
	shaderPath = Config_GetString("[VideoMode]", "ShaderPath", "shaders\\");

	// set some defaults
	uint32_t defaultDevice = D3DADAPTER_DEFAULT;
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
		Con_PrintError("Could not create Direct3D9 object");
		return false;
	}

	// Get the number of devices/video cards in the system
	d3d.NumDrivers = d3d.lpD3D->GetAdapterCount();

	Con_PrintMessage("\t Found " + IntToString(d3d.NumDrivers) + " video adapter(s)");

	// Get adapter information for all available devices (vid card name, etc)
	for (uint32_t driverIndex = 0; driverIndex < d3d.NumDrivers; driverIndex++)
	{
		LastError = d3d.lpD3D->GetAdapterIdentifier(driverIndex, /*D3DENUM_WHQL_LEVEL*/0, &d3d.Driver[driverIndex].AdapterInfo);
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
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
			Con_PrintError("GetAdapterDisplayMode call failed");
			LogDxError(LastError, __LINE__, __FILE__);
			return false;
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
	uint32_t numDisplayFormats = sizeof(DisplayFormats) / sizeof(DisplayFormats[0]);

	uint32_t numFomats = 0;

	// loop through all the devices, getting the list of formats available for each
	for (uint32_t thisDevice = 0; thisDevice < d3d.NumDrivers; thisDevice++)
	{
		d3d.Driver[thisDevice].NumModes = 0;

		for (uint32_t displayFormatIndex = 0; displayFormatIndex < numDisplayFormats; displayFormatIndex++)
		{
			// get the number of display moves available on this adapter for this particular format
			uint32_t numAdapterModes = d3d.lpD3D->GetAdapterModeCount(thisDevice, DisplayFormats[displayFormatIndex]);

			for (uint32_t modeIndex = 0; modeIndex < numAdapterModes; modeIndex++)
			{
				D3DDISPLAYMODE DisplayMode;

				// does this adapter support the requested format?
				d3d.lpD3D->EnumAdapterModes(thisDevice, DisplayFormats[displayFormatIndex], modeIndex, &DisplayMode);

				// Filter out low-resolution modes
				if (DisplayMode.Width < 640 || DisplayMode.Height < 480)
					continue;

				uint32_t j = 0;

				// Check if the mode already exists (to filter out refresh rates)
				for(; j < d3d.Driver[thisDevice].NumModes; j++)
				{
					if ((d3d.Driver[thisDevice].DisplayMode[j].Width  == DisplayMode.Width) &&
						(d3d.Driver[thisDevice].DisplayMode[j].Height == DisplayMode.Height) &&
						(d3d.Driver[thisDevice].DisplayMode[j].Format == DisplayMode.Format))
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

					uint32_t f = 0;

					// Check if the mode's format already exists
					for (; f < numFomats; f++ )
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

	for (uint32_t i = 0; i < (sizeof(DisplayFormats) / sizeof(DisplayFormats[0])); i++)
	{
		for (uint32_t j = 0; j < d3d.Driver[defaultDevice].NumModes; j++)
		{
			// found a usable mode
			if ((d3d.Driver[defaultDevice].DisplayMode[j].Width == width) && (d3d.Driver[defaultDevice].DisplayMode[j].Height == height))
			{
				// try find a matching depth buffer format
				for (uint32_t d = 0; d < (sizeof(DepthFormats) / sizeof(DepthFormats[0])); d++)
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

						// fix this..
						goto gotValidFormats;
					}
				}
			}
		}
	}

	// we jump to here when we have picked all valid formats
	gotValidFormats:

	if (!gotOne)
	{
		Con_PrintError("Couldn't find match for user requested resolution");

		// set some default values?
		width = 800;
		height = 600;
		SelectedBackbufferFormat = DisplayFormats32[0]; // use the first in the list
	}

	// set up the presentation parameters
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory (&d3dpp, sizeof(D3DPRESENT_PARAMETERS));
	d3dpp.hDeviceWindow = hWndMain;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;

	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = SelectedDepthFormat;
	d3dpp.BackBufferFormat = SelectedBackbufferFormat;

	d3dpp.Windowed = (windowed) ? TRUE : FALSE;

	d3dpp.BackBufferWidth = width;
	d3dpp.BackBufferHeight = height;

	// setting this to interval one will cap the framerate to monitor refresh
	// the timer goes a bit mad if this isnt capped!
	if (useVSync)
	{
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		Con_PrintMessage("V-Sync is on");
	}
	else
	{
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		Con_PrintMessage("V-Sync is off");
	}

	// resize the win32 window to match our d3d backbuffer size
	ChangeWindowsSize(d3dpp.BackBufferWidth, d3dpp.BackBufferHeight);

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
	LastError = d3d.lpD3D->CreateDevice(defaultDevice, D3DDEVTYPE_HAL, hWndMain,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &d3dpp, &d3d.lpD3DDevice);

	if (FAILED(LastError))
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
		Con_PrintError("Could not create Direct3D device");
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// get device capabilities
	D3DCAPS9 d3dCaps;
	d3d.lpD3DDevice->GetDeviceCaps(&d3dCaps);

	// set this to true initially
	d3d.supportsShaders = true;

	// check pixel shader support
	if (d3dCaps.PixelShaderVersion < (D3DPS_VERSION(2,0)))
	{
		Con_PrintError("Device does not support Pixel Shader version 2.0 or greater");
		d3d.supportsShaders = false;
	}

	// check vertex shader support
	if (d3dCaps.VertexShaderVersion < (D3DVS_VERSION(2,0)))
	{
		Con_PrintError("Device does not support Vertex Shader version 2.0 or greater");
		d3d.supportsShaders = false;
	}

	// check and remember if we have dynamic texture support
	if (d3dCaps.Caps2 & D3DCAPS2_DYNAMICTEXTURES)
	{
		d3d.supportsDynamicTextures = true;
	}
	else
	{
		d3d.supportsDynamicTextures = false;
		Con_PrintError("Device does not support D3DUSAGE_DYNAMIC");
	}

	// check texture support
	if (d3dCaps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		Con_PrintError("Device requires square textures");
	}

	if (d3dCaps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
	{
		Con_PrintError("Device requires power of two textures");

		// check conditonal support
		if (d3dCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)
		{
			Con_PrintError("Device has conditional power of two textures support");
		}
	}

	// check max texture size
	Con_PrintMessage("Max texture size: " + IntToString(d3dCaps.MaxTextureWidth));

	// Log resolution set
	Con_PrintMessage("\t Resolution set: " + IntToString(d3dpp.BackBufferWidth) + " x " + IntToString(d3dpp.BackBufferHeight));

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

	SetTransforms();

	Con_AddCommand("dumptex", WriteMenuTextures);
	Con_AddCommand("r_toggleWireframe", ToggleWireframe);
	Con_AddCommand("r_setfov", SetFov);

	d3d.lpD3DDevice->CreateVertexDeclaration(declMain, &d3d.vertexDecl);
	d3d.lpD3DDevice->CreateVertexDeclaration(declOrtho, &d3d.orthoVertexDecl);
	d3d.lpD3DDevice->CreateVertexDeclaration(declFMV, &d3d.fmvVertexDecl);
	LastError = d3d.lpD3DDevice->CreateVertexDeclaration(declTallFontText, &d3d.cloudVertexDecl);
	if (FAILED(LastError))
	{
		OutputDebugString("CreateVertexDeclaration failed\n");
	}

	if (!CreateVertexShader("vertex.vsh", &d3d.vertexShader, &vertexConstantTable))
	{
		return false;
	}
	CreateVertexShader("orthoVertex.vsh", &d3d.orthoVertexShader, &orthoConstantTable);
	CreateVertexShader("fmvVertex.vsh", &d3d.fmvVertexShader, &fmvConstantTable);
	CreateVertexShader("tallFontTextVertex.vsh", &d3d.cloudVertexShader, &cloudConstantTable);

	CreatePixelShader("pixel.psh", &d3d.pixelShader);
	CreatePixelShader("fmvPixel.psh", &d3d.fmvPixelShader);
	CreatePixelShader("tallFontTextPixel.psh", &d3d.cloudPixelShader);

	r_Texture blankTexture;

	// create a 1x1 resolution texture to set to shader for sampling when we don't want to texture an object (eg what was NULL texture in fixed function pipeline)
	d3d.lpD3DDevice->CreateTexture(1, 1, 1, 0, D3DFMT_L8, D3DPOOL_MANAGED, &blankTexture, NULL);

	D3DLOCKED_RECT lock;
	blankTexture->LockRect(0, &lock, NULL, 0);

	// set pixel to white
	memset(lock.pBits, 255, lock.Pitch);

	blankTexture->UnlockRect(0);

	NO_TEXTURE = Tex_AddTexture("Blank", blankTexture, 1, 1);

	Con_PrintMessage("Initialised Direct3D9 succesfully");

	Con_Init();
	Net_Initialise();
	Font_Init();

	Init();

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

	// set up orthographic projection matrix
	D3DXMatrixOrthoLH(&matOrtho2, (float)ScreenDescriptorBlock.SDB_Width, (float)ScreenDescriptorBlock.SDB_Height, 1.0f, 10.0f);

	// set up projection matrix
	D3DXMatrixPerspectiveFovLH(&matProjection, D3DXToRadian(75), (float)ScreenDescriptorBlock.SDB_Width / (float)ScreenDescriptorBlock.SDB_Height, 64.0f, 1000000.0f);

	// set up a viewport transform matrix
	matViewPort = matIdentity;

	matViewPort._11 = (float)(ScreenDescriptorBlock.SDB_Width / 2);
	matViewPort._22 = (float)((-ScreenDescriptorBlock.SDB_Height) / 2);
	matViewPort._33 = (1.0f - 0.0f);
	matViewPort._41 = (0 + matViewPort._11); // dwX + dwWidth / 2
	matViewPort._42 = (float)(ScreenDescriptorBlock.SDB_Height / 2) + 0;
	matViewPort._43 = 0.0f; // minZ
	matViewPort._44 = 1.0f;

/*
	d3d.lpD3DDevice->SetTransform(D3DTS_WORLD,		&matIdentity);
	d3d.lpD3DDevice->SetTransform(D3DTS_VIEW,		&matIdentity);
	d3d.lpD3DDevice->SetTransform(D3DTS_PROJECTION, &matOrtho);
*/
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

	Tex_DeInit();

	// release back-buffer copy surface, vertex buffer and index buffer
	ReleaseVolatileResources();

	// release constant tables
	SAFE_RELEASE(vertexConstantTable);
	SAFE_RELEASE(orthoConstantTable);
	SAFE_RELEASE(fmvConstantTable);
	SAFE_RELEASE(cloudConstantTable);

	// release vertex declarations
	SAFE_RELEASE(d3d.vertexDecl);
	SAFE_RELEASE(d3d.orthoVertexDecl);
	SAFE_RELEASE(d3d.fmvVertexDecl);
	SAFE_RELEASE(d3d.cloudVertexDecl);

	// release pixel shaders
	SAFE_RELEASE(d3d.pixelShader);
	SAFE_RELEASE(d3d.fmvPixelShader);
	SAFE_RELEASE(d3d.cloudPixelShader);

	// release vertex shaders
	SAFE_RELEASE(d3d.vertexShader);
	SAFE_RELEASE(d3d.fmvVertexShader);
	SAFE_RELEASE(d3d.orthoVertexShader);
	SAFE_RELEASE(d3d.cloudVertexShader);

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

void ReleaseD3DTexture(r_Texture *d3dTexture)
{
	// release d3d texture
	SAFE_RELEASE(*d3dTexture);
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
		LastError = d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSU Wrap fail");
		}

		LastError = d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSV Wrap fail");
		}

		LastError = d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSW Wrap fail");
		}
	}
	else if (textureAddressMode == TEXTURE_CLAMP)
	{
		// clamp texture addresses
		LastError = d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSU Clamp fail");
		}

		LastError = d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSV Clamp fail");
		}

		LastError = d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSW Clamp fail");
		}
	}
}

void ChangeFilteringMode(enum FILTERING_MODE_ID filteringRequired)
{
	if (CurrentRenderStates.FilteringMode == filteringRequired)
		return;

	CurrentRenderStates.FilteringMode = filteringRequired;

	switch (CurrentRenderStates.FilteringMode)
	{
		case FILTERING_BILINEAR_OFF:
		{
			d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
			break;
		}
		case FILTERING_BILINEAR_ON:
		{
			d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			break;
		}
		default:
		{
			LOCALASSERT("Unrecognized filtering mode"==0);
			OutputDebugString("Unrecognized filtering mode\n");
			break;
		}
	}
}

void ToggleWireframe()
{
	if (CurrentRenderStates.WireFrameModeIsOn)
		CheckWireFrameMode(0);
	else
		CheckWireFrameMode(1);
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
/*
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_ANISOTROPIC );
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC );
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
*/
//	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 8);

	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
/*
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP,	D3DTOP_MODULATE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1,	D3DTA_TEXTURE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2,	D3DTA_DIFFUSE);

	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP,	D3DTOP_MODULATE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2,	D3DTA_DIFFUSE);

	d3d.lpD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP,	D3DTOP_DISABLE);
	d3d.lpD3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP,	D3DTOP_DISABLE);

	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
*/
	float alphaRef = 0.5f;
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAREF,			*((DWORD*)&alphaRef));//(DWORD)0.5);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAFUNC,		D3DCMP_GREATER);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE,	TRUE);

	d3d.lpD3DDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_POINTSCALEENABLE,  TRUE);

	float pointSize = 1.0f;
	float pointScale = 1.0f;
	d3d.lpD3DDevice->SetRenderState(D3DRS_POINTSIZE,	*((DWORD*)&pointSize));
	d3d.lpD3DDevice->SetRenderState(D3DRS_POINTSCALE_B, *((DWORD*)&pointScale));

//	ChangeFilteringMode(FILTERING_BILINEAR_OFF);
//	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
//	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);

	ChangeTranslucencyMode(TRANSLUCENCY_OFF);

	d3d.lpD3DDevice->SetRenderState(D3DRS_CULLMODE,			D3DCULL_NONE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_CLIPPING,			TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_LIGHTING,			FALSE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_SPECULARENABLE,	TRUE);

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

