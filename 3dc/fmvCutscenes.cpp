
#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include "fmvPlayback.h"
#include "fmvCutscenes.h"
#include "console.h"

#define MAX_FMVS 4

struct fmvCutscene
{
	BOOL isPlaying;
	TheoraFMV *fmvClass;
};

fmvCutscene fmvList[MAX_FMVS];


#include "logString.h"
#include "vorbisPlayer.h"
#include "console.h"

#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];
uint32_t NumberOfFMVTextures = 0;

extern void UpdateFMVTexture(FMVTEXTURE *ftPtr);

extern "C" {
int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;
int OpenFMV(const char *filenamePtr);
int GetVolumeOfNearestVideoScreen(void);

int FmvSoundVolume = 65536 / 512;
int MoviesAreActive = 1;
int IntroOutroMoviesAreActive = 1;
int VolumeOfNearestVideoScreen = 0;
int PanningOfNearestVideoScreen = 0;
#include "inline.h"
}

VorbisPlayback *menuMusic = NULL;
bool MenuBackground = false;

void ReleaseAllFMVTexturesForDeviceReset()
{
	for (uint32_t i = 0; i < NumberOfFMVTextures; i++)
	{
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}

	// check for fullscreen intro/outro fmvs
	for (uint32_t i = 0; i < MAX_FMVS; i++)
	{
		if (fmvList[i].isPlaying && fmvList[i].fmvClass)
		{
			for (uint32_t j = 0; j < 3; j++)
			{
				if (fmvList[i].fmvClass->frameTextures[j].texture)
				{
					fmvList[i].fmvClass->frameTextures[j].texture->Release();
					fmvList[i].fmvClass->frameTextures[j].texture = NULL;
				}
			}
/*
			// lets double check..
			if (fmvList[i].fmvClass->mDisplayTexture)
			{
				fmvList[i].fmvClass->mDisplayTexture->Release();
				fmvList[i].fmvClass->mDisplayTexture = NULL;
			}
*/
		}
	}
}

void RecreateAllFMVTexturesAfterDeviceReset()
{
	for (uint32_t i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].ImagePtr->Direct3DTexture = CreateFmvTexture(&FMVTexture[i].ImagePtr->ImageWidth, &FMVTexture[i].ImagePtr->ImageHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
	}
/*
	// check for fullscreen intro/outro fmvs
	for (int i = 0; i < MAX_FMVS; i++)
	{
		if (fmvList[i].isPlaying && fmvList[i].fmvClass)
		{
			// lets double check..
			if (fmvList[i].fmvClass->mTextureWidth > 0)
			{
				fmvList[i].fmvClass->mDisplayTexture = CreateFmvTexture(&fmvList[i].fmvClass->mTextureWidth, &fmvList[i].fmvClass->mTextureHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
			}
		}
	}
*/
}

void FindLightingValuesFromTriggeredFMV(uint8_t *bufferPtr, FMVTEXTURE *ftPtr)
{
	unsigned int totalRed = 0;
	unsigned int totalBlue = 0;
	unsigned int totalGreen = 0;

	FmvColourRed = totalRed/48*16;
	FmvColourGreen = totalGreen/48*16;
	FmvColourBlue = totalBlue/48*16;
}

