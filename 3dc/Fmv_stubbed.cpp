//#undef USE_FMV
//#ifndef USE_FMV
//#if _XBOX
#if 1

extern "C" {

#include "d3_func.h"
#include "fmv.h"
#include "avp_userprofile.h"
#include "inline.h"
#include <math.h>

#include <assert.h>
#include "sndfile.h"

extern int GotAnyKey;

int SmackerSoundVolume = 65536/512;
int MoviesAreActive;
int IntroOutroMoviesAreActive = 1;
int VolumeOfNearestVideoScreen = 0;
int PanningOfNearestVideoScreen = 0;

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

extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);

void drive_decoding(void *arg);
void display_frame(void *arg);
//void handle_video_data (OggPlay * player, int track_num, OggPlayVideoData * video_data, int frame);
bool FmvWait();
int NextFMVFrame();
int FmvOpen(char *filenamePtr);
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);
void writeFmvData(unsigned char *destData, unsigned char* srcData, int width, int height, int pitch);
int CreateFMVAudioBuffer(int channels, int rate);
//void handle_audio_data (OggPlay * player, int track, OggPlayAudioData * data, int samples);
int WriteToDsound(int dataSize, int offset);
int GetWritableBufferSize();
int updateAudioBuffer(int numBytes, short *data);

extern void EndMenuBackgroundBink()
{
}

extern void StartMenuBackgroundBink()
{
}

void UpdateFMVAudioBuffer(void *arg) 
{

}

extern void PlayBinkedFMV(char *filenamePtr)
{
	return;
}

void FmvClose()
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
	return;
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
					Smack *smackHandle = NULL;
					char filename[30];
					{
						char *filenamePtr = filename;
						do
						{
							*filenamePtr++ = *strPtr;
						}
						while(*strPtr++!='.');

//						*filenamePtr++='s';
//						*filenamePtr++='m';
//						*filenamePtr++='k';
						*filenamePtr++='o';
						*filenamePtr++='g';
						*filenamePtr++='v';
						*filenamePtr=0;
					}

					/* do check here to see if it's a theora file rather than just any old file with the right name? */
					FILE* file=fopen(filename,"rb");
					if(!file)
					{
						OutputDebugString("cant find smacker file!\n");
						OutputDebugString(filename);
					}
					else 
					{	
						OutputDebugString("found smacker file!");
						OutputDebugString(filename);
						OutputDebugString("\n");
						smackHandle = new Smack;
						smackHandle->isValidFile = 1;
						fclose(file);
					}

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
//		ReleaseD3DTexture8(FMVTexture[i].SrcTexture);
//		ReleaseD3DTexture8(FMVTexture[i].SrcSurface);
		ReleaseD3DTexture8(FMVTexture[i].DestTexture);
	}
}

