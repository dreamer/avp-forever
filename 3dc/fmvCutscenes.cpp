
#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include "fmvPlayback.h"
#include "fmvCutscenes.h"
#include "d3_func.h"

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
#define restrict
#include <emmintrin.h>

VorbisCodec *menuMusic = NULL;
bool MenuBackground = false;

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

#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];
int NumberOfFMVTextures = 0;

int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;

/* structure holds pointers to y, u, v channels */
typedef struct _OggPlayYUVChannels {
    unsigned char * ptry;
    unsigned char * ptru;
    unsigned char * ptrv;
    int             y_width;
    int             y_height;
	int				y_pitch;
    int             uv_width;
    int             uv_height;
	int				uv_pitch;
} OggPlayYUVChannels;

/* structure holds pointers to y, u, v channels */
typedef struct _OggPlayRGBChannels {
    unsigned char * ptro;
    int             rgb_width;
    int             rgb_height;
	int				rgb_pitch;
} OggPlayRGBChannels;

extern unsigned char DebouncedGotAnyKey;

int NextFMVFrame();
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);

void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb);

extern void StartMenuBackgroundFmv()
{
	return;

	const char *filenamePtr = "fmvs\\menubackground.ogv";

//	OpenTheoraVideo(filenamePtr, PLAYLOOP);

	MenuBackground = true;
}

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

