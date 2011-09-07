#include <string>
#include <assert.h>
#include "renderer.h"

void NextVideoMode2()
{
	R_NextVideoMode();
}

void PreviousVideoMode2()
{
	R_PreviousVideoMode();
}

char *GetVideoModeDescription2()
{
	return R_GetDeviceName();
}

char *GetVideoModeDescription3() 
{
	const uint32_t kBufferSize = 32;

	static char buf[kBufferSize + 1]; // always have room for a null character
	buf[kBufferSize] = '\0';

	size_t sizeofText = R_GetVideoModeDescription().size(); // number of chars in string (doesn't include a null terminator)

	// ensure we don't overflow our char buffer
	if (sizeofText > kBufferSize)
	{
		sizeofText = kBufferSize;
	}

	// copy the string to buf
	memcpy(&buf[0], R_GetVideoModeDescription().c_str(), sizeofText+1); // R_GetVideoModeDescription().c_str() returns a null terminated string

	return buf;
}

// called when you select "use selected settings" when selecting a video mode
void SetAndSaveDeviceAndVideoModePreferences()
{
	R_SetCurrentVideoMode();
}