void ReleaseAllFMVTexturesForDeviceReset()
{
	for(int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;
//		ReleaseD3DTexture8(FMVTexture[i].SrcTexture);
//		ReleaseD3DTexture8(FMVTexture[i].SrcSurface);
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
//				SmackClose(FMVTexture[i].SmackHandle);
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

/* not needed */
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

int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr, int pitch)
{
//	OutputDebugString("NextFMVTextureFrame\n");
	int smackerFormat = 1;
	int w = 128;
	int h = 96;
	
	{
//		extern D3DINFO d3d;
//		smackerFormat = GetSmackerPixelFormat(&(d3d.TextureFormat[d3d.CurrentTextureFormat].ddsd.ddpfPixelFormat));
	}
//	if (smackerFormat) w*=2;

#if 0
	if (MoviesAreActive && ftPtr->SmackHandle)
	{
		int volume = MUL_FIXED(SmackerSoundVolume*256,GetVolumeOfNearestVideoScreen());
//		SmackVolumePan(ftPtr->SmackHandle,SMACKTRACKS,volume,PanningOfNearestVideoScreen);
		ftPtr->SoundVolume = SmackerSoundVolume;
	    
//	    if (SmackWait(ftPtr->SmackHandle)) return 0;
		if (FmvWait()) return 0;
		/* unpack frame */

		writeFmvData((unsigned char*)bufferPtr, textureData, w, h, pitch);

//		if (textureData == NULL) return -1;
//		bufferPtr = textureData;
//	  	SmackToBuffer(ftPtr->SmackHandle,0,0,w,96,bufferPtr,smackerFormat);

//		SmackDoFrame(ftPtr->SmackHandle);

		/* are we at the last frame yet? */
//		if (ftPtr->IsTriggeredPlotFMV && (ftPtr->SmackHandle->FrameNum==(ftPtr->SmackHandle->Frames-1)) )
		if (playing == 0)
		{
			if(ftPtr->SmackHandle)
			{
				OutputDebugString("closing ingame fmv..\n");
				FmvClose();

				delete ftPtr->SmackHandle;
				ftPtr->SmackHandle = NULL;
			}

//			SmackClose(ftPtr->SmackHandle);
			
			ftPtr->MessageNumber = 0;
		}
		else
		{
			/* next frame, please */
//			SmackNextFrame(ftPtr->SmackHandle);
		}
		ftPtr->StaticImageDrawn=0;
	}
#endif
	if(!ftPtr->StaticImageDrawn || smackerFormat)
	{
		int i = w*h;///2;
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

extern int NumActiveBlocks;
extern DISPLAYBLOCK *ActiveBlockList[];
#include "showcmds.h"
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
						#if 0
						rightEarDirection.vx = Global_VDB_Ptr->VDB_Mat.mat11;
						rightEarDirection.vy = Global_VDB_Ptr->VDB_Mat.mat12;
						rightEarDirection.vz = Global_VDB_Ptr->VDB_Mat.mat13;
						Normalise(&disp);
						#else
						rightEarDirection.vx = Global_VDB_Ptr->VDB_Mat.mat11;
						rightEarDirection.vy = 0;
						rightEarDirection.vz = Global_VDB_Ptr->VDB_Mat.mat31;
						disp.vy=0;
						Normalise(&disp);
						Normalise(&rightEarDirection);
						#endif
						PanningOfNearestVideoScreen = 32768 + DotProduct(&disp,&rightEarDirection)/2;
					}
				}
			}
		}
	}
//	PrintDebuggingText("Volume: %d, Pan %d\n",VolumeOfNearestVideoScreen,PanningOfNearestVideoScreen);
	return VolumeOfNearestVideoScreen;
}

int FmvOpen(char *filenamePtr)
{
	return 1;
}

void drive_decoding(void *arg) 
{
}

void display_frame(void *arg)  
{	
}

void float_to_short_array(const float* in, short* out, int len) 
{
	int i = 0;
	float scaled_value = 0;		
	for(i = 0; i < len; i++) {				
		scaled_value = floorf(0.5 + 32768 * in[i]);
		if (in[i] < 0) {
			out[i] = (scaled_value < -32768.0) ? -32768 : (short)scaled_value;
		} else {
			out[i] = (scaled_value > 32767.0) ? 32767 : (short)scaled_value;
		}
	}
}
/*
void handle_audio_data(OggPlay * player, int track, OggPlayAudioData * data, int samples) 
{
}
*/

int updateAudioBuffer(int numBytes, short *data)
{
	return 0;
}

int GetWritableBufferSize()
{
	return 0;
}

int WriteToDsound(/*char *audioData,*/ int dataSize, int offset)
{
	return 1;
}
/*
void handle_video_data (OggPlay * player, int track_num, OggPlayVideoData * video_data, int frame) 
{
}
*/
bool FmvWait()
{
	return false;
}

void writeFmvData(unsigned char *destData, unsigned char* srcData, int width, int height, int pitch)
{
}

int NextFMVFrame()
{
	return 0;
}

int CreateFMVAudioBuffer(int channels, int rate)
{
	return 0;
}

} // extern "C"

#endif