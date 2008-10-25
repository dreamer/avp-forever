#ifndef _INCLUDED_AW_H_
#define _INCLUDED_AW_H_

typedef void* DDObject; // BJD DIRECT DRAW
typedef void* D3DDevice;//IDirect3DDevice9 *D3DDevice;
typedef void* DDPalette; // BJD DIRECT DRAW

#define GUID_D3D_TEXTURE IID_IDirect3DTexture9
#define GUID_DD_SURFACE IID_IDirectDrawSurface9

typedef void* DD_SURFACE_DESC;
typedef void* DD_S_CAPS;

struct AwBackupTexture;
typedef struct AwBackupTexture * AW_BACKUPTEXTUREHANDLE;

typedef struct DIRECTDRAWSURFACE
{
	unsigned char *buffer;

	int width;
	int height;

} DIRECTDRAWSURFACE;

typedef DIRECTDRAWSURFACE * LPDIRECTDRAWSURFACE;
typedef DIRECTDRAWSURFACE DDSurface;

typedef struct DIRECT3DTEXTURE
{
	unsigned char *buffer;

	int width;
	int height;

} DIRECT3DTEXTURE;

typedef DIRECT3DTEXTURE * LPDIRECT3DTEXTURE;
typedef DIRECT3DTEXTURE AVPTexture;

typedef LPDIRECT3DTEXTURE9 D3DTEXTUREHANDLE; 

//typedef int D3DTEXTUREHANDLE;

/* Pre-DX8 vertex formats
	taken from http://www.mvps.org/directx/articles/definitions_for_dx7_vertex_types.htm
*/

//#define D3DTLFVF_VERTEX    D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1

typedef struct _D3DTLVERTEX {
    union {
        float sx;
        float dvSX;
    };
    union {
        float sy;
        float dvSY;
    };
    union {
        float sz;
        float dvSZ;
    };
    union {
        float rhw;
        float dvRHW;
    };
    union {
        D3DCOLOR color;
        D3DCOLOR dcColor;
    };
    union {
        D3DCOLOR specular;
        D3DCOLOR dcSpecular;
    };
    union {
        float tu;
        float dvTU;
    };
    union {
        float tv;
        float dvTV;
    };
#if (defined __cplusplus) && (defined D3D_OVERLOADS)
    _D3DTLVERTEX() { }
    _D3DTLVERTEX(const D3DVECTOR& v,float w,D3DCOLOR col,D3DCOLOR spec, float _tu, float _tv)
        { sx = v.x; sy = v.y; sz = v.z; rhw=w;
          color = col; specular = spec;
          tu = _tu; tv = _tv;
    }
#endif
} D3DTLVERTEX, *LPD3DTLVERTEX;

#define D3DFVF_TLVERTEX	(D3DFVF_XYZRHW|/*D3DFVF_RESERVED0|*/D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1)

// 32bit RGBA to 32bit D3DFMT_A8R8G8B8 format
#define RGB16(r, g, b) ( ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

// 32bit RGBA to 16bit D3DFMT_A1R5G5B5 format
#define ARGB1555(a, r, g, b) ( (a << 15)| ((r >> 3) << 10)| ((g >> 3) << 5)| (b >> 3))

// R G B A
//#define RGB32(r, g, b) ( (255) | (r >> 8) | (g >> 8) | (b >> 8))
//#define RGB32 (r << 24) | (g << 16) | (b << 8) | a);

// BJD - taken from d3dtypes.h
#define RGBA_MAKE(r, g, b, a)   ((D3DCOLOR) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
//#define RGBA_MAKE(r, g, b, a)	D3DCOLOR_ARGB(a,r,g,b)

#endif /* _INCLUDED_AW_H_ */
