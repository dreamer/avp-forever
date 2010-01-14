#ifndef _fmvCutscenes_h_
#define _fmvCutscenes_h_

#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "d3_func.h"

typedef struct FMVTEXTURE
{
	IMAGEHEADER *ImagePtr;
	//Smack *SmackHandle;
	int SmackHandle;
	int SoundVolume;
	int IsTriggeredPlotFMV;
	int StaticImageDrawn;
	int MessageNumber;

	uint8_t *RGBBuffer;

}FMVTEXTURE;

extern void PlayFMV(const char *filenamePtr);
extern void StartTriggerPlotFMV(int number);
void PlayMenuMusic();
void StartMenuMusic();
int NextFMVTextureFrame(FMVTEXTURE *ftPtr);
void ReleaseAllFMVTexturesForDeviceReset();
void RecreateAllFMVTexturesAfterDeviceReset();
extern void StartMenuBackgroundFmv();

extern void SetupFMVTexture(FMVTEXTURE *ftPtr);

#endif // #ifndef _FMV_