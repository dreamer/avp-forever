
#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include "FmvPlayback.h"
#include "FmvCutscenes.h"
#include "console.h"
#include "TextureManager.h"
#include "logString.h"
#include "VorbisPlayer.h"
#include "console.h"
#include <vector>
#include <string>
#include "io.h"
#include "Input.h"
#include "inline.h"
#include "avp_userprofile.h"
#include <math.h>
#include <assert.h>

#include "TheoraPlayback.h"
#include "BinkPlayback.h"
#include "SmackerPlayback.h"

static const int kMaxFMVs = 4;
SmackerPlayback *fmvList[kMaxFMVs];

FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];
uint32_t NumberOfFMVTextures = 0;

extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
extern char LevelName[];
extern int NumActiveBlocks;
extern DISPLAYBLOCK *ActiveBlockList[];

extern void UpdateFMVTexture(FMVTEXTURE *ftPtr);
extern void SetupFMVTexture(FMVTEXTURE *ftPtr);
extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);

int NextFMVFrame();

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

//VorbisPlayback	*menuMusic = NULL;
SmackerPlayback		*menuMusic = NULL;
//TheoraPlayback	*menuFMV = NULL;
BinkPlayback	*menuFMV = NULL;
bool MenuBackgroundFMV = false;

extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);


void FindLightingValuesFromTriggeredFMV(uint8_t *bufferPtr, FMVTEXTURE *ftPtr)
{
	uint32_t totalRed   = 0;
	uint32_t totalBlue  = 0;
	uint32_t totalGreen = 0;

	FmvColourRed   = totalRed/48*16;
	FmvColourGreen = totalGreen/48*16;
	FmvColourBlue  = totalBlue/48*16;
}

int NextFMVTextureFrame(FMVTEXTURE *ftPtr)
{
	uint32_t width = 0;
	uint32_t height = 0;
	Tex_GetDimensions(ftPtr->textureID, width, height);

	uint8_t *DestBufferPtr = ftPtr->RGBBuffer;

	if (MoviesAreActive && ftPtr->fmvHandle != -1)
	{
		int volume = MUL_FIXED(FmvSoundVolume*256, GetVolumeOfNearestVideoScreen());

		// TODO: Volume and Panning
//fixme		AudioStream_SetBufferVolume(fmvList[ftPtr->fmvHandle].fmvClass->mAudioStream, volume);
//fixme		AudioStream_SetPan(fmvList[ftPtr->fmvHandle].fmvClass->mAudioStream, PanningOfNearestVideoScreen);

		ftPtr->SoundVolume = FmvSoundVolume;

		if (ftPtr->IsTriggeredPlotFMV && (!fmvList[ftPtr->fmvHandle]->IsPlaying()))
		{
			ftPtr->MessageNumber = 0;
			delete fmvList[ftPtr->fmvHandle];
			fmvList[ftPtr->fmvHandle] = NULL;
			ftPtr->fmvHandle = -1;
		}
		else
		{
			if (!fmvList[ftPtr->fmvHandle]->mFrameReady)
				return 0;

			fmvList[ftPtr->fmvHandle]->ConvertFrame(width, height, DestBufferPtr, width * sizeof(uint32_t));
		}

		ftPtr->StaticImageDrawn = false;
	}
	else
	{
		uint32_t i = width * height;
		uint32_t seed = FastRandom();
		int *ptr = (int*)DestBufferPtr;
		do
		{
			seed = ((seed * 1664525) + 1013904223);
			*ptr++ = seed;
		}
		while (--i);
		ftPtr->StaticImageDrawn = true;
	}
	FindLightingValuesFromTriggeredFMV(ftPtr->RGBBuffer, ftPtr);
	return 1;
}

// opens the menu background FMV
void StartMenuBackgroundFmv()
{
	MenuBackgroundFMV = false;

	menuFMV = new BinkPlayback();

	// start playback threads
	if (menuFMV->Open("fmvs/menubackground.bik") != FMV_OK)
	{
		delete menuFMV;
		menuFMV = NULL;
		return;
	}

	MenuBackgroundFMV = true;
}

