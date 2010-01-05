#ifndef _INCLUDED_AW_H_
#define _INCLUDED_AW_H_

struct AwBackupTexture;
typedef struct AwBackupTexture * AW_BACKUPTEXTUREHANDLE;

typedef struct AVPTEXTURE
{
	unsigned char *buffer;

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

} D3DTLVERTEX, *LPD3DTLVERTEX;

#define D3DFVF_TLVERTEX	(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1)

// 32bit RGBA to 32bit D3DFMT_A8R8G8B8 format
#define RGB16(r, g, b) ( ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

// 32bit RGBA to 16bit D3DFMT_A1R5G5B5 format
#define ARGB1555(a, r, g, b) ( (a << 15)| ((r >> 3) << 10)| ((g >> 3) << 5)| (b >> 3))

// bjd - taken from d3dtypes.h
#define RGBA_MAKE(r, g, b, a)   ((D3DCOLOR) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

#endif /* _INCLUDED_AW_H_ */