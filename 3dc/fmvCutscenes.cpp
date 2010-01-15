
#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include "fmvPlayback.h"
#include "fmvCutscenes.h"
#include "d3_func.h"
#include "console.h"

#define MAX_FMVS 4

struct fmvCutscene
{
	int isPlaying;
	TheoraFMV *fmvClass;
};

fmvCutscene fmvList[MAX_FMVS];

#ifdef _XBOX
#include <d3dx8.h>
#include <xtl.h>
#define D3DLOCK_DISCARD 0
#else
#include <d3dx9.h>
#endif

#include "logString.h"
#include "vorbisPlayer.h"
#include "console.h"

#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];
int NumberOfFMVTextures = 0;

extern void UpdateFMVTexture(FMVTEXTURE *ftPtr);

extern "C" {
int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;
}

VorbisCodec *menuMusic = NULL;
bool MenuBackground = false;

void ReleaseAllFMVTexturesForDeviceReset()
{
	for (int i = 0; i < NumberOfFMVTextures; i++)
	{
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}

	// check for fullscreen intro/outro fmvs
	for (int i = 0; i < MAX_FMVS; i++)
	{
		if (fmvList[i].isPlaying && fmvList[i].fmvClass)
		{
			// lets double check..
			if (fmvList[i].fmvClass->mDisplayTexture)
			{
				fmvList[i].fmvClass->mDisplayTexture->Release();
				fmvList[i].fmvClass->mDisplayTexture = NULL;
			}
		}
	}
}