int NextFMVTextureFrame(FMVTEXTURE *ftPtr)
{
	uint32_t w = ftPtr->ImagePtr->ImageWidth;
	uint32_t h = ftPtr->ImagePtr->ImageHeight;

	uint8_t *DestBufferPtr = ftPtr->RGBBuffer;

	if (MoviesAreActive && ftPtr->fmvHandle != -1)
	{
		int volume = MUL_FIXED(FmvSoundVolume*256, GetVolumeOfNearestVideoScreen());

//fixme		AudioStream_SetBufferVolume(fmvList[ftPtr->fmvHandle].fmvClass->mAudioStream, volume);
//fixme		AudioStream_SetPan(fmvList[ftPtr->fmvHandle].fmvClass->mAudioStream, PanningOfNearestVideoScreen);

		ftPtr->SoundVolume = FmvSoundVolume;

		if (ftPtr->IsTriggeredPlotFMV && (!fmvList[ftPtr->fmvHandle].fmvClass->IsPlaying()))
		{
			OutputDebugString("closing ingame fmv..\n");

			ftPtr->MessageNumber = 0;
			delete fmvList[ftPtr->fmvHandle].fmvClass;
			fmvList[ftPtr->fmvHandle].fmvClass = NULL;
			fmvList[ftPtr->fmvHandle].isPlaying = FALSE;
			ftPtr->fmvHandle = -1;
		}
		else
		{
			if (!fmvList[ftPtr->fmvHandle].fmvClass->mFrameReady)
				return 0;

			fmvList[ftPtr->fmvHandle].fmvClass->NextFrame(w, h, DestBufferPtr, w * 4);
		}

		ftPtr->StaticImageDrawn = 0;
	}
	else if (!ftPtr->StaticImageDrawn || /*smackerFormat*/1)
	{
		uint32_t i = w * h;
		uint32_t seed = FastRandom();
		int *ptr = (int*)DestBufferPtr;
		do
		{
			seed = ((seed * 1664525) + 1013904223);
			*ptr++ = seed;
		}
		while (--i);
		ftPtr->StaticImageDrawn = 1;
	}
	FindLightingValuesFromTriggeredFMV((uint8_t*)ftPtr->RGBBuffer, ftPtr);
	return 1;
}

void StartMenuBackgroundFmv()
{
	return;

	const char *filenamePtr = "fmvs\\menubackground.ogv";

	MenuBackground = true;
}


extern "C" {

#include "avp_userprofile.h"
#include <math.h>
#include <assert.h>

extern unsigned char GotAnyKey;
extern int NumActiveBlocks;
extern DISPLAYBLOCK *ActiveBlockList[];
extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);
extern IMAGEHEADER ImageHeaderArray[];
#if MaxImageGroups>1
	extern int NumImagesArray[];
#else
	extern int NumImages;
#endif

extern unsigned char DebouncedGotAnyKey;

int NextFMVFrame();
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);

extern int PlayMenuBackgroundFmv()
{
	return 0;
#if 0 // temporarily disabled
	if (!MenuBackground)
		return 0;

	int playing = 0;

	if (frameReady)
	{
		playing = NextFMVFrame();
		DrawFmvFrame(frameWidth, frameHeight, textureWidth, textureHeight, mDisplayTexture);
	}

	return 1;
#endif
}

extern void EndMenuBackgroundFmv()
{
	return;
#if 0 // temporarily disabled
	if (!MenuBackground)
		return;

	FmvClose();

	MenuBackground = false;
#endif
}

void InitFmvCutscenes()
{
	memset(fmvList, 0, sizeof(fmvCutscene) * MAX_FMVS);
}

int32_t FindFreeFmvHandle()
{
	for (uint32_t i = 0; i < MAX_FMVS; i++)
	{
		if (!fmvList[i].isPlaying)
		{
			return i;
		}
	}
	return -1;
}

int OpenFMV(const char *filenamePtr)
{
	// find a free handle in our fmv list to use
	int32_t fmvHandle = FindFreeFmvHandle();

	if (fmvHandle != -1)
	{
		// found a free slot
		fmvList[fmvHandle].fmvClass = new TheoraFMV();
		if (fmvList[fmvHandle].fmvClass->Open(filenamePtr) != FMV_OK)
		{
			delete fmvList[fmvHandle].fmvClass;
			fmvList[fmvHandle].fmvClass = NULL;
			return -1;
		}
		fmvList[fmvHandle].isPlaying = TRUE;
	}
	else
	{
		Con_PrintError("No more free fmv slots");
		return -1;
	}
	
	return fmvHandle;
}

extern void PlayFMV(const char *filenamePtr)
{
	if (!IntroOutroMoviesAreActive)
		return;

	TheoraFMV fmv;
	if (fmv.Open(filenamePtr) != FMV_OK)
		return;

	bool playing = true;

	while (playing)
	{
		CheckForWindowsMessages();

		if (!fmv.IsPlaying())
			playing = false;

		if (fmv.mFrameReady)
			playing = fmv.NextFrame();

		ThisFramesRenderingHasBegun();

		if (fmv.mTexturesReady)
		{
			DrawFmvFrame2(fmv.mFrameWidth, fmv.mFrameHeight, fmv.frameTextures[0].width, fmv.frameTextures[0].height, fmv.frameTextures[0].texture, fmv.frameTextures[1].texture, fmv.frameTextures[2].texture);
		}

		ThisFramesRenderingHasFinished();
		FlipBuffers();

		DirectReadKeyboard();

		// added DebouncedGotAnyKey to ensure previous frames key press for starting level doesn't count
		if (GotAnyKey && DebouncedGotAnyKey)
		{
			fmv.Close();
			playing = false;
		}
	}
}

