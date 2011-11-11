#ifndef _FmvPlayback_h_
#define _FmvPlayback_h_


#include "RingBuffer.h"
#include "AudioStreaming.h"
#include "TextureManager.h"

enum fmvErrors
{
	FMV_OK,
	FMV_INVALID_FILE,
	FMV_ERROR
};

/* structure holds pointers to y, u, v channels */
typedef struct _OggPlayYUVChannels 
{
	unsigned char * ptry;
	unsigned char * ptru;
	unsigned char * ptrv;
	int				y_width;
	int				y_height;
	int				y_pitch;
	int				uv_width;
	int				uv_height;
	int				uv_pitch;
} OggPlayYUVChannels;

/* structure holds pointers to y, u, v channels */
typedef struct _OggPlayRGBChannels 
{
	unsigned char * ptro;
	int				rgb_width;
	int				rgb_height;
	int				rgb_pitch;
} OggPlayRGBChannels;

void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb);




#endif
