#ifndef _INCLUDED_AW_H_
#define _INCLUDED_AW_H_

#include <stdint.h>

typedef struct AVPTEXTURE
{
	uint8_t *buffer;

	uint32_t width;
	uint32_t height;

} AVPTEXTURE;

// taken from d3dtypes.h
#define RGBA_MAKE(r, g, b, a)   ((D3DCOLOR) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))

#endif /* _INCLUDED_AW_H_ */
