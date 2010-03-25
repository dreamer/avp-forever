#ifndef _INCLUDED_AW_H_
#define _INCLUDED_AW_H_

#include "stdint.h"

struct AwBackupTexture;
typedef struct AwBackupTexture * AW_BACKUPTEXTUREHANDLE;

typedef struct AVPTEXTURE
{
	uint8_t *buffer;

	int width;
	int height;

} AVPTEXTURE;

#ifdef WIN32
typedef LPDIRECT3DTEXTURE9 D3DTEXTURE;
#endif

#ifdef _XBOX
typedef LPDIRECT3DTEXTURE8 D3DTEXTURE;
#endif

typedef int D3DTEXTUREHANDLE;

/* Pre-DX8 vertex formats
	taken from http://www.mvps.org/directx/articles/definitions_for_dx7_vertex_types.htm
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

//#define D3DFVF_TLVERTEX	(D3DFVF_XYZ/*RHW*/ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)
//#define D3DFVF_HARDWARETLVERTEX	(D3DFVF_XYZ/*W*/ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)
#define D3DFVF_LVERTEX	(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

typedef struct ORTHOVERTEX 
{
	float x;
	float y;
	float z;		// Position in 3d space 

	DWORD colour;	// Colour  

	float u;
	float v;		// Texture coordinates 

} ORTHOVERTEX;

// orthographic quad vertex format
#define D3DFVF_ORTHOVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

// 32bit RGBA to 32bit D3DFMT_A8R8G8B8 format
#define RGB16(r, g, b) ( ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

// 32bit RGBA to 16bit D3DFMT_A1R5G5B5 format
#define ARGB1555(a, r, g, b) ( (a << 15)| ((r >> 3) << 10)| ((g >> 3) << 5)| (b >> 3))

// bjd - taken from d3dtypes.h
#define RGBA_MAKE(r, g, b, a)   ((D3DCOLOR) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

#endif /* _INCLUDED_AW_H_ */