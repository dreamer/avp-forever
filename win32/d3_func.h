#ifndef _included_d3_func_h_
#define _included_d3_func_h_

#ifdef WIN32
#include <d3d9.h>
#include <d3dx9.h>
#include <Dxerr.h>

typedef IDirect3DVertexBuffer9 r_VertexBuffer;
typedef IDirect3DIndexBuffer9  r_IndexBuffer;
typedef IDirect3DTexture9	   *r_Texture; // keep this as pointer type?

#include "aw.h"
#include <stdint.h>
#include "renderer.h"
#include "renderStates.h"
#include "textureManager.h"

bool R_CreateVertexBuffer(uint32_t length, uint32_t usage, r_VertexBuffer **vertexBuffer);
bool R_LockVertexBuffer(r_VertexBuffer *vertexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, void **data, enum R_USAGE usage);
bool R_UnlockVertexBuffer(r_VertexBuffer *vertexBuffer);
bool R_SetVertexBuffer(r_VertexBuffer *vertexBuffer, uint32_t FVFsize);
bool R_DrawPrimitive(uint32_t numPrimitives);

// texture functions
bool R_LockTexture(r_Texture texture, uint8_t **data, uint32_t *pitch);
bool R_UnlockTexture(r_Texture texture);
bool R_CreateTexture(uint32_t width, uint32_t height, uint32_t bpp, enum TextureUsage usageType, struct Texture &texture);
bool R_CreateTextureFromAvPTexture(AVPTEXTURE &AvPTexture, enum TextureUsage usageType, Texture &texture);
void R_ReleaseTexture(r_Texture &texture);

/*
  Direct3D globals
*/

/*
 *	Pre-DX8 vertex format
 *	taken from http://www.mvps.org/directx/articles/definitions_for_dx7_vertex_types.htm
 */

typedef struct _D3DTVERTEX
{
	float sx;
	float sy;
	float sz;

	D3DCOLOR color;
	D3DCOLOR specular;

	float tu;
	float tv;

} D3DLVERTEX;

#define D3DFVF_LVERTEX	(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

typedef struct POINTSPRITEVERTEX
{
	float x;
	float y;
	float z;

	float size;

	DWORD colour;

	float u;
	float v;

} POINTSPRITEVERTEX;

#define D3DFVF_POINTSPRITEVERTEX (D3DFVF_XYZ | D3DFVF_PSIZE | D3DFVF_DIFFUSE | D3DFVF_TEX1)

typedef struct ORTHOVERTEX
{
	float x;
	float y;
	float z;

	DWORD colour;	// Colour

	float u;
	float v;		// Texture coordinates

} ORTHOVERTEX;

// orthographic quad vertex format
#define D3DFVF_ORTHOVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

typedef struct FMVVERTEX
{
	float x;
	float y;
	float z;		// Position in 3d space

	float u1;
	float v1;		// Texture coordinates 1

	float u2;
	float v2;		// Texture coordinates 2

	float u3;
	float v3;		// Texture coordinates 3

} FMVVERTEX;

/*
  Maximum number of Direct3D drivers ever
  expected to be resident on the system.
*/
#define MAX_D3D_DRIVERS 5

/*
  Description of a D3D driver.
*/

typedef struct D3DDriverInfo
{
	D3DFORMAT				Formats[20];
	D3DADAPTER_IDENTIFIER9	AdapterInfo;
	D3DDISPLAYMODE			DisplayMode[100];
	uint32_t				NumModes;
} D3DDRIVERINFO;

typedef struct D3DInfo
{
    LPDIRECT3D9				lpD3D;
    LPDIRECT3DDEVICE9		lpD3DDevice;
    D3DVIEWPORT9			D3DViewport;
	D3DPRESENT_PARAMETERS	d3dpp;

	LPDIRECT3DVERTEXBUFFER9 lpD3DVertexBuffer;
	LPDIRECT3DINDEXBUFFER9	lpD3DIndexBuffer;

	LPDIRECT3DVERTEXBUFFER9 lpD3DOrthoVertexBuffer;
	LPDIRECT3DINDEXBUFFER9	lpD3DOrthoIndexBuffer;

	LPDIRECT3DVERTEXBUFFER9 lpD3DParticleVertexBuffer;
	LPDIRECT3DINDEXBUFFER9	lpD3DParticleIndexBuffer;

	LPDIRECT3DVERTEXDECLARATION9 vertexDecl;
	LPDIRECT3DVERTEXSHADER9      vertexShader;
	LPDIRECT3DPIXELSHADER9       pixelShader;

	LPDIRECT3DVERTEXDECLARATION9 orthoVertexDecl;
	LPDIRECT3DVERTEXSHADER9      orthoVertexShader;

	LPDIRECT3DVERTEXDECLARATION9 fmvVertexDecl;
	LPDIRECT3DVERTEXSHADER9      fmvVertexShader;
	LPDIRECT3DPIXELSHADER9       fmvPixelShader;

	LPDIRECT3DVERTEXDECLARATION9 cloudVertexDecl;
	LPDIRECT3DVERTEXSHADER9      cloudVertexShader;
	LPDIRECT3DPIXELSHADER9       cloudPixelShader;

    uint32_t				NumDrivers;
    uint32_t				CurrentDriver;
    D3DDRIVERINFO			Driver[MAX_D3D_DRIVERS];
    uint32_t				CurrentTextureFormat;
    uint32_t				NumTextureFormats;

	bool					supportsShaders;
	bool					supportsDynamicTextures;
} D3DINFO;

extern D3DINFO d3d;

r_Texture CreateD3DTexture(AVPTEXTURE *tex, uint32_t usage, D3DPOOL poolType);
r_Texture CreateD3DTexturePadded(AVPTEXTURE *tex, uint32_t *realWidth, uint32_t *realHeight);
r_Texture CreateD3DTallFontTexture(AVPTEXTURE *tex);

bool InitialiseDirect3D();
bool ChangeGameResolution	(uint32_t width, uint32_t height/*, uint32_t colour_depth*/);
void DrawAlphaMenuQuad		(uint32_t topX, uint32_t topY, uint32_t textureID, uint32_t alpha);
void DrawTallFontCharacter	(uint32_t topX, uint32_t topY, uint32_t textureID, uint32_t texU, uint32_t texV, uint32_t charWidth, uint32_t alpha);
void DrawCloudTable			(uint32_t topX, uint32_t topY, uint32_t wordLength, uint32_t alpha);
void DrawFadeQuad			(uint32_t topX, uint32_t topY, uint32_t alpha);
void DrawSmallMenuCharacter (uint32_t topX, uint32_t topY, uint32_t texU, uint32_t texV, uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha);
void DrawQuad				(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType);
void DrawFmvFrame			(uint32_t frameWidth, uint32_t frameHeight, uint32_t textureWidth, uint32_t textureHeight, r_Texture fmvTexture);
void DrawFmvFrame2(uint32_t frameWidth, uint32_t frameHeight, uint32_t *textures, uint32_t numTextures);
void NewQuad				(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t colour, uint32_t textureID, enum TRANSLUCENCY_TYPE translucencyType, float *UVList);
void CreateScreenShotImage();
void DeRedTexture(r_Texture texture);
void ReleaseD3DTexture(r_Texture *d3dTexture);
void SetTransforms();

extern uint32_t NO_TEXTURE;

D3DINFO GetD3DInfo();
char* GetDeviceName();
#define RGB_MAKE	D3DCOLOR_XRGB

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#endif /* ifdef WIN32 */

#endif /* ! _included_d3_func_h_ */
