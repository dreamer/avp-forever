#ifndef _FMV_
#define _FMV_

//typedef int Smack;

typedef struct Smack
{
	int isValidFile;
	int FrameNum;
	int Frames;
}Smack;

#include "3dc.h"
#include "module.h"
//#include "inline.h"
#include "stratdef.h"
//#include "gamedef.h"
//#include "avp_menus.h"
//#include "avp_userprofile.h"
#include "d3_func.h"

typedef struct FMVTEXTURE
{
	IMAGEHEADER *ImagePtr;
	Smack *SmackHandle;
	int SoundVolume;
	int IsTriggeredPlotFMV;
	int StaticImageDrawn;

	int MessageNumber;

//	D3DTEXTURE SrcSurface;
//	D3DTEXTURE SrcTexture;
	D3DTEXTURE DestTexture;

//	PALETTEENTRY SrcPalette[256];

	int RedScale;
	int GreenScale;
	int BlueScale;

}FMVTEXTURE;

extern void StartTriggerPlotFMV(int number);
void PlayMenuMusic();
void StartMenuMusic();
//int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr);
int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr, int pitch);


#endif // #ifndef _FMV_