extern void PlayFMV(const char *filenamePtr)
{
	if (!IntroOutroMoviesAreActive)
		return;

	TheoraFMV *newFmv = new TheoraFMV();
	newFmv->Open(filenamePtr);
//	newFmv->Open(/*file*/"d:\\Fmvs\\MarineIntro.ogv");

//	if (OpenTheoraVideo(filenamePtr) != 0)
//		return;

	int playing = 1;

	while (playing)
	{
		CheckForWindowsMessages();

		if (!CheckTheoraPlayback())
			playing = 0;

		if (frameReady)
			playing = NextFMVFrame();

		ThisFramesRenderingHasBegun();
		ClearScreenToBlack();

		if (mDisplayTexture)
		{
			DrawFmvFrame(frameWidth, frameHeight, textureWidth, textureHeight, mDisplayTexture);
		}

		ThisFramesRenderingHasFinished();
		FlipBuffers();

		DirectReadKeyboard();

		// added DebouncedGotAnyKey to ensure previous frames key press for starting level doesn't count
		if (GotAnyKey && DebouncedGotAnyKey)
		{
			playing = 0;
		}
	}

	FmvClose();
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

int NextFMVFrame()
{
	return 1;
#if 0 // temporarily disabled
	if (fmvPlaying == false)
		return 0;

	EnterCriticalSection(&frameCriticalSection);

	D3DLOCKED_RECT textureLock;
	if (FAILED(mDisplayTexture->LockRect(0, &textureLock, NULL, D3DLOCK_DISCARD)))
	{
		OutputDebugString("can't lock FMV texture\n");
		return 0;
	}

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
	oggRgb.ptro = static_cast<uint8_t*>(textureLock.pBits);
	oggRgb.rgb_height = frameHeight;
	oggRgb.rgb_width = frameWidth;
	oggRgb.rgb_pitch = textureLock.Pitch;

	oggplay_yuv2rgb(&oggYuv, &oggRgb);

	if (FAILED(mDisplayTexture->UnlockRect(0)))
	{
		OutputDebugString("can't unlock FMV texture\n");
		return 0;
	}

	frameReady = false;

	LeaveCriticalSection(&frameCriticalSection);

	return 1;
#endif
}

/* Vanilla implementation if YUV->RGB conversion */
void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb)
{
#ifdef VANILLA
	unsigned char * ptry = yuv->ptry;
	unsigned char * ptru = yuv->ptru;
	unsigned char * ptrv = yuv->ptrv;
	unsigned char * ptro = rgb->ptro;
	unsigned char * ptro2;
	int i, j;

	for (i = 0; i < yuv->y_height; i++)
	{
		ptro2 = ptro;
		for (j = 0; j < yuv->y_width; j += 2)
		{
			short pr, pg, pb, y;
			short r, g, b;

			pr = (-56992 + ptrv[j/2] * 409) >> 8;
			pg = (34784 - ptru[j/2] * 100 - ptrv[j/2] * 208) >> 8;
			pb = (-70688 + ptru[j/2] * 516) >> 8;

			y = 298*ptry[j] >> 8;
			r = y + pr;
			g = y + pg;
			b = y + pb;

			*ptro2++ = CLAMP(r);
			*ptro2++ = CLAMP(g);
			*ptro2++ = CLAMP(b);
			*ptro2++ = 255;

			y = 298*ptry[j + 1] >> 8;
			r = y + pr;
			g = y + pg;
			b = y + pb;

			*ptro2++ = CLAMP(r);
			*ptro2++ = CLAMP(g);
			*ptro2++ = CLAMP(b);
			*ptro2++ = 255;
		}

		ptry += yuv->y_pitch;

		if (i & 1)
		{
			ptru += yuv->uv_pitch;
			ptrv += yuv->uv_pitch;
		}

		ptro += rgb->rgb_pitch;
	}
#else // mmx
	int               i;
	unsigned char   * restrict ptry;
	unsigned char   * restrict ptru;
	unsigned char   * restrict ptrv;
	unsigned char   * ptro;

	register __m64    *y, *o;
	register __m64    zero, ut, vt, imm, imm2;
	register __m64    r, g, b;
	register __m64    tmp, tmp2;

	zero = _mm_setzero_si64();

	ptro = rgb->ptro;
	ptry = yuv->ptry;
	ptru = yuv->ptru;
	ptrv = yuv->ptrv;

	for (i = 0; i < yuv->y_height; i++)
	{
		int j;
		o = (__m64*)ptro;
		ptro += rgb->rgb_pitch;

		for (j = 0; j < yuv->y_width; j += 8)
		{
			y = (__m64*)&ptry[j];

			ut = _m_from_int(*(int *)(ptru + j/2));
			vt = _m_from_int(*(int *)(ptrv + j/2));

			ut = _m_punpcklbw(ut, zero);
			vt = _m_punpcklbw(vt, zero);

			/* subtract 128 from u and v */
			imm = _mm_set1_pi16(128);
			ut = _m_psubw(ut, imm);
			vt = _m_psubw(vt, imm);

			/* transfer and multiply into r, g, b registers */
			imm = _mm_set1_pi16(-51);
			g = _m_pmullw(ut, imm);
			imm = _mm_set1_pi16(130);
			b = _m_pmullw(ut, imm);
			imm = _mm_set1_pi16(146);
			r = _m_pmullw(vt, imm);
			imm = _mm_set1_pi16(-74);
			imm = _m_pmullw(vt, imm);
			g = _m_paddsw(g, imm);

			/* add 64 to r, g and b registers */
			imm = _mm_set1_pi16(64);
			r = _m_paddsw(r, imm);
			g = _m_paddsw(g, imm);
			imm = _mm_set1_pi16(32);
			b = _m_paddsw(b, imm);

			/* shift r, g and b registers to the right */
			r = _m_psrawi(r, 7);
			g = _m_psrawi(g, 7);
			b = _m_psrawi(b, 6);

			/* subtract 16 from r, g and b registers */
			imm = _mm_set1_pi16(16);
			r = _m_psubsw(r, imm);
			g = _m_psubsw(g, imm);
			b = _m_psubsw(b, imm);

			y = (__m64*)&ptry[j];

			/* duplicate u and v channels and add y
			 * each of r,g, b in the form [s1(16), s2(16), s3(16), s4(16)]
			 * first interleave, so tmp is [s1(16), s1(16), s2(16), s2(16)]
			 * then add y, then interleave again
			 * then pack with saturation, to get the desired output of
			 *   [s1(8), s1(8), s2(8), s2(8), s3(8), s3(8), s4(8), s4(8)]
			 */
			tmp = _m_punpckhwd(r, r);
			imm = _m_punpckhbw(*y, zero);
			//printf("tmp: %llx imm: %llx\n", tmp, imm);
			tmp = _m_paddsw(tmp, imm);
			tmp2 = _m_punpcklwd(r, r);
			imm2 = _m_punpcklbw(*y, zero);
			tmp2 = _m_paddsw(tmp2, imm2);
			r = _m_packuswb(tmp2, tmp);

			tmp = _m_punpckhwd(g, g);
			tmp2 = _m_punpcklwd(g, g);
			tmp = _m_paddsw(tmp, imm);
			tmp2 = _m_paddsw(tmp2, imm2);
			g = _m_packuswb(tmp2, tmp);

			tmp = _m_punpckhwd(b, b);
			tmp2 = _m_punpcklwd(b, b);
			tmp = _m_paddsw(tmp, imm);
			tmp2 = _m_paddsw(tmp2, imm2);
			b = _m_packuswb(tmp2, tmp);
			//printf("duplicated r g and b: %llx %llx %llx\n", r, g, b);

			/* now we have 8 8-bit r, g and b samples.  we want these to be packed
			 * into 32-bit values.
			 */
			//r = _m_from_int(0);
			//b = _m_from_int(0);
			imm = _mm_set1_pi32(0xFFFFFFFF);
			tmp = _m_punpcklbw(r, b);
			tmp2 = _m_punpcklbw(g, imm);
			*o++ = _m_punpcklbw(tmp, tmp2);
			*o++ = _m_punpckhbw(tmp, tmp2);
			//printf("tmp, tmp2, write1, write2: %llx %llx %llx %llx\n", tmp, tmp2,
			//                _m_punpcklbw(tmp, tmp2), _m_punpckhbw(tmp, tmp2));
			tmp = _m_punpckhbw(r, b);
			tmp2 = _m_punpckhbw(g, imm);
			*o++ = _m_punpcklbw(tmp, tmp2);
			*o++ = _m_punpckhbw(tmp, tmp2);
		}
		if (i & 0x1)
		{
			ptru += yuv->uv_pitch;
			ptrv += yuv->uv_pitch;
		}
		ptry += yuv->y_pitch;
	}
	_m_empty();
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

void ReleaseAllFMVTexturesForDeviceReset()
{
	for (int i = 0; i < NumberOfFMVTextures; i++)
	{
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}
#if 0 // temporarily disabled
	// non ingame fmv?
	if (mDisplayTexture)
	{
		mDisplayTexture->Release();
		mDisplayTexture = NULL;
	}
#endif
}

void RecreateAllFMVTexturesAfterDeviceReset()
{
	for (int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].ImagePtr->Direct3DTexture = CreateFmvTexture(&FMVTexture[i].ImagePtr->ImageWidth, &FMVTexture[i].ImagePtr->ImageHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
	}
#if 0 // temporarily disabled
	// non ingame fmv? - use a better way to determine this..
	if ((textureWidth && textureHeight) && (!mDisplayTexture))
	{
		mDisplayTexture = CreateFmvTexture(&textureWidth, &textureHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
	}
#endif
}

// bjd - the below three functions could maybe be moved out of this file altogether as vorbisPlayer can handle it
void StartMenuMusic()
{
	// we need to load IntroSound.ogg here using vorbisPlayer
	menuMusic = Vorbis_LoadFile("IntroSound.ogg");
}

void PlayMenuMusic()
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

void FindLightingValuesFromTriggeredFMV(uint8_t *bufferPtr, FMVTEXTURE *ftPtr)
{
	unsigned int totalRed=0;
	unsigned int totalBlue=0;
	unsigned int totalGreen=0;

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

// this code contains code from the liboggplay library. the yuv->rgb routines are taken from their source code. Below is the license that is
// included with liboggplay

/*
   Copyright (C) 2003 CSIRO Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the CSIRO nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/