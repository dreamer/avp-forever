#include "gles.h"
#include "GLES/gl.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <EGL/egl.h>
#include "SDL_syswm.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

typedef struct
{
	EGLDisplay		eglDisplay;
	EGLConfig		eglConfig;
	EGLSurface		eglSurface;
	EGLContext		eglContext;
	int fbdev;
	SDL_Surface *	screen;
} GLES_Data;

static GLES_Data* G_Data = NULL;

#define CHK_FREE_RET( chk, ptr, ret ) \
	if ( chk ) {																\
		fprintf( stderr, "ERROR: %s at %s(%d)\n", #chk, __FILE__, __LINE__ );	\
		GLES_Shutdown( );														\
		return ret;																\
	}

static int GLES_TestError(const char* msg)
{
	EGLint err = eglGetError();
	if (err!=EGL_SUCCESS) {
		fprintf(stderr,"EGL ERROR: 0x%x near %s\n",err,msg);
		return 1;
	}

	err = glGetError();
	if(err!=GL_NO_ERROR) {
		fprintf(stderr,"GL ERROR: 0x%x near %s\n",err,msg);
		return 1;
	}

	return 0;
}

SDL_Surface * GLES_Init( )
{
	// If already initialised, shutdown first.
	GLES_Shutdown( );

	// Config we want.
	EGLint egl_config[] =
	{
		EGL_BUFFER_SIZE, 16,
		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,
		EGL_ALPHA_SIZE, 0,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, EGL_DONT_CARE,
		EGL_CONFIG_CAVEAT, EGL_NONE,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
		EGL_NONE
	};

	// Allocate our global data and zero it out.
	GLES_Data* data = ( GLES_Data * ) malloc( sizeof( GLES_Data ) );
	memset( data, 0, sizeof( GLES_Data ) );

	// Get full screen SDL context.
	data->screen = SDL_SetVideoMode( 0, 0, 0, SDL_FULLSCREEN );
	CHK_FREE_RET(data->screen==NULL,data,0);

	// Get window information from SDL.
	SDL_SysWMinfo  sysWmInfo;
	SDL_VERSION(&sysWmInfo.version);
	SDL_GetWMInfo(&sysWmInfo);

	// Get EGL display (passing in display from SDL/X11).
	data->eglDisplay = eglGetDisplay( ( EGLNativeDisplayType )sysWmInfo.info.x11.display );
	CHK_FREE_RET(data->eglDisplay==EGL_NO_DISPLAY,data,0);
	CHK_FREE_RET(GLES_TestError("eglGetDisplay"),data,0);

	// Initialise EGL display.
	EGLBoolean r = eglInitialize( data->eglDisplay,0,0 );
	CHK_FREE_RET(!r,data,0);
	CHK_FREE_RET(GLES_TestError("eglInitialize"),data,0);

	// Get config we set up.
	int iConfigs;
	r = eglChooseConfig(data->eglDisplay, egl_config, &data->eglConfig, 1, &iConfigs);
	CHK_FREE_RET(!r||iConfigs!=1,data,0);
	CHK_FREE_RET(GLES_TestError("eglChooseConfig"),data,0);

	// Create window surface.
	data->eglSurface = eglCreateWindowSurface(data->eglDisplay, data->eglConfig, (NativeWindowType)sysWmInfo.info.x11.window, NULL);
	CHK_FREE_RET(data->eglSurface==EGL_NO_SURFACE,data,0);
	CHK_FREE_RET(GLES_TestError("eglCreateWindowSurface"),data,0);

	// GLES version 1.
	EGLint contextAttribs[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 1,
		EGL_NONE
	};

	// Create context.
	data->eglContext = eglCreateContext(data->eglDisplay,data->eglConfig,NULL,contextAttribs);
	CHK_FREE_RET(data->eglContext==EGL_NO_CONTEXT,data,0);
	CHK_FREE_RET(GLES_TestError("eglCreateContext"),data,0);

	// Make context current.
	eglMakeCurrent(data->eglDisplay,data->eglSurface,data->eglSurface,data->eglContext);
	GLES_TestError("eglMakeCurrent");

	// Open frame buffer (for vsync).
	data->fbdev = open("/dev/fb0",O_RDONLY);

	// Store our data globally so we can use it in other functions.
	G_Data = data;

	// Return SDL screen to calling code.
	return data->screen;
}

int GLES_SwapBuffers( )
{
	GLES_Data* data = G_Data;

	if (data->eglDisplay!=EGL_NO_DISPLAY && data->eglSurface!=EGL_NO_SURFACE)
	{
		if ( data->fbdev>=0 )
		{
			int arg = 0;
			ioctl( data->fbdev, _IOW('F', 0x20, __u32), &arg );
		}
		eglSwapBuffers(data->eglDisplay,data->eglSurface);

		return 1;
	}

	return 0;
}

int GLES_Shutdown( )
{
	GLES_Data* data = G_Data;
	if ( data ) 
	{
		if (data->eglDisplay!=EGL_NO_DISPLAY) 
			eglMakeCurrent(data->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
		if (data->eglContext!=EGL_NO_CONTEXT) 
			eglDestroyContext ( data->eglDisplay, data->eglContext );
		if (data->eglSurface!=EGL_NO_SURFACE) 
			eglDestroySurface ( data->eglDisplay, data->eglSurface );
		if (data->eglDisplay!=EGL_NO_DISPLAY) 
			eglTerminate ( data->eglDisplay );
		if (data->fbdev>=0) 
			close(data->fbdev);

		free( data );
	}
	G_Data = NULL;

	return 0;
}
