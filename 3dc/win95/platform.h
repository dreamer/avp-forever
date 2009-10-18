#ifndef PLATFORM_INCLUDED
#define PLATFORM_INCLUDED

/*

 Platform Specific Header Include

*/

#ifdef __cplusplus
extern "C"  {
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/*
	Standard windows functionality
*/
#ifdef WIN32
	/*
		Minimise header files to
		speed compiles...
	*/
	#define WIN32_LEAN_AND_MEAN

	#include <windows.h>
	#include <windowsx.h>
	#include <winuser.h>
	#include <mmsystem.h>
#endif

/*
	DirectX functionality
*/
#ifdef WIN32
	#include <d3d9.h>
	typedef int DPID;

	#define DIRECTINPUT_VERSION 0x0800
	#include "dinput.h"
#endif
#ifdef _XBOX
	#include <xtl.h>
	#include "xbox_defines.h"
#endif

#include "aw.h"

#define platform_pc		TRUE
#define Saturn			FALSE

#define Hardware2dTextureClipping FALSE

/*

 Types

*/

typedef RECT RECT_AVP;


/* Watcom C 64-bit values */

typedef struct LONGLONGCH {

	unsigned int lo32;
	int hi32;

} LONGLONGCH;


/*

 Sine and Cosine

*/

#define GetSin(a) sine[a]
#define GetCos(a) cosine[a]

/*
	Available processor types
*/

typedef enum {

	PType_OffBottomOfScale,
	PType_486,
	PType_Pentium,
	PType_P6,
	PType_PentiumMMX,
	PType_Klamath,
	PType_OffTopOfScale

} PROCESSORTYPES;

/*
	Video mode decsription (to be filled
	in by DirectDraw callback).
*/

typedef struct videomodeinfo {

	int Width; /* in pixels */
	int Height; /* in pixels */
	int ColourDepth; /* in bits per pixel */

} VIDEOMODEINFO;

/*
	Maximum number of display modes
	that could be detected on unknown
	hardware.
*/

#define MaxAvailableVideoModes 100

/*
	#defines, structures etc for
	textprint system
*/

#define MaxMsgChars 100
#define MaxMessages 20

/* Font description */

#define CharWidth   8 /* In PIXELS, not bytes */
#define CharHeight  10 /* In PIXELS, not bytes */
#define CharVertSep (CharHeight + 0) /* In PIXELS, not bytes */

#define FontStart 33 // random squiggle in standard ASCII
#define FontEnd 127 // different random squiggle in standard ASCII

#define FontInvisValue 0x00 // value to be colour keyed out of font blit

typedef struct printqueueitem {
	
	char text[MaxMsgChars];
	int text_length;
	int x;
	int y;
		
} PRINTQUEUEITEM;



/* KJL 12:30:05 9/9/97 - new keyboard, mouse etc. enum */
enum KEY_ID
{
	KEY_ESCAPE,

	KEY_0,			
	KEY_1,			
	KEY_2,			
	KEY_3,			
	KEY_4,			
	KEY_5,			
	KEY_6,			
	KEY_7,			
	KEY_8,			
	KEY_9,			

	KEY_A,			
	KEY_B,			
	KEY_C,			
	KEY_D,			
	KEY_E,			
	KEY_F,			
	KEY_G,			
	KEY_H,			
	KEY_I,			
	KEY_J,			
	KEY_K,			
	KEY_L,			
	KEY_M,			
	KEY_N,			
	KEY_O,			
	KEY_P,			
	KEY_Q,			
	KEY_R,			
	KEY_S,			
	KEY_T,			
	KEY_U,			
	KEY_V,			
	KEY_W,			
	KEY_X,			
	KEY_Y,			
	KEY_Z,			

	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_CR,
	KEY_TAB,
	KEY_INS,
	KEY_DEL,
	KEY_END,
	KEY_HOME,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_BACKSPACE,	
	KEY_COMMA,		
	KEY_FSTOP,		
	KEY_SPACE,
			
	KEY_LEFTSHIFT,	
	KEY_RIGHTSHIFT,  
	KEY_LEFTALT,     
	KEY_RIGHTALT,    
	KEY_LEFTCTRL,	
	KEY_RIGHTCTRL,   
	
	KEY_CAPS,		
	KEY_NUMLOCK,		
	KEY_SCROLLOK,	

	KEY_NUMPAD0,     
	KEY_NUMPAD1,     
	KEY_NUMPAD2,     
	KEY_NUMPAD3,     
	KEY_NUMPAD4,     
	KEY_NUMPAD5,     
	KEY_NUMPAD6,     
	KEY_NUMPAD7,     
	KEY_NUMPAD8,     
	KEY_NUMPAD9,     
	KEY_NUMPADSUB,   
	KEY_NUMPADADD,   
	KEY_NUMPADDEL,
	KEY_NUMPADENTER,
	KEY_NUMPADDIVIDE,
	KEY_NUMPADMULTIPLY,

	KEY_LBRACKET,	
	KEY_RBRACKET,	
	KEY_SEMICOLON,	
	KEY_APOSTROPHE,	
	KEY_GRAVE,		
	KEY_BACKSLASH,	
	KEY_SLASH,
	KEY_CAPITAL,
	KEY_MINUS,
	KEY_EQUALS,
	KEY_LWIN,
	KEY_RWIN,
	KEY_APPS,


	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,								 
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,

	KEY_JOYSTICK_BUTTON_1,
	KEY_JOYSTICK_BUTTON_2,
	KEY_JOYSTICK_BUTTON_3,
	KEY_JOYSTICK_BUTTON_4,
	KEY_JOYSTICK_BUTTON_5,
	KEY_JOYSTICK_BUTTON_6,
	KEY_JOYSTICK_BUTTON_7,
	KEY_JOYSTICK_BUTTON_8,
	KEY_JOYSTICK_BUTTON_9,
	KEY_JOYSTICK_BUTTON_10,
	KEY_JOYSTICK_BUTTON_11,
	KEY_JOYSTICK_BUTTON_12,
	KEY_JOYSTICK_BUTTON_13,
	KEY_JOYSTICK_BUTTON_14,
	KEY_JOYSTICK_BUTTON_15,
	KEY_JOYSTICK_BUTTON_16,
	
	KEY_LMOUSE,
	KEY_MMOUSE,
	KEY_RMOUSE,

	KEY_MOUSEBUTTON4,
	KEY_MOUSEWHEELUP,
	KEY_MOUSEWHEELDOWN,

	// Dutch
	KEY_ORDINAL,
	KEY_LESSTHAN,
	KEY_PLUS,

	// French
	KEY_RIGHTBRACKET,
	KEY_ASTERISK,
	KEY_DOLLAR,
	KEY_U_GRAVE,
	KEY_EXCLAMATION,
	KEY_COLON,

	// German
	KEY_BETA,
	KEY_A_UMLAUT,
	KEY_O_UMLAUT,
	KEY_U_UMLAUT,
	KEY_HASH,

	// Spanish
	KEY_UPSIDEDOWNEXCLAMATION,
	KEY_C_CEDILLA,
	KEY_N_TILDE,

	// accents & diacritics
	KEY_DIACRITIC_GRAVE,
	KEY_DIACRITIC_ACUTE,
	KEY_DIACRITIC_CARET,
	KEY_DIACRITIC_UMLAUT,


	KEY_VOID=255, // used to indicate a blank spot in the key config
	MAX_NUMBER_OF_INPUT_KEYS,

};
/* 
	Mouse handler modes (velocity mode is for
	interfacing to input functions that need
	to read the mouse velocity, e.g. WeRequest
	functions, while poistion mode is for menus
	and similar problems).

    The initial mode is defined as a compiled in
	global in game.c
*/

typedef enum {

	MouseVelocityMode,
	MousePositionMode

} MOUSEMODES;


/* Defines for the purpose of familiarity of name only */

#define		LeftButton		0x0001
#define		RightButton		0x0002
#define		MiddleButton	0x0004

#define		LeftMouse		1
#define		MiddleMouse		2
#define		RightMouse		3
#define		ExtraMouse1		4
#define		ExtraMouse2		5

#define MaxScreenWidth 1600		/* Don't get this wrong! */

typedef struct WinScaleXY {

	float x;
	float y;

} WINSCALEXY;

typedef enum {

	WinInitFull,
	WinInitChange

} WININITMODES;

typedef enum {

	WindowModeFullScreen,
	WindowModeSubWindow

} WINDOWMODES;

/*

 .BMP File header

*/

/* 
  Pack the header to 1 byte alignment so that the 
  loader works (John's code, still under test).
*/

#ifdef __WATCOMC__
#pragma pack (1)
#endif

typedef struct bmpheader {

	unsigned short BMP_ID;	/* Contains 'BM' */
	int BMP_Size;

	short BMP_Null1;
	short BMP_Null2;

	int BMP_Image;		/* Byte offset of image start relative to offset 14 */
	int BMP_HeadSize;	/* Size of header (40 for Windows, 12 for OS/2) */
	int BMP_Width;		/* Width of image in pixels */
	int BMP_Height;		/* Height of image in pixels */

	short BMP_Planes;	/* Number of image planes (must be 1) */
	short BMP_Bits;		/* Number of bits per pixel (1,4,8 or 24) */

	int BMP_Comp;			/* Compression type */
	int BMP_CSize;		/* Size in bytes of compressed image */
	int BMP_Hres;			/* Horizontal resolution in pixels/meter */
	int BMP_Vres;			/* Vertical resolution in pixels/meter */
	int BMP_Colours;		/* Number of colours used, below (N) */
	int BMP_ImpCols;		/* Number of important colours */

} BMPHEADER;

/*
	Function prototypes
*/

/* 
	Windows functionality.  Note current
	DirectInput functions are also here since
	they are really part of the Win32 multimedia
	library.
*/

long GetWindowsTickCount(void);
void CheckForWindowsMessages(void);
BOOL ExitWindowsSystem(void);
BOOL InitialiseWindowsSystem(HINSTANCE hInstance, int nCmdShow, int WinInitMode);
void KeyboardHandlerKeyDown(WPARAM wParam);
void KeyboardHandlerKeyUp(WPARAM wParam);
void MouseVelocityHandler(UINT message, LPARAM lParam);
void MousePositionHandler(UINT message, LPARAM lParam);
int  ReadJoystick(void); 
int  CheckForJoystick(void);
BOOL SpawnRasterThread();
BOOL WaitForRasterThread();


/* Direct 3D */
void ColourFillBackBuffer(int FillColour);
void ColourFillBackBufferQuad(int FillColour, int LeftX, int TopY, int RightX, int BotY);
void FlipBuffers(void);
BOOL InitialiseDirect3DImmediateMode(void);
BOOL LockExecuteBuffer(void);
BOOL UnlockExecuteBufferAndPrepareForUse(void);
BOOL BeginD3DScene(void);
BOOL EndD3DScene(void);
BOOL ExecuteBuffer(void);
void ReleaseDirect3D(void);
void WritePolygonToExecuteBuffer(int* itemptr);
void WriteGouraudPolygonToExecuteBuffer(int* itemptr);
void Write2dTexturedPolygonToExecuteBuffer(int* itemptr);
void WriteGouraud2dTexturedPolygonToExecuteBuffer(int* itemptr);
void Write3dTexturedPolygonToExecuteBuffer(int* itemptr);
void WriteGouraud3dTexturedPolygonToExecuteBuffer(int* itemptr);
void WriteBackdrop2dTexturedPolygonToExecuteBuffer(int* itemptr);
void WriteBackdrop3dTexturedPolygonToExecuteBuffer(int* itemptr);
void ReleaseAvPTexture(AvPTexture* texture);
BOOL SetExecuteBufferDefaults(void);
#if SUPPORT_MMX
void SelectMMXOptions(void);
#endif

/* KJL 11:28:31 9/9/97 - Direct Input prototypes */
BOOL InitialiseDirectInput(void);
void ReleaseDirectInput(void);
BOOL InitialiseDirectKeyboard();
void DirectReadKeyboard(void);
void ReleaseDirectKeyboard(void);
BOOL InitialiseDirectMouse();
void DirectReadMouse(void);
void ReleaseDirectMouse(void);

/*
	Internal
*/
#ifdef AVP_DEBUG_VERSION
int textprint(const char* t, ...);
#else
#define textprint(ignore)
#endif

int textprintXY(int x, int y, const char* t, ...);
void LoadSystemFonts(char* fname);
void DisplayWin95String(int x, int y, unsigned char *buffer);
void WriteStringToTextBuffer(int x, int y, unsigned char *buffer);
void FlushTextprintBuffer(void);
void InitPrintQueue(void);
void InitJoysticks(void);
void ReadJoysticks(void);
int DeallocateAllImages(void);
int MinimizeAllImages(void);
int RestoreAllImages(void);
PROCESSORTYPES ReadProcessorType(void);

/*

 Jake's image functions

*/

#ifdef MaxImageGroups

	#if (MaxImageGroups > 1)

		void SetCurrentImageGroup(unsigned int group);
		int DeallocateCurrentImages(void);

	#endif /* MaxImageGroups > 1 */

#endif /* defined(MaxImageGroups) */


/*
	Project callbacks
*/

void ExitGame(void);

#if optimiseflip
void ProcessProjectWhileWaitingToBeFlippable();
#endif

#ifdef __cplusplus
};
#endif

//#define PLATFORM_INCLUDED

#endif
