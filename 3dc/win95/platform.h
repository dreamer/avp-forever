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

#include "os_header.h"

typedef unsigned int DPID;

#include "aw.h"

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
	Function prototypes
*/

/* 
	Windows functionality.  Note current
	DirectInput functions are also here since
	they are really part of the Win32 multimedia
	library.
*/

void CheckForWindowsMessages(void);
BOOL ExitWindowsSystem(void);
BOOL InitialiseWindowsSystem(HINSTANCE hInstance, int nCmdShow, int WinInitMode);
void KeyboardHandlerKeyDown(WPARAM wParam);
void KeyboardHandlerKeyUp(WPARAM wParam);
int  ReadJoystick(void); 
int  CheckForJoystick(void);


/* Direct 3D */
void ColourFillBackBuffer(int FillColour);
void FlipBuffers(void);
BOOL InitialiseDirect3D(void);
void ReleaseDirect3D(void);
void ReleaseAvPTexture(AVPTEXTURE *texture);

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
void FlushTextprintBuffer(void);
void InitPrintQueue(void);
void InitJoysticks(void);
void ReadJoysticks(void);
int DeallocateAllImages(void);
int MinimizeAllImages(void);
int RestoreAllImages(void);

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

#ifdef __cplusplus
};
#endif

//#define PLATFORM_INCLUDED

#endif
