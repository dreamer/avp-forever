#ifndef _included_d3_func_h_
#define _included_d3_func_h_

#include <xtl.h>
#include <stdint.h>
#include "aw.h"
#include <string>
#include <vector>

typedef D3DXMATRIX R_MATRIX;

struct r_VertexBuffer
{
	IDirect3DVertexBuffer8 *vertexBuffer;
	uint8_t *dynamicVBMemory;
	size_t	dynamicVBMemorySize;
	uint32_t stride;

	// constructor to ensure above pointers are null
	r_VertexBuffer()
	{
		vertexBuffer    = 0;
		dynamicVBMemory = 0;
	}
};

struct r_IndexBuffer
{
	IDirect3DIndexBuffer8 *indexBuffer;
	uint8_t *dynamicIBMemory;
	size_t	dynamicIBMemorySize;

	// constructor to ensure above pointers are null
	r_IndexBuffer()
	{
		indexBuffer     = 0;
		dynamicIBMemory = 0;
	}
};

typedef IDirect3DTexture8		*r_Texture;		// keep this as pointer type?

typedef DWORD *r_vertexDeclaration;

struct r_VertexShader
{
	bool 			isValid;
	uint32_t		refCount;
	DWORD			shader;
	r_vertexDeclaration		vertexDeclaration;
	std::string 	shaderName;

	r_VertexShader()
	{
		isValid  = false;
		refCount = 0;
		shader   = 0;
		vertexDeclaration = 0;
	}
};

struct r_PixelShader
{
	bool			isValid;
	uint32_t		refCount;
	DWORD			shader;
	std::string 	shaderName;

	r_PixelShader()
	{
		isValid  = false;
		refCount = 0;
		shader   = 0;
	}
};

#define D3DLOCK_DISCARD	0

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

struct D3DLVERTEX
{
	float x;
	float y;
	float z;

	D3DCOLOR color;
	D3DCOLOR specular;

	float u;
	float v;
};

struct ORTHOVERTEX
{
	float x;
	float y;
	float z;

	DWORD colour;

	float u;
	float v;
};

struct PARTICLEVERTEX
{
	float x;
	float y;
	float z;

	DWORD colour;

	float u;
	float v;
};

struct TALLFONT
{
	float x;
	float y;
	float z;

	DWORD colour;

	float u1;
	float v1;

	float u2;
	float v2;
};


struct FMVVERTEX
{
	float x;
	float y;
	float z;

	float u1;
	float v1;        // Texture coordinates 1

	float u2;
	float v2;        // Texture coordinates 2

	float u3;
	float v3;        // Texture coordinates 3
};

struct DECAL_VERTEX
{
	float x;
	float y;
	float z;

	DWORD colour;

	float u;
	float v;
};

struct RHW_VERTEX
{
	float x;
	float y;
	float z;
	float rhw;

	D3DCOLOR color;

	float u;
	float v;
};

/*
  Maximum number of Direct3D drivers ever
  expected to be resident on the system.
*/
#define MAX_D3D_DRIVERS 2

/*
  Maximum number of texture formats ever
  expected to be reported by a Direct3D
  driver.
*/
#define MAX_TEXTURE_FORMATS 10

/*
  Description of a D3D driver.
*/

typedef struct D3DDriverInfo
{
	D3DFORMAT				Formats[20];
	D3DADAPTER_IDENTIFIER8	AdapterInfo;
	D3DDISPLAYMODE			DisplayMode[100];
	uint32_t				NumModes;
} D3DDRIVERINFO;


typedef struct D3DInfo
{
	LPDIRECT3D8				lpD3D;
	LPDIRECT3DDEVICE8		lpD3DDevice;
	D3DVIEWPORT8			D3DViewport;
	D3DPRESENT_PARAMETERS	d3dpp;

	// vertex and index buffers
	class VertexBuffer		*mainVB;
	class IndexBuffer		*mainIB;

	class VertexBuffer		*orthoVB;
	class IndexBuffer		*orthoIB;

	class VertexBuffer		*particleVB;
	class IndexBuffer		*particleIB;

	class VertexBuffer		*decalVB;
	class IndexBuffer		*decalIB;

	// effect manager to handle shaders
	class EffectManager		*effectSystem;

	// shader IDs
	effectID_t				mainEffect;
	effectID_t				decalEffect;
	effectID_t				orthoEffect;
	effectID_t				tallTextEffect;
	effectID_t				fmvEffect;
	effectID_t				particleEffect;
	effectID_t				rhwEffect;

	// vertex declarations
	class VertexDeclaration		*mainDecl;
	class VertexDeclaration		*orthoDecl;
	class VertexDeclaration		*decalDecl;
	class VertexDeclaration		*fmvDecl;
	class VertexDeclaration		*tallTextDecl;
	class VertexDeclaration		*particleDecl;
	class VertexDeclaration		*rhwDecl;

	// matrices
	D3DXMATRIX matOrtho;
	D3DXMATRIX matAlienWeaponProjection;
	D3DXMATRIX matProjection;
	D3DXMATRIX matView;
	D3DXMATRIX matIdentity;
	D3DXMATRIX matViewPort;

	D3DXPLANE  frustumPlanes[6];

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

#define SAFE_DELETE(p) { delete (p); (p) = 0; }
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#endif /* ! _included_d3_func_h_ */
