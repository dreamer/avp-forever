//#ifndef WIN32
extern "C" {

#include "d3_func.h"
#include "fmv.h"
#include "avp_userprofile.h"

int SmackerSoundVolume=65536/512;
int MoviesAreActive;
int IntroOutroMoviesAreActive=1;

#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];
int NumberOfFMVTextures;

extern IMAGEHEADER ImageHeaderArray[];
#if MaxImageGroups>1
	extern int NumImagesArray[];
#else
	extern int NumImages;
#endif

int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;

extern void EndMenuBackgroundBink()
{
}

extern void StartMenuBackgroundBink()
{
}

extern void PlayBinkedFMV(char *filenamePtr)
{
}

extern int PlayMenuBackgroundBink()
{
	return 0;
}

void UpdateAllFMVTextures()
{
	extern void UpdateFMVTexture(FMVTEXTURE *ftPtr);
	int i = NumberOfFMVTextures;

	while(i--)
	{
		UpdateFMVTexture(&FMVTexture[i]);
	}
}

extern void StartTriggerPlotFMV(int number)
{
	int i = NumberOfFMVTextures;
	char buffer[25];

	if (CheatMode_Active != CHEATMODE_NONACTIVE) return;

	sprintf(buffer,"FMVs//message%d.smk",number);
	{
		FILE* file=fopen(buffer,"rb");
		if(!file)
		{
			return;
		}
		fclose(file);
	}
	while(i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			FMVTexture[i].MessageNumber = number;
		}
	}
}

void StartMenuMusic()
{
}

extern void StartFMVAtFrame(int number, int frame)
{ 
}

void ScanImagesForFMVs()
{
	extern void SetupFMVTexture(FMVTEXTURE *ftPtr);
	int i;
	IMAGEHEADER *ihPtr;
	NumberOfFMVTextures=0;

	#if MaxImageGroups>1
	for (j=0; j<MaxImageGroups; j++)
	{
		if (NumImagesArray[j])
		{
			ihPtr = &ImageHeaderArray[j*MaxImages];
			for (i = 0; i<NumImagesArray[j]; i++, ihPtr++)
			{
	#else
	{
		if(NumImages)
		{
			ihPtr = &ImageHeaderArray[0];
			for (i = 0; i<NumImages; i++, ihPtr++)
			{
	#endif
				char *strPtr;
				if(strPtr = strstr(ihPtr->ImageName,"FMVs"))
				{
					Smack *smackHandle;
					char filename[30];
					{
						char *filenamePtr = filename;
						do
						{
							*filenamePtr++ = *strPtr;
						}
						while(*strPtr++!='.');

						*filenamePtr++='s';
						*filenamePtr++='m';
						*filenamePtr++='k';
						*filenamePtr=0;
					}
					
					smackHandle = 0;
					if (smackHandle)
					{
						FMVTexture[NumberOfFMVTextures].IsTriggeredPlotFMV = 0;
					}
					else
					{
						FMVTexture[NumberOfFMVTextures].IsTriggeredPlotFMV = 1;
					}

					{
						FMVTexture[NumberOfFMVTextures].SmackHandle = smackHandle;
						FMVTexture[NumberOfFMVTextures].ImagePtr = ihPtr;
						FMVTexture[NumberOfFMVTextures].StaticImageDrawn=0;
						SetupFMVTexture(&FMVTexture[NumberOfFMVTextures]);
						NumberOfFMVTextures++;
					}
				}
			}		
		}
	}
}

void ReleaseAllFMVTextures()
{
	for(int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;
		ReleaseD3DTexture8(FMVTexture[i].SrcTexture);
		ReleaseD3DTexture8(FMVTexture[i].SrcSurface);
		ReleaseD3DTexture8(FMVTexture[i].DestTexture);
	}
}

void ReleaseAllFMVTexturesForDeviceReset()
{
	for(int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;
		ReleaseD3DTexture8(FMVTexture[i].SrcTexture);
		ReleaseD3DTexture8(FMVTexture[i].SrcSurface);
		ReleaseD3DTexture8(FMVTexture[i].DestTexture);
//		ReleaseD3DTexture8(FMVTexture[i].ImagePtr->Direct3DTexture);
		if(FMVTexture[i].ImagePtr->Direct3DTexture != NULL)
		{
			FMVTexture[i].ImagePtr->Direct3DTexture->Release();
			FMVTexture[i].ImagePtr->Direct3DTexture = NULL;
		}
	}
}

void PlayMenuMusic()
{
}

extern void InitialiseTriggeredFMVs()
{
	int i = NumberOfFMVTextures;
	while(i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			if(FMVTexture[i].SmackHandle)
			{
				FMVTexture[i].MessageNumber = 0;
			}

			FMVTexture[i].SmackHandle = 0;
		}
	}
}

void EndMenuMusic()
{
}

extern void GetFMVInformation(int *messageNumberPtr, int *frameNumberPtr)
{
	int i = NumberOfFMVTextures;

	while(i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			if(FMVTexture[i].SmackHandle)
			{
				*messageNumberPtr = FMVTexture[i].MessageNumber;
				*frameNumberPtr = 0;
				return;
			}
		}
	}

	*messageNumberPtr = 0;
	*frameNumberPtr = 0;
}

void CloseFMV()
{
}

/* call this for res change, alt tabbing and whatnot */
extern void ReleaseBinkTextures()
{
	//ReleaseD3DTexture8(&binkTexture);
}

void FindLightingValuesFromTriggeredFMV(unsigned char *bufferPtr, FMVTEXTURE *ftPtr)
{
	unsigned int totalRed=0;
	unsigned int totalBlue=0;
	unsigned int totalGreen=0;

	FmvColourRed = totalRed/48*16;
	FmvColourGreen = totalGreen/48*16;
	FmvColourBlue = totalBlue/48*16;
}

int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr)
{
	int smackerFormat = 1;
	int w = 128;
	
	{
		extern D3DINFO d3d;
//		smackerFormat = GetSmackerPixelFormat(&(d3d.TextureFormat[d3d.CurrentTextureFormat].ddsd.ddpfPixelFormat));
	}
	if (smackerFormat) w*=2;

	if (!ftPtr->StaticImageDrawn || smackerFormat)
	{
		int i = w*96/4;
		unsigned int seed = FastRandom();
		int *ptr = (int*)bufferPtr;
		do
		{
			seed = ((seed*1664525)+1013904223);
			*ptr++ = seed;
		}
		while(--i);
		ftPtr->StaticImageDrawn=1;
	}
	FindLightingValuesFromTriggeredFMV((unsigned char*)bufferPtr,ftPtr);
	return 1;
}

} // extern "C"

//#endif