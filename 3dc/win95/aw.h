#ifndef _INCLUDED_AW_H_
#define _INCLUDED_AW_H_

#include <stdint.h>

struct AwBackupTexture;
typedef struct AwBackupTexture * AW_BACKUPTEXTUREHANDLE;

typedef struct AVPTEXTURE
{
	uint8_t *buffer;

	int width;
	int height;

} AVPTEXTURE;

// 32bit RGBA to 32bit D3DFMT_A8R8G8B8 format
#define RGB16(r, g, b) ( ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

// 32bit RGBA to 16bit D3DFMT_A1R5G5B5 format
#define ARGB1555(a, r, g, b) ( (a << 15)| ((r >> 3) << 10)| ((g >> 3) << 5)| (b >> 3))

// bjd - taken from d3dtypes.h
#define RGBA_MAKE(r, g, b, a)   ((D3DCOLOR) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

#endif /* _INCLUDED_AW_H_ */
