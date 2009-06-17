#ifndef FMV_H
#define FMV_H

typedef struct Smack
{
	int isValidFile;
	int FrameNum;
	int Frames;
}Smack;

/* fmv structure */
struct AvPFMV
{
	/* textures needed */
	D3DTEXTURE	texture;
	D3DTEXTURE	dynamicTexture;

};

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

	D3DTEXTURE DestTexture;

}FMVTEXTURE;

extern void StartTriggerPlotFMV(int number);
void PlayMenuMusic();
void StartMenuMusic();
int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr, int pitch);
void ReleaseAllFMVTexturesForDeviceReset();
void RecreateAllFMVTexturesAfterDeviceReset();

extern void SetupFMVTexture(FMVTEXTURE *ftPtr);

#endif // #ifndef _FMV_