/* Bink! player, KJL 99/4/30 */
#include "bink.h"

#include "3dc.h"
#include "d3_func.h"

#define UseLocalAssert 1
#include "ourasert.h"

extern char *ScreenBuffer;
extern LPDIRECTSOUND DSObject; 
//extern int GotAnyKey;
extern unsigned char GotAnyKey;
//extern int DebouncedGotAnyKey;
extern unsigned char DebouncedGotAnyKey;
extern int IntroOutroMoviesAreActive;
//extern DDPIXELFORMAT DisplayPixelFormat;

extern void DirectReadKeyboard(void);

void ThisFramesRenderingHasFinished(void);
void ThisFramesRenderingHasBegun(void);
extern void ClearScreenToBlack(void);
static int NextBinkFrame(BINK *binkHandle);
static int GetBinkPixelFormat(void);

static int BinkSurfaceType;

extern D3DINFO d3d;
LPDIRECT3DTEXTURE9 binkTexture;
LPDIRECT3DTEXTURE9 binkDynamicTexture;

/* call this for res change, alt tabbing and whatnot */
extern void ReleaseBinkTextures()
{
	ReleaseD3DTexture8(&binkTexture);
}

void PlayBinkedFMV(char *filenamePtr)
{
	BINK* binkHandle;
	int playing = 1;
	HRESULT LastError;

	if (!IntroOutroMoviesAreActive) return;

	BinkSurfaceType = GetBinkPixelFormat();
	
	/* skip FMV if surface type not supported */
	if (BinkSurfaceType == -1) return;
	
	/* use Direct sound */
	BinkSoundUseDirectSound(DSObject);
	/* open smacker file */
	binkHandle = BinkOpen(filenamePtr,0);
	if (!binkHandle)
	{
		char message[100];
		sprintf(message,"Unable to access file: %s\n",filenamePtr);
		MessageBox(NULL,message,"AvP Error",MB_OK+MB_SYSTEMMODAL);
		exit(0x111);
		return;
	}

//	SAFE_RELEASE(binkTexture);
//	SAFE_RELEASE(binDynamickTexture);
	ReleaseD3DTexture8(&binkTexture);
	ReleaseD3DTexture8(&binkDynamicTexture);

	LastError = d3d.lpD3DDevice->lpVtbl->CreateTexture(d3d.lpD3DDevice, 1024, 1024, 1, 0, D3DFMT_R5G6B5, D3DPOOL_DEFAULT, &binkTexture, 0);
	if(FAILED(LastError))
	{
		OutputDebugString("\n couldn't create bink texture");
		return;
	}
	LastError =	d3d.lpD3DDevice->lpVtbl->CreateTexture(d3d.lpD3DDevice, 1024, 1024, 1, D3DUSAGE_DYNAMIC, D3DFMT_R5G6B5, /*D3DPOOL_DEFAULT*/D3DPOOL_SYSTEMMEM, &binkDynamicTexture, 0);
	if(FAILED(LastError))
	{
		OutputDebugString("\n couldn't create bink dynamic texture");
		return;
	}

	while(playing)
	{
		CheckForWindowsMessages();
	    if (!BinkWait(binkHandle)) 
			playing = NextBinkFrame(binkHandle);
		
//		FlipBuffers();
		DirectReadKeyboard();
		/* added DebouncedGotAnyKey to ensure previous frame's key press for starting level doesn't count */
		if (GotAnyKey && DebouncedGotAnyKey) playing = 0;
	}
	/* close file */
	BinkClose(binkHandle);
	ReleaseD3DTexture8(&binkTexture);
	ReleaseD3DTexture8(&binkDynamicTexture);
}

