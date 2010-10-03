#ifdef WIN32

#ifndef _included_d3_func_h_
#define _included_d3_func_h_

#include <d3d9.h>
#include <d3dx9.h>
#include <Dxerr.h>
#include <stdint.h>
#include <string>

typedef D3DXMATRIX R_MATRIX;

struct r_VertexBuffer
{
	IDirect3DVertexBuffer9	*vertexBuffer;
};

struct r_IndexBuffer
{
	IDirect3DIndexBuffer9	*indexBuffer;
};

typedef IDirect3DTexture9	*r_Texture;		// keep this as pointer type?

typedef IDirect3DVertexDeclaration9 *r_vertexDeclaration;

struct Texture;

struct r_VertexShader
{
	bool 			isValid;
	uint32_t		refCount;
	IDirect3DVertexShader9	*shader;
	ID3DXConstantTable		*constantTable;
	std::string 	shaderName;
};

struct r_PixelShader
{
	bool			isValid;
	uint32_t		refCount;
	IDirect3DPixelShader9 *shader;
	std::string 	shaderName;
};

#include "aw.h"
#include "vertexBuffer.h"
#include "indexBuffer.h"
#include "renderStates.h"
#include "textureManager.h"
#include "shaderManager.h"
#include "VertexDeclaration.h"
#include <string>

bool R_BeginScene();
bool R_EndScene();

// vertex buffer functions
bool R_CreateVertexBuffer(uint32_t size, uint32_t usage, r_VertexBuffer &vertexBuffer);
bool R_ReleaseVertexBuffer(r_VertexBuffer &vertexBuffer);
bool R_LockVertexBuffer(r_VertexBuffer &vertexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, void **data, enum R_USAGE usage);
bool R_UnlockVertexBuffer(r_VertexBuffer &vertexBuffer);
bool R_SetVertexBuffer(r_VertexBuffer &vertexBuffer, uint32_t FVFsize);
bool R_DrawPrimitive(uint32_t numPrimitives);

bool R_DrawIndexedPrimitive(uint32_t numVerts, uint32_t startIndex, uint32_t numPrimitives);

// index buffer functions
bool R_CreateIndexBuffer(uint32_t size, uint32_t usage, r_IndexBuffer &indexBuffer);
bool R_ReleaseIndexBuffer(r_IndexBuffer &indexBuffer);
bool R_LockIndexBuffer(r_IndexBuffer &indexBuffer, uint32_t offsetToLock, uint32_t sizeToLock, uint16_t **data, enum R_USAGE usage);
bool R_UnlockIndexBuffer(r_IndexBuffer &indexBuffer);
bool R_SetIndexBuffer(r_IndexBuffer &indexBuffer);

// texture functions
bool R_SetTexture(uint32_t stage, uint32_t textureID);
bool R_LockTexture(r_Texture texture, uint8_t **data, uint32_t *pitch, enum TextureLock lockType);
bool R_UnlockTexture(r_Texture texture);
bool R_CreateTexture(uint32_t width, uint32_t height, uint32_t bpp, enum TextureUsage usageType, Texture &texture);
bool R_CreateTextureFromAvPTexture(AVPTEXTURE &AvPTexture, enum TextureUsage usageType, Texture &texture);
bool R_CreateTextureFromFile(const std::string &fileName, Texture &texture);
void R_ReleaseTexture(r_Texture &texture);
r_Texture CreateD3DTexture(AVPTEXTURE *tex, uint32_t usage, D3DPOOL poolType);
r_Texture CreateD3DTexturePadded(AVPTEXTURE *tex, uint32_t *realWidth, uint32_t *realHeight);
r_Texture CreateD3DTallFontTexture(AVPTEXTURE *tex);

// vertex declaration
bool R_CreateVertexDeclaration(r_vertexDeclaration &declaration, std::vector<struct vertexElement> &elements);
bool R_SetVertexDeclaration(r_vertexDeclaration &declaration);
bool R_ReleaseVertexDeclaration(r_vertexDeclaration &declaration);

