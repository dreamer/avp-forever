#include <fstream>
#include <string>
#include <sstream>
#include "logString.h"
#include "configFile.h"
#include <assert.h>

extern "C"
{
#include "VideoModes.h"
#include <stdio.h>
#include "renderer.h"

extern D3DInfo d3d;

int CurrentVideoMode = 0;
DEVICEANDVIDEOMODE PreferredDeviceAndVideoMode;

void NextVideoMode2()
{
	if (++CurrentVideoMode >= d3d.Driver[d3d.CurrentDriver].NumModes)
	{
		CurrentVideoMode = 0;
	}
}

void PreviousVideoMode2()
{
	if (--CurrentVideoMode < 0)
	{
		CurrentVideoMode = d3d.Driver[d3d.CurrentDriver].NumModes - 1;
	}
}

char *GetVideoModeDescription2()
{
#ifdef _XBOX
	return "Microsoft Xbox";
#else
	return d3d.Driver[d3d.CurrentDriver].AdapterInfo.Description;
#endif
}

char *GetVideoModeDescription3() 
{
	static char buf[64];

	int colourDepth = 0;
	
	// determine colour depth from d3d format
	switch (d3d.Driver[d3d.CurrentDriver].DisplayMode[CurrentVideoMode].Format) 
	{
		case D3DFMT_X8R8G8B8:
			colourDepth = 32;
			break;
		case D3DFMT_A8R8G8B8:
			colourDepth = 32;
			break;
		case D3DFMT_R5G6B5:
			colourDepth = 16;
			break;
		case D3DFMT_X1R5G5B5:
			colourDepth = 16;
			break;
#ifdef _XBOX
		case D3DFMT_LIN_X8R8G8B8:
			colourDepth = 32;
			break;
#endif
		default: 
			colourDepth = 16;
			break;
	}

	assert (d3d.Driver[d3d.CurrentDriver].DisplayMode[CurrentVideoMode].Width != 0);
	assert (d3d.Driver[d3d.CurrentDriver].DisplayMode[CurrentVideoMode].Height != 0);

	sprintf(buf, "%dx%dx%d", d3d.Driver[d3d.CurrentDriver].DisplayMode[CurrentVideoMode].Width, d3d.Driver[d3d.CurrentDriver].DisplayMode[CurrentVideoMode].Height, colourDepth);
	return buf;
}

void GetDeviceAndVideoModePrefences() 
{
	int colourDepth = 0;

	for (int i = 0; i < d3d.Driver[d3d.CurrentDriver].NumModes; i++)
	{
		// determine colour depth from d3d format
		switch (d3d.Driver[d3d.CurrentDriver].DisplayMode[i].Format) 
		{
			case D3DFMT_X8R8G8B8:
				colourDepth = 32;
				break;
			case D3DFMT_A8R8G8B8:
				colourDepth = 32;
				break;
			case D3DFMT_R5G6B5:
				colourDepth = 16;
				break;
			case D3DFMT_X1R5G5B5:
				colourDepth = 16;
				break;
#ifdef _XBOX
			case D3DFMT_LIN_X8R8G8B8:
				colourDepth = 32;
				break;
#endif
			default: 
				colourDepth = 16;
				break;
		}

		if ((PreferredDeviceAndVideoMode.Width == d3d.Driver[d3d.CurrentDriver].DisplayMode[i].Width)
		  &&(PreferredDeviceAndVideoMode.Height == d3d.Driver[d3d.CurrentDriver].DisplayMode[i].Height)
		  &&(PreferredDeviceAndVideoMode.ColourDepth == colourDepth))
		{
			CurrentVideoMode = i;
			break;
		}
	}
}

void SelectBasicDeviceAndVideoMode() 
{
	// default to 800x600x32
	PreferredDeviceAndVideoMode.Width = 800;
	PreferredDeviceAndVideoMode.Height = 600;
	PreferredDeviceAndVideoMode.ColourDepth = 32;
}

void LoadDeviceAndVideoModePreferences()
{
	PreferredDeviceAndVideoMode.Width = Config_GetInt("[VideoMode]", "Width", 800);
	PreferredDeviceAndVideoMode.Height= Config_GetInt("[VideoMode]", "Height", 600);
	PreferredDeviceAndVideoMode.ColourDepth = Config_GetInt("[VideoMode]", "ColourDepth", 32);

	GetDeviceAndVideoModePrefences();
}

static void SetDeviceAndVideoModePreferences(void)
{
	PreferredDeviceAndVideoMode.Width = d3d.Driver[d3d.CurrentDriver].DisplayMode[CurrentVideoMode].Width;
	PreferredDeviceAndVideoMode.Height = d3d.Driver[d3d.CurrentDriver].DisplayMode[CurrentVideoMode].Height;

	int colourDepth = 0;

	// determine colour depth from d3d format
	switch (d3d.Driver[d3d.CurrentDriver].DisplayMode[CurrentVideoMode].Format)
	{
		case D3DFMT_X8R8G8B8:
			colourDepth = 32;
			break;
		case D3DFMT_A8R8G8B8:
			colourDepth = 32;
			break;
		case D3DFMT_R5G6B5:
			colourDepth = 16;
			break;
		case D3DFMT_X1R5G5B5:
			colourDepth = 16;
			break;
#ifdef _XBOX
		case D3DFMT_LIN_X8R8G8B8:
			colourDepth = 32;
			break;
#endif
		default: 
			colourDepth = 16;
			break;
	}

	PreferredDeviceAndVideoMode.ColourDepth = colourDepth;

	Config_SetInt("[VideoMode]", "Height", PreferredDeviceAndVideoMode.Height);
	Config_SetInt("[VideoMode]", "Width" , PreferredDeviceAndVideoMode.Width);
	Config_SetInt("[VideoMode]", "ColourDepth", PreferredDeviceAndVideoMode.ColourDepth);

	Config_Save();

	ChangeGameResolution(PreferredDeviceAndVideoMode.Width, PreferredDeviceAndVideoMode.Height, PreferredDeviceAndVideoMode.ColourDepth);
}

// called when you select "use selected settings" when selecting a video mode
void SaveDeviceAndVideoModePreferences()
{
	SetDeviceAndVideoModePreferences();
}

}

