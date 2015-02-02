#ifndef _os_header_h_
#define _os_header_h_

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX

	#include <windows.h>
	#include <mmsystem.h>
#endif

#ifdef _XBOX
	#include <xtl.h>
#endif

#endif
