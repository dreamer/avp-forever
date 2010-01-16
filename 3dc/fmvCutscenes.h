#ifndef _fmvCutscenes_h_
#define _fmvCutscenes_h_

#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "d3_func.h"

#ifdef __cplusplus
extern "C"
{
#endif
void PlayMenuMusic(void);
extern void PlayFMV(const char *filenamePtr);
void StartMenuMusic();
extern void StartTriggerPlotFMV(int number);
void UpdateAllFMVTextures();
#ifdef __cplusplus
}
#endif

typedef struct FMVTEXTURE
{
	IMAGEHEADER *ImagePtr;
	int32_t fmvHandle;
	int SoundVolume;
	int IsTriggeredPlotFMV;
	int StaticImageDrawn;
	int MessageNumber;

	uint8_t *RGBBuffer;

}FMVTEXTURE;

int NextFMVTextureFrame(FMVTEXTURE *ftPtr);
void ReleaseAllFMVTexturesForDeviceReset();
void RecreateAllFMVTexturesAfterDeviceReset();
void StartMenuBackgroundFmv();
void SetupFMVTexture(FMVTEXTURE *ftPtr);

#endif // #ifndef _FMV_