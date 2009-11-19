#ifndef FMV_H
#define FMV_H

typedef struct Smack
{
	int isValidFile;
	int FrameNum;
	int Frames;
}Smack;

#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "d3_func.h"

typedef struct FMVTEXTURE
{
	IMAGEHEADER *ImagePtr;
	Smack *SmackHandle;
	int SoundVolume;
	int IsTriggeredPlotFMV;
	int StaticImageDrawn;

	int MessageNumber;

	byte *RGBBuffer;

}FMVTEXTURE;

extern void StartTriggerPlotFMV(int number);
void PlayMenuMusic();
void StartMenuMusic();
int NextFMVTextureFrame(FMVTEXTURE *ftPtr);
void ReleaseAllFMVTexturesForDeviceReset();
void RecreateAllFMVTexturesAfterDeviceReset();
extern void StartMenuBackgroundFmv();

extern void SetupFMVTexture(FMVTEXTURE *ftPtr);

#endif // #ifndef _FMV_