void RecreateAllFMVTexturesAfterDeviceReset()
{
	for (int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].ImagePtr->Direct3DTexture = CreateFmvTexture(&FMVTexture[i].ImagePtr->ImageWidth, &FMVTexture[i].ImagePtr->ImageHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
	}

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
	int w = ftPtr->ImagePtr->ImageWidth;
	int h = ftPtr->ImagePtr->ImageHeight;

	byte *DestBufferPtr = ftPtr->RGBBuffer;

#if 0 // temporarily disabled
	if (MoviesAreActive && ftPtr->SmackHandle)
	{
		int volume = MUL_FIXED(FmvSoundVolume*256, GetVolumeOfNearestVideoScreen());

//		AudioStream_SetBufferVolume(fmvAudioStream, volume);
//		AudioStream_SetPan(fmvAudioStream, PanningOfNearestVideoScreen);

		ftPtr->SoundVolume = FmvSoundVolume;

		if (!frameReady)
			return 0;

		if (ftPtr->IsTriggeredPlotFMV && (!CheckTheoraPlayback()))
		{
			OutputDebugString("closing ingame fmv..\n");

			ftPtr->SmackHandle = 0;
			ftPtr->MessageNumber = 0;
			FmvClose();
		}
		else
		{
			NextFMVFrame2(DestBufferPtr, w * 4);
		}

		ftPtr->StaticImageDrawn = 0;
	}
	else
#endif
	if (!ftPtr->StaticImageDrawn || /*smackerFormat*/1)
	{
		int i = w * h;
		unsigned int seed = FastRandom();
		int *ptr = (int*)DestBufferPtr;
		do
		{
			seed = ((seed * 1664525) + 1013904223);
			*ptr++ = seed;
		}
		while(--i);
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
#include "inline.h"
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

int FmvSoundVolume = 65536/512;
int MoviesAreActive = 1;
int IntroOutroMoviesAreActive = 1;
int VolumeOfNearestVideoScreen = 0;
int PanningOfNearestVideoScreen = 0;

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

int FindFreeFmvHandle()
{
	for (int i = 0; i < MAX_FMVS; i++)
	{
		if (!fmvList[i].isPlaying)
		{
			return i;
		}
	}
	return -1;
}

extern void PlayFMV(const char *filenamePtr)
{
	if (!IntroOutroMoviesAreActive)
		return;

	TheoraFMV *fmv = NULL;
	int fmvHandle = FindFreeFmvHandle();

	if (fmvHandle > -1)
	{
		// found a free slot
		fmvList[fmvHandle].fmvClass = new TheoraFMV();
		fmvList[fmvHandle].fmvClass->Open(filenamePtr);
		fmvList[fmvHandle].isPlaying = 1;
		fmv = fmvList[fmvHandle].fmvClass;
	}
	else
	{
		Con_PrintError("No more free fmv slots");
		return;
	}

	bool playing = true;

	while (playing)
	{
		CheckForWindowsMessages();

		if (!fmv->IsPlaying())
			playing = false;

		if (fmv->mFrameReady)
			playing = fmv->NextFrame();

		ThisFramesRenderingHasBegun();
		ClearScreenToBlack();

		if (fmv->mDisplayTexture)
		{
			DrawFmvFrame(fmv->mFrameWidth, fmv->mFrameHeight, fmv->mTextureWidth, fmv->mTextureHeight, fmv->mDisplayTexture);
		}

		ThisFramesRenderingHasFinished();
		FlipBuffers();

		DirectReadKeyboard();

		// added DebouncedGotAnyKey to ensure previous frames key press for starting level doesn't count
		if (GotAnyKey && DebouncedGotAnyKey)
		{
			playing = false;
		}
	}

	fmv->Close();
	delete fmv;
	fmv = NULL;
	fmvList[fmvHandle].isPlaying = 0;
}

int NextFMVFrame2(uint8_t *frameBuffer, int pitch)
{
	return 1;
#if 0 // temporarily disabled
	if (fmvPlaying == false)
		return 0;

	assert (frameBuffer);

	EnterCriticalSection(&frameCriticalSection);

	OggPlayYUVChannels oggYuv;
	oggYuv.ptry = buffer[0].data;
	oggYuv.ptru = buffer[2].data;
	oggYuv.ptrv = buffer[1].data;

	oggYuv.y_width = buffer[0].width;
	oggYuv.y_height = buffer[0].height;
	oggYuv.y_pitch = buffer[0].stride;

	oggYuv.uv_width = buffer[1].width;
	oggYuv.uv_height = buffer[1].height;
	oggYuv.uv_pitch = buffer[1].stride;

	OggPlayRGBChannels oggRgb;
	oggRgb.ptro = static_cast<uint8_t*>(frameBuffer);
	oggRgb.rgb_height = frameHeight;
	oggRgb.rgb_width = frameWidth;
	oggRgb.rgb_pitch = pitch;

	oggplay_yuv2rgb(&oggYuv, &oggRgb);

	frameReady = false;

	LeaveCriticalSection(&frameCriticalSection);

	return 1;
#endif
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
	int i = NumberOfFMVTextures;

	while(i--)
	{
		UpdateFMVTexture(&FMVTexture[i]);
	}
}

extern void StartTriggerPlotFMV(int number)
{
	OutputDebugString("StartTriggerPlotFMV\n");

	int i = NumberOfFMVTextures;
	char buffer[25];

	if (CheatMode_Active != CHEATMODE_NONACTIVE)
		return;

	sprintf(buffer, "FMVs//message%d.ogv", number);
	{
		FILE *file = avp_fopen(buffer, "rb");
		if (!file)
		{
			//OutputDebugString("couldn't open fmv file: ");
			//OutputDebugString(buffer);
			Con_PrintError("Couldn't open fmv file ");// + buffer);
			return;
		}
		fclose(file);
	}

	OutputDebugString("going to open ");
	OutputDebugString(buffer);
	OutputDebugString("\n");

	while(i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
#if 0 // temporarily disabled
			if (OpenTheoraVideo(buffer) < 0)
			{
				return;
			}
#endif
			FMVTexture[i].SmackHandle = 1;
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
			for (i = 0; i<NumImages; i++, ihPtr++)
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
						while(*strPtr++!='.');

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
	FmvClose();

	for (int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;
		FMVTexture[i].SmackHandle = 0;

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
	menuMusic = Vorbis_LoadFile("IntroSound.ogg");
}

void PlayMenuMusic(void)
{
	// we can just leave this blank as a thread will handle the updates
}

void EndMenuMusic()
{
	Vorbis_Release(menuMusic);
	menuMusic = NULL;
}

extern void InitialiseTriggeredFMVs()
{
	int i = NumberOfFMVTextures;
	while (i--)
	{
		FMVTexture[i].MessageNumber = 0;
		FMVTexture[i].SmackHandle = 0;
/*
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			if (FMVTexture[i].SmackHandle)
			{
				FMVTexture[i].MessageNumber = 0;
				FMVTexture[i].SmackHandle = 0;
			}
		}
*/
	}
}

extern void GetFMVInformation(int *messageNumberPtr, int *frameNumberPtr)
{
	int i = NumberOfFMVTextures;
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
				if (dist<leastDistanceRecorded && dist<ONE_FIXED)
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