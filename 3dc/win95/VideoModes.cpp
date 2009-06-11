#include <fstream>
#include <string>
#include <sstream>
#include "logString.h"

extern "C"
{
#include "VideoModes.h"
#include <stdio.h>
#include "d3_func.h"

extern D3DInfo d3d;

int CurrentVideoMode = 0;
DEVICEANDVIDEOMODE PreferredDeviceAndVideoMode;

void NextVideoMode2() 
{
	if (++CurrentVideoMode >= d3d.NumModes)
	{
		CurrentVideoMode = 0;
	}
}

void PreviousVideoMode2() 
{
	if (--CurrentVideoMode < 0)
	{
		CurrentVideoMode = d3d.NumModes - 1;
	}
}

char *GetVideoModeDescription2()
{
#ifdef _XBOX
	char *description = "Microsoft Xbox";
	return description;
#else
	return d3d.AdapterInfo.Description;
#endif
}

char *GetVideoModeDescription3() 
{
	static char buf[64];

	int colourDepth = 0;
	
	// determine colour depth from d3d format
	switch(d3d.DisplayMode[CurrentVideoMode].Format) 
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
	sprintf(buf, "%dx%dx%d", d3d.DisplayMode[CurrentVideoMode].Width, d3d.DisplayMode[CurrentVideoMode].Height, colourDepth);
	return buf;
}

void GetDeviceAndVideoModePrefences() 
{
	int colourDepth = 0;

	for (int i=0; i < d3d.NumModes; i++)
	{
		// determine colour depth from d3d format
		switch(d3d.DisplayMode[i].Format) 
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

		if ((PreferredDeviceAndVideoMode.Width == d3d.DisplayMode[i].Width)
		  &&(PreferredDeviceAndVideoMode.Height == d3d.DisplayMode[i].Height)
		  &&(PreferredDeviceAndVideoMode.ColourDepth == colourDepth))
		{
			CurrentVideoMode = i;
			break;
		}
	}
}

void SelectBasicDeviceAndVideoMode() 
{
	// default to 640x480x16
	PreferredDeviceAndVideoMode.Width = 640;
	PreferredDeviceAndVideoMode.Height = 480;
	PreferredDeviceAndVideoMode.ColourDepth = 16;
	
	// create new file here?
#ifdef _XBOX
	std::ofstream file("d:\\AliensVsPredator.cfg");
#else
	std::ofstream file("AliensVsPredator.cfg");
#endif
	file << "[VideoMode]\n";
	file << "Width = " << PreferredDeviceAndVideoMode.Width << "\n";
	file << "Height = " << PreferredDeviceAndVideoMode.Height << "\n";
	file << "ColourDepth = " << PreferredDeviceAndVideoMode.ColourDepth << "\n";
	file.close();
}

/* parses an int from a string and returns it */
int StringToInt(const std::string &string)
{
	std::stringstream ss;
	int value = 0;

	/* copy string to stringstream */
	ss << string;
	/* copy from stringstream to int */
	ss >> value;
	
	return value;
}

void LoadDeviceAndVideoModePreferences() 
{
#ifdef _XBOX
	std::ifstream file("d:\\AliensVsPredator.cfg");
#else
	std::ifstream file("AliensVsPredator.cfg");
#endif
	
	// if the file doesn't exist
	if(!file)
	{
		LogErrorString("Can't find file AliensVsPredator.cfg - creating and using basic display mode", __LINE__, __FILE__);
		SelectBasicDeviceAndVideoMode();
		return;
	}

	std::string temp;

	getline(file, temp);
	if(temp != "[VideoMode]") 
	{
		OutputDebugString("didn't find [VideoMode] config header\n");
	}

	getline (file,temp, '=');
	if (temp == "Width ") 
	{
		file.ignore(1);
		getline(file, temp);

		/* convert our string to an int */
		PreferredDeviceAndVideoMode.Width = StringToInt(temp);
	}
	else {
		OutputDebugString("nope, wasnt it\n");
	}

	getline (file,temp, '=');
	if (temp == "Height ") 
	{
		file.ignore(1);
		getline(file, temp);

		/* convert our string to an int */
		PreferredDeviceAndVideoMode.Height = StringToInt(temp);
	}
	else {
		OutputDebugString("nope, wasnt it\n");
	}

	getline (file,temp, '=');
	if (temp == "ColourDepth ") 
	{
		file.ignore(1);
		getline(file, temp);

		/* convert our string to an int */
		int value = StringToInt(temp);

		if(value == 32 || value == 16) PreferredDeviceAndVideoMode.ColourDepth = value;
		else PreferredDeviceAndVideoMode.ColourDepth = 16;
	}
	else {
		OutputDebugString("nope, wasnt it\n");
	}
/*
	char buf[100];
	sprintf(buf, "width: %d, height: %d depth: %d\n", PreferredDeviceAndVideoMode.Width, PreferredDeviceAndVideoMode.Height, PreferredDeviceAndVideoMode.ColourDepth);
	OutputDebugString(buf);
*/
	file.close();

	GetDeviceAndVideoModePrefences();
}

//const int TotalVideoModes = sizeof(VideoModeList) / sizeof(VideoModeList[0]);

static void SetDeviceAndVideoModePreferences(void)
{
	PreferredDeviceAndVideoMode.Width = d3d.DisplayMode[CurrentVideoMode].Width;
	PreferredDeviceAndVideoMode.Height = d3d.DisplayMode[CurrentVideoMode].Height;

	int colourDepth = 0;
	
	// determine colour depth from d3d format
	switch(d3d.DisplayMode[CurrentVideoMode].Format)
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
	/*
	DEVICEANDVIDEOMODESDESC *dPtr = &DeviceDescriptions[CurrentlySelectedDevice];
	VIDEOMODEDESC *vmPtr = &(dPtr->VideoModes[CurrentlySelectedVideoMode]);

	PreferredDeviceAndVideoMode.DDGUID = dPtr->DDGUID;
	PreferredDeviceAndVideoMode.DDGUIDIsSet = dPtr->DDGUIDIsSet;
	PreferredDeviceAndVideoMode.Width = vmPtr->Width;
	PreferredDeviceAndVideoMode.Height = vmPtr->Height;
	PreferredDeviceAndVideoMode.ColourDepth = vmPtr->ColourDepth;
	*/
}

// called when you select "use selected settings" when selecting a video mode
void SaveDeviceAndVideoModePreferences() 
{
	//	FILE* file=fopen("AliensVsPredator.cfg","rb");
#ifdef _XBOX
	std::ofstream file("d:\\AliensVsPredator.cfg");
#else
	std::ofstream file("AliensVsPredator.cfg");
#endif
	
	// if the file doesn't exist
	if(!file)
	{
		OutputDebugString("can't find file AliensVsPredator.cfg\n");
		SelectBasicDeviceAndVideoMode();
		return;
	}

	SetDeviceAndVideoModePreferences();

	file << "[VideoMode]\n";
	file << "Width = " << PreferredDeviceAndVideoMode.Width << "\n";
	file << "Height = " << PreferredDeviceAndVideoMode.Height << "\n";
	file << "ColourDepth = " << PreferredDeviceAndVideoMode.ColourDepth << "\n";
	file.close();
}

};


