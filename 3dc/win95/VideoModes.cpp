#include <string>
#include <assert.h>
#include "renderer.h"

extern "C"
{

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
	const uint32_t bufferSize = 64;

	static char buf[bufferSize + 1]; // always have room for a null character

	uint32_t sizeofText = R_GetVideoModeDescription().size();

	// ensure we don't overflow our char buffer
	if (sizeofText > bufferSize)
	{
		sizeofText = bufferSize;
	}

	strncpy(buf, R_GetVideoModeDescription().c_str(), sizeofText + 1); // sizeofText + 1 so strncpy will append a '0' to string

	return buf;
}

// called when you select "use selected settings" when selecting a video mode
void SetAndSaveDeviceAndVideoModePreferences()
{
	R_SetCurrentVideoMode();
}

}
