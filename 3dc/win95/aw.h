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
typedef uint32_t RCOLOR;
#define RGBA_MAKE(r, g, b, a)   ((uint32_t) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b)))
#define RCOLOR_ARGB(a, r, g, b) ((uint32_t) ((((a)&0xff)<<24) | (((r)&0xff)<<16) | (((g)&0xff)<<8) | ((b)&0xff)))
#define RCOLOR_XRGB(r,g,b)		RCOLOR_ARGB(0xff, r, g, b)

#endif /* _INCLUDED_AW_H_ */