void FmvClose()
{
#if 0 // temporarily disabled
	fmvPlaying = false;
	CloseTheoraVideo();
#endif
}

void UpdateAllFMVTextures()
{
	extern void UpdateFMVTexture(FMVTEXTURE *ftPtr);
	uint32_t i = NumberOfFMVTextures;

	while (i--)
	{
		UpdateFMVTexture(&FMVTexture[i]);
	}
}

extern void StartTriggerPlotFMV(int number)
{
	uint32_t i = NumberOfFMVTextures;
	char buffer[25];

	if (CheatMode_Active != CHEATMODE_NONACTIVE)
		return;

	sprintf(buffer, "FMVs\\message%d.ogv", number);
	{
		FILE *file = avp_fopen(buffer, "rb");
		if (!file)
		{
			Con_PrintError("Couldn't open triggered plot fmv file");
			return;
		}
		fclose(file);
	}

	while (i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			// close it if it's open
			if (FMVTexture[i].fmvHandle)
			{
				if (fmvList[FMVTexture[i].fmvHandle].isPlaying)
					fmvList[FMVTexture[i].fmvHandle].fmvClass->Close();
					FMVTexture[i].fmvHandle = -1;
			}

			int32_t fmvHandle = OpenFMV(buffer);

			// couldn't open it
			if (fmvHandle == -1)
			{
				FMVTexture[i].fmvHandle = -1;
				return;
			}

			FMVTexture[i].fmvHandle = fmvHandle;
			FMVTexture[i].MessageNumber = number;
		}
	}
}

extern void StartFMVAtFrame(int number, int frame)
{
}

// called during each level load
void ScanImagesForFMVs()
{
	extern void SetupFMVTexture(FMVTEXTURE *ftPtr);
	int i;
	IMAGEHEADER *ihPtr;
	NumberOfFMVTextures = 0;

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
		if (NumImages)
		{
			ihPtr = &ImageHeaderArray[0];
			for (i = 0; i < NumImages; i++, ihPtr++)
			{
	#endif
				char *strPtr;
				if (strPtr = strstr(ihPtr->ImageName,"FMVs"))
				{
					char filename[30];
					{
						char *filenamePtr = filename;
						do
						{
							*filenamePtr++ = *strPtr;
						}
						while (*strPtr++!='.');

						*filenamePtr++='o';
						*filenamePtr++='g';
						*filenamePtr++='v';
						*filenamePtr=0;
					}

					// do a check here to see if it's a theora file rather than just any old file with the right name?
					FILE *file = avp_fopen(filename, "rb");
					if (file)
					{
						fclose(file);
						FMVTexture[NumberOfFMVTextures].IsTriggeredPlotFMV = 0;
					}
					else
					{
						FMVTexture[NumberOfFMVTextures].IsTriggeredPlotFMV = 1;
					}

					FMVTexture[NumberOfFMVTextures].ImagePtr = ihPtr;
					FMVTexture[NumberOfFMVTextures].fmvHandle = -1; // just to be sure
					FMVTexture[NumberOfFMVTextures].StaticImageDrawn = 0;
					SetupFMVTexture(&FMVTexture[NumberOfFMVTextures]);
					NumberOfFMVTextures++;
				}
			}
		}
	}
}

// called when player quits level and returns to main menu
void ReleaseAllFMVTextures()
{
	OutputDebugString("exiting level..closing FMV system\n");

	for (uint32_t i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;

		if (FMVTexture[i].fmvHandle != -1)
		{
			if (fmvList[FMVTexture[i].fmvHandle].isPlaying)
			{
//				fmvList[FMVTexture[i].fmvHandle].fmvClass->Close();
				delete fmvList[FMVTexture[i].fmvHandle].fmvClass;
				fmvList[FMVTexture[i].fmvHandle].fmvClass = NULL;
			}

			FMVTexture[i].fmvHandle = -1;
		}

		if (FMVTexture[i].RGBBuffer)
		{
			delete []FMVTexture[i].RGBBuffer;
			FMVTexture[i].RGBBuffer = NULL;
		}
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}
	NumberOfFMVTextures = 0;
}

