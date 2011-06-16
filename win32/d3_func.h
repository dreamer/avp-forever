#ifdef WIN32

#ifndef _included_d3_func_h_
#define _included_d3_func_h_

#include <d3d9.h>
#include <d3dx9.h>
#include <Dxerr.h>
#include <stdint.h>
#include "aw.h"
#include <string>
#include <vector>

typedef D3DXMATRIX R_MATRIX;

struct r_VertexBuffer
{
	IDirect3DVertexBuffer9	*vertexBuffer;

	// constructor to ensure above pointer is null
	r_VertexBuffer()
	{
		vertexBuffer = 0;
	}
};

struct r_IndexBuffer
{
	IDirect3DIndexBuffer9	*indexBuffer;
	
	// constructor to ensure above pointer is null
	r_IndexBuffer()
	{
		indexBuffer = 0;
	}
};

typedef IDirect3DTexture9	*r_Texture;		// keep this as pointer type?

typedef IDirect3DVertexDeclaration9 *r_vertexDeclaration;

struct r_VertexShader
{
	bool 			isValid;
	int32_t			refCount;
	IDirect3DVertexShader9	*shader;
	ID3DXConstantTable		*constantTable;
	r_vertexDeclaration		vertexDeclaration;
	std::string 			shaderName;
	std::vector<D3DXHANDLE>	constantsArray;

	r_VertexShader()
	{
		isValid = false;
		refCount = 0;
		shader = 0;
		constantTable = 0;
		vertexDeclaration = 0;
	}
};

struct r_PixelShader
{
	bool			isValid;
	int32_t			refCount;
	IDirect3DPixelShader9	*shader;
	ID3DXConstantTable		*constantTable;
	std::string 			shaderName;
	std::vector<D3DXHANDLE>	constantsArray;

	r_PixelShader()
	{
		isValid = false;
		refCount = 0;
		shader = 0;
		constantTable = 0;
	}
};

#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "ShaderManager.h"
#include "VertexDeclaration.h"
#include "TextureManager.h"

/*
  Direct3D globals
*/

/*
 *	Pre-DX8 vertex format
 *	taken from http://www.mvps.org/directx/articles/definitions_for_dx7_vertex_types.htm
 */

typedef struct _D3DLVERTEX
{
	float x;
	float y;
	float z;

	D3DCOLOR color;
	D3DCOLOR specular;

	float u;
	float v;

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

	class VertexBuffer		*starsVB;
	class IndexBuffer		*starsIB;

	// effect manager to handle shaders
	class EffectManager		*effectSystem;

	// shader IDs
	effectID_t				mainEffect;
	effectID_t				orthoEffect;
	effectID_t				cloudEffect;
	effectID_t				fmvEffect;
	effectID_t				starsEffect;

	// vertex declarations
	class VertexDeclaration		*mainDecl;
	class VertexDeclaration		*orthoDecl;
	class VertexDeclaration		*fmvDecl;
	class VertexDeclaration		*tallFontText;

	// enumeration
	uint32_t				NumDrivers;
	uint32_t				CurrentDriver;
	int32_t					CurrentVideoMode;
	D3DDRIVERINFO			Driver[MAX_D3D_DRIVERS];
	uint32_t				CurrentTextureFormat;
	uint32_t				NumTextureFormats;

	bool					supportsShaders;
	bool					supportsDynamicTextures;

	uint32_t				fieldOfView;
	float					aspectRatio;
} D3DINFO;

extern D3DINFO d3d;

uint32_t XPercentToScreen(float percent);
uint32_t YPercentToScreen(float percent);

uint32_t R_GetScreenWidth();

extern texID_t NO_TEXTURE;
extern texID_t MISSING_TEXTURE;

#define RGB_MAKE	D3DCOLOR_XRGB

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#endif /* ! _included_d3_func_h_ */

#endif /* ifdef WIN32 */
