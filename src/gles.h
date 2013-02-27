#ifndef __GLES_H__
#define __GLES_H__

#include "SDL_video.h"

#if defined(__cplusplus)
extern "C" {
#endif

SDL_Surface * GLES_Init( );
int GLES_SwapBuffers( );
int GLES_Shutdown( );

#if defined(__cplusplus)
};
#endif

#endif