// bjd - the below three functions could maybe be moved out of this file altogether as vorbisPlayer can handle it
void StartMenuMusic()
{
	// we need to load IntroSound.ogg here using vorbisPlayer
	menuMusic = new VorbisPlayback;
	if (!menuMusic->Open("FMVs\\IntroSound.ogg"))
	{
		Con_PrintError("Can't open file IntroSound.ogg");
		delete menuMusic;
		menuMusic = NULL;
	}
}

void PlayMenuMusic(void)
{
	// we can just leave this blank as a thread will handle the updates
}

void EndMenuMusic()
{
	if (menuMusic)
	{
		delete menuMusic;
		menuMusic = NULL;
	}
}

extern void InitialiseTriggeredFMVs()
{
	uint32_t i = NumberOfFMVTextures;
	while (i--)
	{
		if (FMVTexture[i].fmvHandle != -1)
		{
			delete fmvList[FMVTexture[i].fmvHandle].fmvClass;
			fmvList[FMVTexture[i].fmvHandle].fmvClass = NULL;
		}
		FMVTexture[i].MessageNumber = 0;
		FMVTexture[i].fmvHandle = -1;
	}
}

extern void GetFMVInformation(int *messageNumberPtr, int *frameNumberPtr)
{
//	uint32_t i = NumberOfFMVTextures;
/*
	while (i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			if (FMVTexture[i].SmackHandle)
			{
				*messageNumberPtr = FMVTexture[i].MessageNumber;
				*frameNumberPtr = 0;
				return;
			}
		}
	}
*/
	*messageNumberPtr = 0;
	*frameNumberPtr = 0;
}

int GetVolumeOfNearestVideoScreen(void)
{
	extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
	int numberOfObjects = NumActiveBlocks;
	int leastDistanceRecorded = 0x7fffffff;
	VolumeOfNearestVideoScreen = 0;

	{
		extern char LevelName[];
		if (!_stricmp(LevelName,"invasion_a"))
		{
			VolumeOfNearestVideoScreen = ONE_FIXED;
			PanningOfNearestVideoScreen = ONE_FIXED/2;
		}
	}

	while (numberOfObjects)
	{
		DISPLAYBLOCK* objectPtr = ActiveBlockList[--numberOfObjects];
		STRATEGYBLOCK* sbPtr = objectPtr->ObStrategyBlock;

		if (sbPtr)
		{
			if (sbPtr->I_SBtype == I_BehaviourVideoScreen)
			{
				int dist;
				VECTORCH disp;

				disp.vx = objectPtr->ObWorld.vx - Global_VDB_Ptr->VDB_World.vx;
				disp.vy = objectPtr->ObWorld.vy - Global_VDB_Ptr->VDB_World.vy;
				disp.vz = objectPtr->ObWorld.vz - Global_VDB_Ptr->VDB_World.vz;

				dist = Approximate3dMagnitude(&disp);
				if (dist < leastDistanceRecorded && dist < ONE_FIXED)
				{
					leastDistanceRecorded = dist;
					VolumeOfNearestVideoScreen = ONE_FIXED + 1024 - dist/2;
					if (VolumeOfNearestVideoScreen>ONE_FIXED) VolumeOfNearestVideoScreen = ONE_FIXED;
					{
						VECTORCH rightEarDirection;

						rightEarDirection.vx = Global_VDB_Ptr->VDB_Mat.mat11;
						rightEarDirection.vy = 0;
						rightEarDirection.vz = Global_VDB_Ptr->VDB_Mat.mat31;
						disp.vy = 0;
						Normalise(&disp);
						Normalise(&rightEarDirection);

						PanningOfNearestVideoScreen = 32768 + DotProduct(&disp,&rightEarDirection)/2;
					}
				}
			}
		}
	}
//	PrintDebuggingText("Volume: %d, Pan %d\n",VolumeOfNearestVideoScreen,PanningOfNearestVideoScreen);
	return VolumeOfNearestVideoScreen;
}

}; // extern C