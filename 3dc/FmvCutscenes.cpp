
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
#include "Di_func.h"
#include "inline.h"
#include "avp_userprofile.h"
#include <math.h>
#include <assert.h>

static const int kMaxFMVs = 4;

struct fmvCutscene
{
	bool isPlaying;
	TheoraFMV *FMVclass;
};

fmvCutscene fmvList[kMaxFMVs];

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

VorbisPlayback	*menuMusic = NULL;
TheoraFMV		*menuFMV = NULL;
bool MenuBackground = false;

void FindLightingValuesFromTriggeredFMV(uint8_t *bufferPtr, FMVTEXTURE *ftPtr)
{
	uint32_t totalRed = 0;
	uint32_t totalBlue = 0;
	uint32_t totalGreen = 0;

	FmvColourRed = totalRed/48*16;
	FmvColourGreen = totalGreen/48*16;
	FmvColourBlue = totalBlue/48*16;
}

int NextFMVTextureFrame(FMVTEXTURE *ftPtr)
{
	// i think these are always 128x128 for ingame textures? fix this later I guess..
	uint32_t width = 0;//128;
	uint32_t height = 0;//128;
	Tex_GetDimensions(ftPtr->textureID, width, height);

	uint8_t *DestBufferPtr = ftPtr->RGBBuffer;

	if (MoviesAreActive && ftPtr->fmvHandle != -1)
	{
		int volume = MUL_FIXED(FmvSoundVolume*256, GetVolumeOfNearestVideoScreen());

		// TODO: Volume and Panning
//fixme		AudioStream_SetBufferVolume(fmvList[ftPtr->fmvHandle].fmvClass->mAudioStream, volume);
//fixme		AudioStream_SetPan(fmvList[ftPtr->fmvHandle].fmvClass->mAudioStream, PanningOfNearestVideoScreen);

		ftPtr->SoundVolume = FmvSoundVolume;

		if (ftPtr->IsTriggeredPlotFMV && (!fmvList[ftPtr->fmvHandle].FMVclass->IsPlaying()))
		{
			ftPtr->MessageNumber = 0;
			delete fmvList[ftPtr->fmvHandle].FMVclass;
			fmvList[ftPtr->fmvHandle].FMVclass = NULL;
			fmvList[ftPtr->fmvHandle].isPlaying = false;
			ftPtr->fmvHandle = -1;
		}
		else
		{
			if (!fmvList[ftPtr->fmvHandle].FMVclass->mFrameReady)
				return 0;

			fmvList[ftPtr->fmvHandle].FMVclass->NextFrame(width, height, DestBufferPtr, width * sizeof(uint32_t));
		}

		ftPtr->StaticImageDrawn = false;
	}
	else// if (!ftPtr->StaticImageDrawn || /*smackerFormat*/1)
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
	return;

	const char *filenamePtr = "fmvs/menubackground.ogv";

	menuFMV = new TheoraFMV();

	// start playback threads
	if (menuFMV->Open(filenamePtr) != FMV_OK)
		return;
	
	MenuBackground = true;
}

// called per frame to display a frame of the menu background FMV
extern int PlayMenuBackgroundFmv()
{
	if (!MenuBackground)
		return 0;

	bool playing = false;

	if (menuFMV->mFrameReady)
	{
		playing = menuFMV->NextFrame();
	}

	if (menuFMV->mTexturesReady)
	{
		DrawFmvFrame2(menuFMV->mFrameWidth, menuFMV->mFrameHeight, menuFMV->frameTextureIDs);
	}

	return 1;
}

extern void EndMenuBackgroundFmv()
{
	if (!MenuBackground)
		return;

	menuFMV->Close();
	delete menuFMV;
	menuFMV = 0;

	MenuBackground = false;
}

void InitFmvCutscenes()
{
	memset(fmvList, 0, sizeof(fmvCutscene) * kMaxFMVs);
}

int32_t FindFreeFmvHandle()
{
	for (uint32_t i = 0; i < kMaxFMVs; i++)
	{
		if (!fmvList[i].isPlaying)
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
		fmvList[fmvHandle].FMVclass = new TheoraFMV();
		if (fmvList[fmvHandle].FMVclass->Open(filenamePtr) != FMV_OK)
		{
			delete fmvList[fmvHandle].FMVclass;
			fmvList[fmvHandle].FMVclass = NULL;
			return -1;
		}
		fmvList[fmvHandle].isPlaying = true;
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

		ThisFramesRenderingHasBegun();

		if (!fmv.IsPlaying())
			playing = false;

		if (fmv.mFrameReady)
			playing = fmv.NextFrame();

		if (fmv.mTexturesReady)
		{
			DrawFmvFrame2(fmv.mFrameWidth, fmv.mFrameHeight, fmv.frameTextureIDs);
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

	sprintf(buffer, "FMVs/message%d.ogv", number);
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
				{
					fmvList[FMVTexture[i].fmvHandle].FMVclass->Close();
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

	for (uint32_t i = 0; i < fmvTextures.size(); i++)
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

			// generate a new string, from occurrence of "FMVs" in string to before the fullstop, then append ".ogv" extension
			std::string fileName = fmvTextures[i].substr(offset1, offset2-offset1) + ".ogv";
			
			// do a check here to see if it's a theora file rather than just any old file with the right name?
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

			uint32_t width = 0;
			uint32_t height = 0;

			Tex_GetDimensions(i, width, height); // TODO: check that i is always going to be correct as texture id

			assert((width == 128) && (height == 128));

			FMVTexture[NumberOfFMVTextures].textureID = i;
			FMVTexture[NumberOfFMVTextures].width = width;
			FMVTexture[NumberOfFMVTextures].height = height;
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
			if (fmvList[FMVTexture[i].fmvHandle].isPlaying)
			{
				delete fmvList[FMVTexture[i].fmvHandle].FMVclass;
				fmvList[FMVTexture[i].fmvHandle].FMVclass = NULL;
			}

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
	// we need to load IntroSound.ogg here using vorbisPlayer
	menuMusic = new VorbisPlayback;
	if (!menuMusic->Open("FMVs/IntroSound.ogg"))
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
			delete fmvList[FMVTexture[i].fmvHandle].FMVclass;
			fmvList[FMVTexture[i].fmvHandle].FMVclass = NULL;
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
