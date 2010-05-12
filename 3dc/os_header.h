#ifdef WIN32

	/*
		Minimise header files to
		speed compiles...
	*/
	#define WIN32_LEAN_AND_MEAN

	#include <windows.h>
	#include <mmsystem.h>

#endif

#ifdef _XBOX

	#include <xtl.h>

#endif