// called per frame to display a frame of the menu background FMV
extern int PlayMenuBackgroundFmv()
{
	if (!MenuBackgroundFMV)
		return 0;

	if (menuFMV->mFrameReady)
	{
		menuFMV->ConvertFrame();
	}

	if (menuFMV->mTexturesReady)
	{
		DrawFmvFrame(menuFMV->mFrameWidth, menuFMV->mFrameHeight, menuFMV->frameTextureIDs);
	}

	return 1;
}

extern void EndMenuBackgroundFmv()
{
	if (!MenuBackgroundFMV)
		return;

	menuFMV->Close();
	delete menuFMV;
	menuFMV = 0;

	MenuBackgroundFMV = false;
}

void InitFmvCutscenes()
{
	for (uint32_t i = 0; i < kMaxFMVs; i++)
	{
		// null the pointer
		fmvList[i] = 0;
	}
}

int32_t FindFreeFmvHandle()
{
	for (uint32_t i = 0; i < kMaxFMVs; i++)
	{
		// find a slot with a NULL (free) pointer
		if (!fmvList[i])
		{
			return i;
		}
	}
	return -1;
}

int32_t OpenFMV(const char *filenamePtr)
{
	// find a free handle in our fmv list to use
	int32_t fmvHandle = FindFreeFmvHandle();

	if (fmvHandle != -1)
	{
		// found a free slot
		fmvList[fmvHandle] = new SmackerPlayback();
		if (fmvList[fmvHandle]->Open(filenamePtr) != FMV_OK)
		{
			delete fmvList[fmvHandle];
			fmvList[fmvHandle] = NULL;
			return -1;
		}
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

	BinkPlayback fmv;

	if (fmv.Open(filenamePtr) != FMV_OK)
		return;

	bool playing = true;

	while (playing)
	{
		CheckForWindowsMessages();

		ThisFramesRenderingHasBegun();

		if (!fmv.IsPlaying())
			playing = false;

		if (fmv.mFrameReady)
			playing = fmv.ConvertFrame();

		if (fmv.mTexturesReady)
		{
			DrawFmvFrame(fmv.mFrameWidth, fmv.mFrameHeight, fmv.frameTextureIDs);
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

void UpdateAllFMVTextures()
{
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

	sprintf(buffer, "FMVs/message%d.smk", number);

	FILE *file = avp_fopen(buffer, "rb");
	if (!file)
	{
		Con_PrintError("Couldn't open triggered plot fmv file");
		return;
	}
	fclose(file);

	while (i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			// close it if it's open
			if (FMVTexture[i].fmvHandle != -1)
			{
				if (fmvList[FMVTexture[i].fmvHandle]->IsPlaying())
				{
					delete fmvList[FMVTexture[i].fmvHandle];
					fmvList[FMVTexture[i].fmvHandle] = 0;
//					fmvList[FMVTexture[i].fmvHandle]->Close();
					FMVTexture[i].fmvHandle = -1;
				}
			}

			FMVTexture[i].fmvHandle = OpenFMV(buffer);
			if (FMVTexture[i].fmvHandle == -1)
			{
				// couldn't open it
				return;
			}

			FMVTexture[i].MessageNumber = number;
		}
	}
}

extern void StartFMVAtFrame(int number, int frame)
{
	// TODO..
}

// called during each level load
void ScanImagesForFMVs()
{
	NumberOfFMVTextures = 0;

	std::vector<std::string> fmvTextures;

	// fill fmvTextures with all texture names
	Tex_GetNamesVector(fmvTextures);

	for (size_t i = 0; i < fmvTextures.size(); i++)
	{
		// find occurrence of "FMVs" in string and store position of occurrence
		std::string::size_type offset1 = fmvTextures[i].find("FMVs");
		if (offset1 == std::string::npos)
		{
			// "FMVs" not found in string, continue
			continue;
		}
		else
		{
			// we found an occurrence. Now find offset of fullstop to allow us to remove the .RIM extension
			std::string::size_type offset2 = fmvTextures[i].find(".");

			// generate a new string, from occurrence of "FMVs" in string to before the fullstop, then append ".smk" extension
			std::string fileName = fmvTextures[i].substr(offset1, offset2-offset1) + ".smk";
			
			// do a check here to see if it's a valid video file rather than just any old file with the right name?
			FILE *file = avp_fopen(fileName.c_str(), "rb");
			if (file)
			{
				fclose(file);
				FMVTexture[NumberOfFMVTextures].IsTriggeredPlotFMV = 0;
			}
			else
			{
				FMVTexture[NumberOfFMVTextures].IsTriggeredPlotFMV = 1;
			}

			uint32_t width  = 0;
			uint32_t height = 0;

			Tex_GetDimensions(i, width, height); // TODO: check that i is always going to be correct as texture id

			assert((width == 128) && (height == 128));

			FMVTexture[NumberOfFMVTextures].textureID = i;
			FMVTexture[NumberOfFMVTextures].width     = width;
			FMVTexture[NumberOfFMVTextures].height    = height;
			FMVTexture[NumberOfFMVTextures].fmvHandle = -1; // just to be sure
			FMVTexture[NumberOfFMVTextures].StaticImageDrawn = false;
			SetupFMVTexture(&FMVTexture[NumberOfFMVTextures]);
			NumberOfFMVTextures++;
		}
	}
}

// called when player quits level and returns to main menu
void ReleaseAllFMVTextures()
{
	for (uint32_t i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;

		if (FMVTexture[i].fmvHandle != -1)
		{
			// close and delete the FMV object
			delete fmvList[FMVTexture[i].fmvHandle];
			fmvList[FMVTexture[i].fmvHandle] = NULL;
/*
			if (fmvList[FMVTexture[i].fmvHandle]->IsPlaying())
			{
				delete fmvList[FMVTexture[i].fmvHandle];
				fmvList[FMVTexture[i].fmvHandle] = NULL;
			}
*/
			FMVTexture[i].fmvHandle = -1;
		}

		delete[] FMVTexture[i].RGBBuffer;
		FMVTexture[i].RGBBuffer = NULL;

		Tex_Release(FMVTexture[i].textureID);
	}
	NumberOfFMVTextures = 0;
}

// bjd - the below three functions could maybe be moved out of this file altogether as vorbisPlayer can handle it
void StartMenuMusic()
{
	menuMusic = new SmackerPlayback;
	if (menuMusic->Open("FMVs/IntroSound.smk") != FMV_OK)
	{
		Con_PrintError("Can't open file IntroSound.smk");
		delete menuMusic;
		menuMusic = NULL;
	}
#if 0
	// we need to load IntroSound.ogg here using vorbisPlayer
	menuMusic = new VorbisPlayback;
	if (!menuMusic->Open("FMVs/IntroSound.ogg"))
	{
		Con_PrintError("Can't open file IntroSound.ogg");
		delete menuMusic;
		menuMusic = NULL;
	}
#endif
}

void PlayMenuMusic(void)
{
	// we can just leave this blank as a thread will handle the updates
}

void EndMenuMusic()
{
	delete menuMusic;
	menuMusic = NULL;
}

extern void InitialiseTriggeredFMVs()
{
	uint32_t i = NumberOfFMVTextures;
	while (i--)
	{
		if (FMVTexture[i].fmvHandle != -1)
		{
			delete fmvList[FMVTexture[i].fmvHandle];
			fmvList[FMVTexture[i].fmvHandle] = NULL;
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

int GetVolumeOfNearestVideoScreen()
{
	int numberOfObjects = NumActiveBlocks;
	int leastDistanceRecorded = 0x7fffffff;
	VolumeOfNearestVideoScreen = 0;
	
	if (!_stricmp(LevelName,"invasion_a"))
	{
		VolumeOfNearestVideoScreen = ONE_FIXED;
		PanningOfNearestVideoScreen = ONE_FIXED/2;
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
					if (VolumeOfNearestVideoScreen > ONE_FIXED) VolumeOfNearestVideoScreen = ONE_FIXED;
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