static int NextBinkFrame(BINK *binkHandle)
{
	D3DLOCKED_RECT texture_rect;
	HRESULT LastError;

	/* unpack frame */
	BinkDoFrame(binkHandle);

	/* lock the d3d texture */
	LastError = binkDynamicTexture->lpVtbl->LockRect(binkDynamicTexture,0,&texture_rect,0,0);
	if(FAILED(LastError))
	{
		OutputDebugString("\n couldn't lock bink dynamic texture");
		return 0;
	}

	/* copy bink frame to texture */
	BinkCopyToBuffer(binkHandle, texture_rect.pBits, texture_rect.Pitch, 1024, 0, 0, BinkSurfaceType);

	/* unlock d3d texture */
	LastError = binkDynamicTexture->lpVtbl->UnlockRect(binkDynamicTexture,0);
	if(FAILED(LastError))
	{
		OutputDebugString("\n couldn't unlock bink dynamic texture");
		return 0;
	}

	/* update rendering texture with FMV image */
	LastError = d3d.lpD3DDevice->lpVtbl->UpdateTexture(d3d.lpD3DDevice, binkDynamicTexture, binkTexture);
	if(FAILED(LastError))
	{
		OutputDebugString("\n couldn't update bink dynamic texture");
		return 0;
	}

	{
		ThisFramesRenderingHasBegun();
		ClearScreenToBlack();
	
			DrawBinkFmv((640-binkHandle->Width)/2, (480-binkHandle->Height)/2, binkHandle->Height, binkHandle->Width, binkTexture);
	
		ThisFramesRenderingHasFinished();
		FlipBuffers();
	}

	/* are we at the last frame yet? */
	if ((binkHandle->FrameNum==(binkHandle->Frames-1))) return 0;

	/* next frame, please */								  
	BinkNextFrame(binkHandle);
	return 1;
}

static int GetBinkPixelFormat(void)
{
#if 0 // bjd
	if( (DisplayPixelFormat.dwFlags & DDPF_RGB) && !(DisplayPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) )
	{
	    int m;
		int redShift=0;
		int greenShift=0;
		int blueShift=0;

		m = DisplayPixelFormat.dwRBitMask;
		LOCALASSERT(m);
		while(!(m&1)) m>>=1;
		while(m&1)
		{
			m>>=1;
			redShift++;
		}

		m = DisplayPixelFormat.dwGBitMask;
		LOCALASSERT(m);
		while(!(m&1)) m>>=1;
		while(m&1)
		{
			m>>=1;
			greenShift++;
		}

		m = DisplayPixelFormat.dwBBitMask;
		LOCALASSERT(m);
		while(!(m&1)) m>>=1;
		while(m&1)
		{
			m>>=1;
			blueShift++;
		}

		if(redShift == 5)
		{
			if (greenShift == 5)
			{
				if (blueShift == 5)
				{
					return BINKSURFACE555;
				}
				else // not supported
				{
					return -1;
				}
			}
			else if (greenShift == 6)
			{
				if (blueShift == 5)
				{
					return BINKSURFACE565;
				}
				else // not supported
				{
					return -1;
				}
			}
		}
		else if (redShift == 6)
		{
			if (greenShift == 5)
			{
				if (blueShift == 5)
				{
					return BINKSURFACE655;
				}
				else // not supported
				{
					return -1;
				}
			}
			else if (greenShift == 6)
			{
				if (blueShift == 4)
				{
					return BINKSURFACE664;
				}
				else // not supported
				{
					return -1;
				}
			}
		}
		else
		{
			return -1;
		}
	}
#endif
	//return -1;
	return BINKSURFACE565;
}

BINK *MenuBackground = 0;

extern void StartMenuBackgroundBink(void)
{
	char *filenamePtr = "fmvs/menubackground.bik";//newer.bik";

	// open smacker file
	MenuBackground = BinkOpen(filenamePtr,0);
	BinkSurfaceType = GetBinkPixelFormat();
	BinkDoFrame(MenuBackground);
}

extern int PlayMenuBackgroundBink(void)
{
	int newframe = 0;
	if(!MenuBackground) return 0;

	if (!BinkWait(MenuBackground)&&IntroOutroMoviesAreActive) newframe=1;

	if(newframe) BinkDoFrame(MenuBackground);

	BinkCopyToBuffer(MenuBackground,(void*)ScreenBuffer,640*2,480,(640-MenuBackground->Width)/2,(480-MenuBackground->Height)/2,BinkSurfaceType|BINKSURFACECOPYALL);

	if(ScreenBuffer == NULL) {
//		OutputDebugString("\n Null ScreenBuffer");
		return 0;
	}

	// next frame, please							  
	if(newframe)BinkNextFrame(MenuBackground);
	return 1;
}
extern void EndMenuBackgroundBink(void)
{
	if(!MenuBackground) return;
	
	BinkClose(MenuBackground);
	MenuBackground = 0;
}

