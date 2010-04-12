#include "3dc.h"
#include "module.h"
#include "platform.h"
#include "kshape.h"
#include "progress_bar.h"
#include "chnktexi.h"
#include "awtexld.h"
#include "ffstdio.h"
#include "inline.h"
#include "gamedef.h"
#include "psnd.h"
#include "textureManager.h"

extern "C"
{
#include "language.h"
#include "d3d_render.h"

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern unsigned char DebouncedGotAnyKey;

extern void MinimalNetCollectMessages(void);
extern void NetSendMessages(void);
extern void RenderGrabbedScreen(void);

extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);

extern IMAGEHEADER ImageHeaderArray[];

extern int AAFontImageNumber;
extern int FadingGameInAfterLoading;
extern void RenderBriefingText(int centreY, int brightness);
};

static int CurrentPosition = 0;
static int BarLeft;
static int BarRight;
static int BarTop;
static int BarBottom;

static const char* Loading_Image_Name = "Menus\\Loading.rim";
static const char* Loading_Bar_Empty_Image_Name = "Menus\\Loadingbar_empty.rim";
static const char* Loading_Bar_Full_Image_Name = "Menus\\Loadingbar_full.rim";

RECT LoadingBarEmpty_DestRect;
RECT LoadingBarEmpty_SrcRect;
RECT LoadingBarFull_DestRect;
RECT LoadingBarFull_SrcRect;

D3DTEXTURE LoadingBarFullTexture;
D3DTEXTURE LoadingBarEmptyTexture;

uint32_t	fullTextureID = 0;
uint32_t	emptyTextureID = 0;

AVPTEXTURE *image = NULL;
AVPTEXTURE *LoadingBarEmpty = NULL;
AVPTEXTURE *LoadingBarFull = NULL;
AVPTEXTURE *aa_font = NULL;

int fullbarHeight, fullbarWidth, emptybarHeight, emptybarWidth;

void Start_Progress_Bar()
{
	AAFontImageNumber = CL_LoadImageOnce("Common\\aa_font.RIM", LIO_D3DTEXTURE|LIO_RELATIVEPATH|LIO_RESTORABLE);
	
	// load other graphics
	{
		char buffer[100];
		CL_GetImageFileName(buffer, 100, Loading_Bar_Empty_Image_Name, LIO_RELATIVEPATH);
		
		// see if graphic can be found in fast file
		size_t fastFileLength;
		void const * pFastFileData = ffreadbuf(buffer, &fastFileLength);

		if (pFastFileData)
		{
			// load from fast file
			LoadingBarEmpty = AwCreateTexture("pxf", pFastFileData, fastFileLength, 0);
		}
		else
		{
			// load graphic from rim file
			LoadingBarEmpty = AwCreateTexture("sf", buffer, 0);
		}
		// create d3d texture here
		LoadingBarEmptyTexture = CreateD3DTexturePadded(LoadingBarEmpty, &emptybarWidth, &emptybarHeight);
		emptyTextureID = Tex_AddTexture(LoadingBarEmptyTexture, LoadingBarEmpty->width, LoadingBarEmpty->height);
	}
	{
		char buffer[100];
		CL_GetImageFileName(buffer, 100, Loading_Bar_Full_Image_Name, LIO_RELATIVEPATH);
		
		// see if graphic can be found in fast file
		size_t fastFileLength;
		void const * pFastFileData = ffreadbuf(buffer, &fastFileLength);
		
		if (pFastFileData)
		{
			// load from fast file
			LoadingBarFull = AwCreateTexture("pxf", pFastFileData, fastFileLength, 0);
		}
		else
		{
			// load graphic from rim file
			LoadingBarFull = AwCreateTexture("sf", buffer, 0);
		}
		
		LoadingBarFullTexture = CreateD3DTexturePadded(LoadingBarFull, &fullbarWidth, &fullbarHeight);
		fullTextureID = Tex_AddTexture(LoadingBarFullTexture, LoadingBarFull->width, LoadingBarFull->height);
	}
	
	// load background image for bar
	char buffer[100];
	CL_GetImageFileName(buffer, 100, Loading_Image_Name, LIO_RELATIVEPATH);
	
	// see if graphic can be found in fast file
	size_t fastFileLength;
	void const * pFastFileData = ffreadbuf(buffer,&fastFileLength);

	if (pFastFileData)
	{
		// load from fast file
		image = AwCreateTexture("pxf", pFastFileData, fastFileLength, 0);
	}
	else
	{
		// load graphic from rim file
		image = AwCreateTexture("sf", buffer, 0);
	}

	// draw initial progress bar
	LoadingBarEmpty_SrcRect.left = 0;
	LoadingBarEmpty_SrcRect.right = 639;
	LoadingBarEmpty_SrcRect.top = 0;
	LoadingBarEmpty_SrcRect.bottom = 39;
	LoadingBarEmpty_DestRect.left = 0;
	LoadingBarEmpty_DestRect.right = ScreenDescriptorBlock.SDB_Width-1;
	LoadingBarEmpty_DestRect.top = ((ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset)*11)/12;
	LoadingBarEmpty_DestRect.bottom = (ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset)-1;

	{
		ThisFramesRenderingHasBegun();

		DrawProgressBar(LoadingBarEmpty_SrcRect, LoadingBarEmpty_DestRect, emptyTextureID/*LoadingBarEmptyTexture*/, LoadingBarEmpty->width, LoadingBarEmpty->height, emptybarWidth, emptybarHeight);

		RenderBriefingText(ScreenDescriptorBlock.SDB_Height/2, ONE_FIXED);

		ThisFramesRenderingHasFinished();

		FlipBuffers();	
	}

	CurrentPosition = 0;
}