// shader functions
bool R_CreateVertexShader(const std::string &fileName, r_VertexShader &vertexShader);
bool R_CreatePixelShader(const std::string &fileName, r_PixelShader &pixelShader);
bool R_SetVertexShader(r_VertexShader &vertexShader);
bool R_SetPixelShader(r_PixelShader &pixelShader);
void R_ReleaseVertexShader(r_VertexShader &vertexShader);
void R_ReleasePixelShader(r_PixelShader &pixelShader);
bool R_SetVertexShaderMatrix(r_VertexShader &vertexShader, const char* constant, R_MATRIX &matrix);

void R_NextVideoMode();
void R_PreviousVideoMode();
std::string& R_GetVideoModeDescription();
void R_SetCurrentVideoMode();

void ChangeTranslucencyMode(enum TRANSLUCENCY_TYPE translucencyRequired);
void ChangeTextureAddressMode(enum TEXTURE_ADDRESS_MODE textureAddressMode);
void ChangeFilteringMode(enum FILTERING_MODE_ID filteringRequired);

// vertex declarations
extern D3DVERTEXELEMENT9 declMain[];
extern D3DVERTEXELEMENT9 declOrtho[];
extern D3DVERTEXELEMENT9 declFMV[];
extern D3DVERTEXELEMENT9 declTallFontText[];

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

	// vertex and index buffers
	class VertexBuffer		*particleVB;
	class IndexBuffer		*particleIB;

	class VertexBuffer		*mainVB;
	class IndexBuffer		*mainIB;

	class VertexBuffer		*orthoVB;
	class IndexBuffer		*orthoIB;

	// effect manager to handle shaders
	class EffectManager		*effectSystem;

	// shader IDs
	uint32_t				mainEffect;
	uint32_t				orthoEffect;
	uint32_t				cloudEffect;
	uint32_t				fmvEffect;

	// vertex declarations
	class VertexDeclaration		*mainDecl;
	class VertexDeclaration		*orthoDecl;
	class VertexDeclaration		*fmvDecl;

	// enumeration
	uint32_t				NumDrivers;
	uint32_t				CurrentDriver;
	int32_t					CurrentVideoMode;
	D3DDRIVERINFO			Driver[MAX_D3D_DRIVERS];
	uint32_t				CurrentTextureFormat;
	uint32_t				NumTextureFormats;

	bool					supportsShaders;
	bool					supportsDynamicTextures;
} D3DINFO;

extern D3DINFO d3d;

bool InitialiseDirect3D();
bool R_ChangeResolution		(uint32_t width, uint32_t height);
void DrawAlphaMenuQuad		(uint32_t topX, uint32_t topY, uint32_t textureID, uint32_t alpha);
void DrawTallFontCharacter	(uint32_t topX, uint32_t topY, uint32_t textureID, uint32_t texU, uint32_t texV, uint32_t charWidth, uint32_t alpha);
void DrawCloudTable			(uint32_t topX, uint32_t topY, uint32_t wordLength, uint32_t alpha);
void DrawFadeQuad			(uint32_t topX, uint32_t topY, uint32_t alpha);
void DrawSmallMenuCharacter (uint32_t topX, uint32_t topY, uint32_t texU, uint32_t texV, uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha);
void DrawQuad				(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType);
void DrawFmvFrame			(uint32_t frameWidth, uint32_t frameHeight, uint32_t textureWidth, uint32_t textureHeight, r_Texture fmvTexture);
void DrawFmvFrame2			(uint32_t frameWidth, uint32_t frameHeight, uint32_t *textures, uint32_t numTextures);
void CreateScreenShotImage();
void DeRedTexture(r_Texture texture);
void ReleaseD3DTexture(r_Texture *d3dTexture);
void SetTransforms();

extern uint32_t NO_TEXTURE;

#define RGB_MAKE	D3DCOLOR_XRGB

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#endif /* ! _included_d3_func_h_ */

#endif /* ifdef WIN32 */
