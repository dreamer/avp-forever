#ifndef _fmvCutscenes_h_
#define _fmvCutscenes_h_

#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "TextureManager.h"

typedef struct FMVTEXTURE
{
	texID_t textureID;
	uint32_t width;
	uint32_t height;
	int32_t fmvHandle;
	int SoundVolume;
	int IsTriggeredPlotFMV;
	int StaticImageDrawn;
	int MessageNumber;

	uint8_t *RGBBuffer;

}FMVTEXTURE;

// variables
extern uint32_t NumberOfFMVTextures;
#define MAX_NO_FMVTEXTURES 10
extern FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];

// functions
void PlayMenuMusic(void);
extern void PlayFMV(const char *filenamePtr);
void StartMenuMusic();
extern void StartTriggerPlotFMV(int number);
void UpdateAllFMVTextures();
void StartMenuBackgroundFmv();
int NextFMVTextureFrame(FMVTEXTURE *ftPtr);
void ReleaseAllFMVTexturesForDeviceReset();
void RecreateAllFMVTexturesAfterDeviceReset();
void SetupFMVTexture(FMVTEXTURE *ftPtr);

#endif // #ifndef _FMV_