void Set_Progress_Bar_Position(int pos)
{
	int NewPosition = DIV_FIXED(pos,PBAR_LENGTH);
	if(NewPosition>CurrentPosition)
	{

		CurrentPosition = NewPosition;
		LoadingBarFull_SrcRect.left = 0;
		LoadingBarFull_SrcRect.right = MUL_FIXED(639,NewPosition);
		LoadingBarFull_SrcRect.top = 0;
		LoadingBarFull_SrcRect.bottom = 39;
		LoadingBarFull_DestRect.left = 0;
		LoadingBarFull_DestRect.right = MUL_FIXED(ScreenDescriptorBlock.SDB_Width-1,NewPosition);
		LoadingBarFull_DestRect.top = ((ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset)*11)/12;
		LoadingBarFull_DestRect.bottom = (ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset)-1;
		
		ThisFramesRenderingHasBegun();

		// need to render the empty bar here again. As we're not blitting anymore, 
		// the empty bar will only be rendered for one frame.
		DrawProgressBar(LoadingBarEmpty_SrcRect, LoadingBarEmpty_DestRect, /*LoadingBarEmptyTexture*/emptyTextureID, LoadingBarEmpty->width, LoadingBarEmpty->height, emptybarWidth, emptybarHeight);

		// also need this here again, or else the text disappears!
		RenderBriefingText(ScreenDescriptorBlock.SDB_Height/2, ONE_FIXED);

		// now render the green percent loaded overlay
		DrawProgressBar(LoadingBarFull_SrcRect, LoadingBarFull_DestRect, /*LoadingBarFullTexture*/fullTextureID, LoadingBarFull->width, LoadingBarFull->height, fullbarWidth, fullbarHeight);
		
		ThisFramesRenderingHasFinished();
		
		FlipBuffers();	
		/*
		If this is a network game , then check the received network messages from 
		time to time (~every second).
		Has nothing to do with the progress bar , but this is a convenient place to
		do the check.
		*/
		
		if (AvP.Network != I_No_Network)
		{
			static int LastSendTime;
			int time = timeGetTime();
			if (time - LastSendTime > 1000 || time < LastSendTime)
			{
				//time to check our messages 
				LastSendTime = time;
				MinimalNetCollectMessages();
				//send messages , mainly  needed so that the host will send the game description
				//allowing people to join while the host is loading
				NetSendMessages();
			}
		}
	}
}

extern "C"
{

void Game_Has_Loaded(void)
{
	extern int NormalFrameTime;
	extern void RenderStringCentred(char *stringPtr, int centreX, int y, int colour);

	SoundSys_StopAll();
	SoundSys_Management();

	int f = 65536;
	ResetFrameCounter();

	do
	{
		CheckForWindowsMessages();
		ReadUserInput();
		ThisFramesRenderingHasBegun();

		if (f)
		{
			LoadingBarFull_SrcRect.left=0;
			LoadingBarFull_SrcRect.right=639;
			LoadingBarFull_SrcRect.top=0;
			LoadingBarFull_SrcRect.bottom=39;
			LoadingBarFull_DestRect.left=MUL_FIXED(ScreenDescriptorBlock.SDB_Width-1,(ONE_FIXED-f)/2);
			LoadingBarFull_DestRect.right=MUL_FIXED(ScreenDescriptorBlock.SDB_Width-1,f)+LoadingBarFull_DestRect.left;

			int h = MUL_FIXED((ScreenDescriptorBlock.SDB_Height)/24,ONE_FIXED-f);
			LoadingBarFull_DestRect.top=((ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset) *11)/12+h;
			LoadingBarFull_DestRect.bottom=(ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset)-1-h;
			
			// also need this here again, or else the text disappears!
			RenderBriefingText(ScreenDescriptorBlock.SDB_Height/2, ONE_FIXED);

			DrawProgressBar(LoadingBarFull_SrcRect, LoadingBarFull_DestRect, /*LoadingBarFullTexture*/fullTextureID, LoadingBarFull->width, LoadingBarFull->height, fullbarWidth, fullbarHeight);
			f -= NormalFrameTime;
			if (f < 0) 
				f = 0;
		}

		RenderStringCentred(GetTextString(TEXTSTRING_INGAME_PRESSANYKEYTOCONTINUE), ScreenDescriptorBlock.SDB_Width/2, ((ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset)*23)/24-9, 0xffffffff);

		ThisFramesRenderingHasFinished();
		FlipBuffers();	
		FrameCounterHandler();

		/* If in a network game then we may as well check the network messages while waiting*/
		if (AvP.Network != I_No_Network)
		{
			MinimalNetCollectMessages();
			//send messages , mainly  needed so that the host will send the game description
			//allowing people to join while the host is loading
			NetSendMessages();
		}
	}

	while (!DebouncedGotAnyKey && AvP.Network != I_Host);

	FadingGameInAfterLoading=ONE_FIXED;

	if (image)
	{
		ReleaseAvPTexture(image);
		image = NULL;
	}

	if (LoadingBarEmpty) 
	{
		ReleaseAvPTexture(LoadingBarEmpty);
		LoadingBarEmpty = NULL;
	}
	if (LoadingBarFull)
	{
		ReleaseAvPTexture(LoadingBarFull);
		LoadingBarFull = NULL;
	}

	SAFE_RELEASE(LoadingBarEmptyTexture);
	SAFE_RELEASE(LoadingBarFullTexture);
}

} // extern C