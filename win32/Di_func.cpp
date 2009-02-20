// Interface  functions (written in C++) for
// Direct3D immediate mode system

// Must link to C code in main engine system

#include "logString.h"
#include <XInput.h> // XInput API

extern "C" {

// Note: INITGUID has NOT been defined here,
// since the definition in d3_func.cpp is amply
// sufficient.

#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "stratdef.h"
#include "gamedef.h"
#include "gameplat.h"
#include "usr_io.h"
extern "C++"{
#include "iofocus.h"
};

#include "showcmds.h"

// DirectInput key down value
#define DikOn 0x80

// Internal DirectInput driver 
// buffer Size for direct mouse read
#define DMouse_BufferSize         128

// Maximum number of buffered events retrievable 
// from a mouse data acquisition
#define DMouse_RetrieveSize       128

/*
	These are (hopefully) temporary #defines,
	introduced as a hack because the curious 
	FIELD_OFFSET macros in the dinput.h header
	don't appear to compile, at least in Watcom 10.6.
	They will obviously have to be kept up to date
	with changes in the DIMOUSESTATE structure manually.
*/
/*
#define DIMouseXOffset 0

#define DIMouseYOffset 4
#define DIMouseZOffset 8

#define DIMouseButton0Offset 12

#define DIMouseButton1Offset 13

#define DIMouseButton2Offset 14

#define DIMouseButton3Offset 15
*/

/*
	Globals
*/

static LPDIRECTINPUT8           lpdi;          // DirectInput interface
static LPDIRECTINPUTDEVICE8     lpdiKeyboard;  // keyboard device interface
static LPDIRECTINPUTDEVICE8     lpdiMouse;     // mouse device interface
static BOOL						DIKeyboardOkay;  // Is the keyboard acquired?

static IDirectInputDevice*		g_pJoystick         = NULL;     
static IDirectInputDevice2*		g_pJoystickDevice2  = NULL;  // needed to poll joystick

static char bGravePressed = No;
// added 14/1/98 by DHM as a temporary hack to debounce the GRAVE key

/*
	Externs for input communication
*/

extern HINSTANCE hInst;
extern HWND hWndMain;

int GotMouse;
//unsigned int MouseButton;
int MouseVelX;
int MouseVelY;
int MouseVelZ;
int MouseX;
int MouseY;
int MouseZ;

extern unsigned char KeyboardInput[];
extern unsigned char GotAnyKey;
static unsigned char LastGotAnyKey;
unsigned char DebouncedGotAnyKey;

int GotJoystick;
JOYCAPS JoystickCaps;
JOYINFOEX JoystickData;
int JoystickEnabled;

DIJOYSTATE JoystickState;          // DirectInput joystick state 

/* XInput stuff from dx sdk sameples */
int GotXPad = 0;

/* XInput controller state */
struct CONTROLER_STATE
{
    XINPUT_STATE state;
    bool bConnected;
};

#define MAX_CONTROLLERS 4  // XInput handles up to 4 controllers 
#define INPUT_DEADZONE  ( 0.24f * FLOAT(0x7FFF) )  // Default to 24% of the +/- 32767 range.   This is a reasonable default value but can be altered if needed.

CONTROLER_STATE g_Controllers[MAX_CONTROLLERS];

int xPadRightX;
int xPadRightY;
int xPadLeftX;
int xPadLeftY;

int xPadLookX;
int xPadLookY;
int xPadMoveX;
int xPadMoveY;

enum
{
	X,
	A,
	Y,
	B,
	LT,
	RT,
	LB,
	RB,
	BACK,
	START,
	LEFTCLICK,
	RIGHTCLICK,
	DUP,
	DDOWN,
	DLEFT,
	DRIGHT
};

#define NUMPADBUTTONS 16
static char GamePadButtons[NUMPADBUTTONS];

/*
	8/4/98 DHM: A new array, analagous to KeyboardInput, except it's debounced
*/
extern "C"
{
	unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
}

// Implementation of the debounced KeyboardInput
// There's probably a more efficient way of getting it direct from DirectInput
// but it's getting late and I can't face reading any more Microsoft documentation...
static unsigned char LastFramesKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];


extern int NormalFrameTime;

static char IngameKeyboardInput[256];

/* mouse - 5 buttons? */
unsigned char MouseButtons[5] = {0};
extern void IngameKeyboardInput_KeyDown(unsigned char key);
extern void IngameKeyboardInput_KeyUp(unsigned char key);
extern void IngameKeyboardInput_ClearBuffer(void);

/* defines to make the keyboard buffer code a little more readable - taken from http://msdn.microsoft.com/en-us/library/ms645540.aspx */
#define LETTER_A	0x41
#define LETTER_Z	0x5A
#define NUMBER_0	0x30
#define NUMBER_9	0x39
/*

 Create DirectInput via CoCreateInstance

*/

BOOL InitialiseDirectInput(void)
{
	return FALSE;
	// try to create di object
	if(FAILED(DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpdi, NULL)))
	//if (DirectInputCreate(hInst, DIRECTINPUT_VERSION, &lpdi, NULL) != DI_OK)
	{
		LogDxErrorString("Can't create DirectInput8 object\n");
		#if debug
		ReleaseDirect3D();
		exit(0x4111);
		#else
		return FALSE;
		#endif
	}
	return TRUE;
}


/*

	Release DirectInput object

*/
void ReleaseDirectInput(void)
{
	if (lpdi!= NULL)
	{
		lpdi->Release();
		lpdi = NULL;
	}
}


// see comments below

#define UseForegroundKeyboard Yes//No

GUID     guid = GUID_SysKeyboard;
BOOL InitialiseDirectKeyboard()
{
	return FALSE;
	// try to create keyboard device
	if (FAILED(lpdi->CreateDevice(guid, &lpdiKeyboard, NULL)))
	{
		LogDxErrorString("Couldn't create DirectInput keyboard\n");
		#if debug
		ReleaseDirect3D();
		exit(0x4112);
		#else
		return FALSE;
		#endif
	}

	// Tell DirectInput that we want to receive data in keyboard format
	if (FAILED(lpdiKeyboard->SetDataFormat(&c_dfDIKeyboard)))
	{
		LogDxErrorString("Couldn't set DirectInput keyboard data format\n");
		#if debug
		ReleaseDirect3D();
		exit(0x4113);
		#else
		return FALSE;
		#endif
	}

    // set cooperative level
	// this level is the most likely to work across
	// multiple hardware targets
	// (i.e. this is probably best for a production
	// release)
	#if UseForegroundKeyboard
    if (FAILED(lpdiKeyboard->SetCooperativeLevel(hWndMain,
                         DISCL_NONEXCLUSIVE | DISCL_FOREGROUND)))
	#else
	// this level makes alt-tabbing multiple instances in
	// SunWindow mode possible without receiving lots
	// of false inputs
    if (FAILED(lpdiKeyboard->SetCooperativeLevel(hWndMain,
                         DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)))
	#endif
	{
		LogDxErrorString("Couldn't set DirectInput cooperative level\n");
		#if debug
		ReleaseDirect3D();
		exit(0x4114);
		#else
		return FALSE;
		#endif
	}

    // try to acquire the keyboard
	if (FAILED(lpdiKeyboard->Acquire()))
	{
		// keyboard was NOT acquired
		DIKeyboardOkay = FALSE;
	}
	else
	{	
		// keyboard was acquired
		DIKeyboardOkay = TRUE;
	}

    // if we get here, all objects were created successfully
    return TRUE;    
}



/*

 Use DirectInput to read keyboard

 PS: I know this function involves an
 apparently unnecessary layer of translation
 between one keyboard array and another one. 
 This is to allow people to swap from a windows
 procedure keyboard handler to a DirectInput one
 without having to change their IDemand functions.

 I can't think of a faster way to do the translation
 below, but given that it only runs once per frame 
 it shouldn't be too bad.  BUT NOTE THAT IT DOES
 ONLY RUN ONCE PER FRAME (FROM READUSERINPUT) AND 
 SO YOU MUST HAVE A DECENT FRAME RATE IF KEYS ARE NOT
 TO BE MISSED.

 NOTE ALSO THAT IF YOU ARE USING THIS SYSTEM YOU CAN
 ACCESS THE KEYBOARD ARRAY IN A TIGHT LOOP WHILE CALLING
 READUSERINPUT BUT -->NOT<-- CHECKWINDOWSMESSAGES (AS REQUIRED
 FOR THE WINPROC HANDLER).  BUT CHECKFORWINDOWSMESSAGES WON'T DO
 ANY HARM.
*/

void DirectReadKeyboard(void)
{
#if 0
	// Local array for map of all 256 characters on
	// keyboard
	BYTE DiKeybd[256];
	HRESULT hRes;

    // Get keyboard state
    hRes = lpdiKeyboard->GetDeviceState(sizeof(DiKeybd), DiKeybd);
	if (hRes != DI_OK)
	{
		if (hRes == DIERR_INPUTLOST)
		{
			// keyboard control lost; try to reacquire
			DIKeyboardOkay = FALSE;
			hRes = lpdiKeyboard->Acquire();
			if (hRes == DI_OK)
			DIKeyboardOkay = TRUE;
		}
	}

    // Check for error values on routine exit
	if (hRes != DI_OK)
	{
		// failed to read the keyboard
		#if debug
		ReleaseDirect3D();
		exit(0x999774);
		#else
		return;
		#endif
	}
#endif

	/* read gamepad/joystick input */
	ReadJoysticks();

	// Take a copy of last frame's inputs:
	memcpy((void*)LastFramesKeyboardInput, (void*)KeyboardInput, MAX_NUMBER_OF_INPUT_KEYS);
	LastGotAnyKey = GotAnyKey;

    // Zero current inputs (i.e. set all keys to FALSE,
	// or not pressed)
    memset((void*)KeyboardInput, FALSE, MAX_NUMBER_OF_INPUT_KEYS);
	GotAnyKey = FALSE;

	/* letters */
	for(int i = LETTER_A; i <= LETTER_Z; i++)
	{
		if (IngameKeyboardInput[i])
		{
			KeyboardInput[KEY_A + i - LETTER_A] = TRUE;
			GotAnyKey = TRUE;
		}
	}

	/* numbers */
	for(int i = NUMBER_0; i <= NUMBER_9; i++)
	{
		if (IngameKeyboardInput[i])
		{
			KeyboardInput[KEY_0 + i - NUMBER_0] = TRUE;
			GotAnyKey = TRUE;
		}
	}

#if 0
	int c;
	/* do letters */
	for (c='a'; c<='z'; c++)
	{
		if (IngameKeyboardInput[c])
		{
			KeyboardInput[KEY_A + c - 'a'] = TRUE;
		}
	}
#endif

	if (IngameKeyboardInput[VK_OEM_5])
	{
		KeyboardInput[KEY_BACKSLASH] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_OEM_2])
	{
		KeyboardInput[KEY_SLASH] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_OEM_4])
	{
		KeyboardInput[KEY_LBRACKET] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_OEM_6])
	{
		KeyboardInput[KEY_RBRACKET] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_OEM_1])
	{
		KeyboardInput[KEY_SEMICOLON] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_OEM_PERIOD])
	{
		KeyboardInput[KEY_FSTOP] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_OEM_COMMA])
	{
		KeyboardInput[KEY_COMMA] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_OEM_7])
	{
		KeyboardInput[KEY_APOSTROPHE] = TRUE;
		GotAnyKey = TRUE;
	}


//	if (IngameKeyboardInput[/*249*/'~'])
	if (IngameKeyboardInput[0xDF])
	{
		KeyboardInput[KEY_GRAVE] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_LEFT])
	{
		KeyboardInput[KEY_LEFT] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_RIGHT])
	{
		KeyboardInput[KEY_RIGHT] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_UP])
	{
		KeyboardInput[KEY_UP] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_DOWN])
	{
		KeyboardInput[KEY_DOWN] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_ESCAPE])
	{
		KeyboardInput[KEY_ESCAPE] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_RETURN])
	{
		KeyboardInput[KEY_CR] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_TAB])
	{
		KeyboardInput[KEY_TAB] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F1])
	{
		KeyboardInput[KEY_F1] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F2])
	{
		KeyboardInput[KEY_F2] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F3])
	{
		KeyboardInput[KEY_F3] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F4])
	{
		KeyboardInput[KEY_F4] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F5])
	{
		KeyboardInput[KEY_F5] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F6])
	{
		KeyboardInput[KEY_F6] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F7])
	{
		KeyboardInput[KEY_F7] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F8])
	{
		KeyboardInput[KEY_F8] = TRUE;
		/* KJL 14:51:38 21/04/98 - F8 does screen shots, and so this is a hack
		to make F8 not count in a 'press any key' situation */
		//	   GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F9])
	{
		KeyboardInput[KEY_F9] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F10])
	{
		KeyboardInput[KEY_F10] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F11])
	{
		KeyboardInput[KEY_F11] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_F12])
	{
		KeyboardInput[KEY_F12] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_INSERT])
	{
		KeyboardInput[KEY_INS] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_DELETE])
	{
		KeyboardInput[KEY_DEL] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_END])
	{
		KeyboardInput[KEY_END] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_HOME])
	{
		KeyboardInput[KEY_HOME] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_PRIOR]) // page up virtual key
	{
		KeyboardInput[KEY_PAGEUP] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NEXT]) // page down virtual key
	{
		KeyboardInput[KEY_PAGEDOWN] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_BACK])
	{
		KeyboardInput[KEY_BACKSPACE] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_SPACE])
	{
		KeyboardInput[KEY_SPACE] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_LSHIFT])
	{
		KeyboardInput[KEY_LEFTSHIFT] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_RSHIFT])
	{
		KeyboardInput[KEY_RIGHTSHIFT] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_LCONTROL])
	{
		KeyboardInput[KEY_LEFTCTRL] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_RCONTROL])
	{
		KeyboardInput[KEY_RIGHTCTRL] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_CAPITAL])
	{
		KeyboardInput[KEY_CAPS] = TRUE;
		KeyboardInput[KEY_CAPITAL] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMLOCK])
	{
		KeyboardInput[KEY_NUMLOCK] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_SCROLL])
	{
		KeyboardInput[KEY_SCROLLOK] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_LMENU])
	{
		KeyboardInput[KEY_LEFTALT] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_RMENU])
	{
		KeyboardInput[KEY_RIGHTALT] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD0])
	{
		KeyboardInput[KEY_NUMPAD0] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD1])
	{
		KeyboardInput[KEY_NUMPAD1] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD2])
	{
		KeyboardInput[KEY_NUMPAD2] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD3])
	{
		KeyboardInput[KEY_NUMPAD3] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD4])
	{
		KeyboardInput[KEY_NUMPAD4] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD5])
	{
		KeyboardInput[KEY_NUMPAD5] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD6])
	{
		KeyboardInput[KEY_NUMPAD6] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD7])
	{
		KeyboardInput[KEY_NUMPAD7] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD8])
	{
		KeyboardInput[KEY_NUMPAD8] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_NUMPAD9])
	{
		KeyboardInput[KEY_NUMPAD9] = TRUE;
		GotAnyKey = TRUE;
	}

	/* subtract/minus symbol on keypad */
	if (IngameKeyboardInput[VK_SUBTRACT])
	{
		KeyboardInput[KEY_NUMPADSUB] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_ADD])
	{
		KeyboardInput[KEY_NUMPADADD] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_DECIMAL])
	{
		KeyboardInput[KEY_NUMPADDEL] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_LWIN])
	{
		KeyboardInput[KEY_LWIN] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_RWIN])
	{
		KeyboardInput[KEY_RWIN] = TRUE;
		GotAnyKey = TRUE;
	}

	if (IngameKeyboardInput[VK_APPS])
	{
		KeyboardInput[KEY_APPS] = TRUE;
		GotAnyKey = TRUE;
	}

	/* mouse buttons */
	if (MouseButtons[LeftMouse])
	{
		KeyboardInput[KEY_LMOUSE] = TRUE;
		GotAnyKey = TRUE;
	}
	if (MouseButtons[MiddleMouse])
	{
		KeyboardInput[KEY_MMOUSE] = TRUE;
		GotAnyKey = TRUE;
	}
	if (MouseButtons[RightMouse])
	{
		KeyboardInput[KEY_RMOUSE] = TRUE;
		GotAnyKey = TRUE;
	}
	if (MouseButtons[ExtraMouse1])
	{
		KeyboardInput[KEY_MOUSEBUTTON4] = TRUE;
		GotAnyKey = TRUE;
	}
	if (MouseButtons[ExtraMouse2])
	{
		KeyboardInput[KEY_MOUSEBUTTON4] = TRUE;
		GotAnyKey = TRUE;
	}

	#if 0
	{
		int c;
		
		/* do letters */
		for (c='a'; c<='z'; c++)
		{
			if (IngameKeyboardInput[c])
			{
				KeyboardInput[KEY_A + c - 'a'] = TRUE;
				GotAnyKey = TRUE;
			}
		}

		if (IngameKeyboardInput[28])
		{
			KeyboardInput[KEY_CR] = TRUE;
			GotAnyKey = TRUE;
		}

		if (IngameKeyboardInput[246])
		{
			KeyboardInput[KEY_O_UMLAUT] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[228])
		{
			KeyboardInput[KEY_A_UMLAUT] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[252])
		{
			KeyboardInput[KEY_U_UMLAUT] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[223])
		{
			KeyboardInput[KEY_BETA] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['+'])
		{
			KeyboardInput[KEY_PLUS] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['#'])
		{
			KeyboardInput[KEY_HASH] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[161])
		{
			KeyboardInput[KEY_UPSIDEDOWNEXCLAMATION] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[231])
		{
			KeyboardInput[KEY_C_CEDILLA] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[241])
		{
			KeyboardInput[KEY_N_TILDE] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[')'])
		{
			KeyboardInput[KEY_RIGHTBRACKET] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['*'])
		{
			KeyboardInput[KEY_ASTERISK] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['$'])
		{
			KeyboardInput[KEY_DOLLAR] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[249])
		{
			KeyboardInput[KEY_U_GRAVE] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['!'])
		{
			KeyboardInput[KEY_EXCLAMATION] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[':'])
		{
			KeyboardInput[KEY_COLON] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[96])
		{
			KeyboardInput[KEY_DIACRITIC_GRAVE] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[180])
		{
			KeyboardInput[KEY_DIACRITIC_ACUTE] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[94])
		{
			KeyboardInput[KEY_DIACRITIC_CARET] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[168])
		{
			KeyboardInput[KEY_DIACRITIC_UMLAUT] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['<'])
		{
			KeyboardInput[KEY_LESSTHAN] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[176])
		{
			KeyboardInput[KEY_ORDINAL] = TRUE;
			GotAnyKey = TRUE;
		}

		if (IngameKeyboardInput['['])
		{
			KeyboardInput[KEY_LBRACKET] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[']'])
		{
			KeyboardInput[KEY_RBRACKET] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[';'])
		{
			KeyboardInput[KEY_SEMICOLON] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['\''])
		{
			KeyboardInput[KEY_APOSTROPHE] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['\\'])
		{
			KeyboardInput[KEY_BACKSLASH] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['/'])
		{
			KeyboardInput[KEY_SLASH] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['-'])
		{
			KeyboardInput[KEY_MINUS] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['='])
		{
			KeyboardInput[KEY_EQUALS] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput[','])
		{
			KeyboardInput[KEY_COMMA] = TRUE;
			GotAnyKey = TRUE;
		}
		if (IngameKeyboardInput['.'])
		{
			KeyboardInput[KEY_FSTOP] = TRUE;
			GotAnyKey = TRUE;
		}
	}

	#endif
#if 0
    // Check and set keyboard array
	// (test checks only for the moment)
    if (DiKeybd[DIK_LEFT] & DikOn)
	  {
	   KeyboardInput[KEY_LEFT] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_RIGHT] & DikOn)
	  {
	   KeyboardInput[KEY_RIGHT] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_UP] & DikOn)
	  {
	   KeyboardInput[KEY_UP] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_DOWN] & DikOn)
	  {
	   KeyboardInput[KEY_DOWN] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_ESCAPE] & DikOn)
	  {
	   KeyboardInput[KEY_ESCAPE] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_RETURN] & DikOn)
	  {
	   KeyboardInput[KEY_CR] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_TAB] & DikOn)
	  {
	   KeyboardInput[KEY_TAB] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F1] & DikOn)
	  {
	   KeyboardInput[KEY_F1] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F2] & DikOn)
	  {
	   KeyboardInput[KEY_F2] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F3] & DikOn)
	  {
	   KeyboardInput[KEY_F3] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F4] & DikOn)
	  {
	   KeyboardInput[KEY_F4] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F5] & DikOn)
	  {
	   KeyboardInput[KEY_F5] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F6] & DikOn)
	  {
	   KeyboardInput[KEY_F6] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F7] & DikOn)
	  {
	   KeyboardInput[KEY_F7] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F8] & DikOn)
	  {
	   KeyboardInput[KEY_F8] = TRUE;
/* KJL 14:51:38 21/04/98 - F8 does screen shots, and so this is a hack
to make F8 not count in a 'press any key' situation */
//	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F9] & DikOn)
	  {
	   KeyboardInput[KEY_F9] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F10] & DikOn)
	  {
	   KeyboardInput[KEY_F10] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F11] & DikOn)
	  {
	   KeyboardInput[KEY_F11] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_F12] & DikOn)
	  {
	   KeyboardInput[KEY_F12] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_INSERT] & DikOn)
	  {
	   KeyboardInput[KEY_INS] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_DELETE] & DikOn)
	  {
	   KeyboardInput[KEY_DEL] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_END] & DikOn)
	  {
	   KeyboardInput[KEY_END] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_HOME] & DikOn)
	  {
	   KeyboardInput[KEY_HOME] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_PRIOR] & DikOn)
	  {
	   KeyboardInput[KEY_PAGEUP] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_NEXT] & DikOn)
	  {
	   KeyboardInput[KEY_PAGEDOWN] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_BACK] & DikOn)
	  {
	   KeyboardInput[KEY_BACKSPACE] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_SPACE] & DikOn)
	  {
	   KeyboardInput[KEY_SPACE] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_LSHIFT] & DikOn)
	  {
	   KeyboardInput[KEY_LEFTSHIFT] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_RSHIFT] & DikOn)
	  {
	   KeyboardInput[KEY_RIGHTSHIFT] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_LCONTROL] & DikOn)
	  {
	   KeyboardInput[KEY_LEFTCTRL] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_RCONTROL] & DikOn)
	  {
	   KeyboardInput[KEY_RIGHTCTRL] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_CAPSLOCK] & DikOn)
	  {
	   KeyboardInput[KEY_CAPS] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_NUMLOCK] & DikOn)
	  {
	   KeyboardInput[KEY_NUMLOCK] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_SCROLL] & DikOn)
	  {
	   KeyboardInput[KEY_SCROLLOK] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_LMENU] & DikOn)
	  {
	   KeyboardInput[KEY_LEFTALT] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_RMENU] & DikOn)
	  {
	   KeyboardInput[KEY_RIGHTALT] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_0] & DikOn)
	  {
	   KeyboardInput[KEY_0] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_1] & DikOn)
	  {
	   KeyboardInput[KEY_1] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_2] & DikOn)
	  {
	   KeyboardInput[KEY_2] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_3] & DikOn)
	  {
	   KeyboardInput[KEY_3] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_4] & DikOn)
	  {
	   KeyboardInput[KEY_4] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_5] & DikOn)
	  {
	   KeyboardInput[KEY_5] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_6] & DikOn)
	  {
	   KeyboardInput[KEY_6] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_7] & DikOn)
	  {
	   KeyboardInput[KEY_7] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_8] & DikOn)
	  {
	   KeyboardInput[KEY_8] = TRUE;
	   GotAnyKey = TRUE;
	  }

    if (DiKeybd[DIK_9] & DikOn)
	  {
	   KeyboardInput[KEY_9] = TRUE;
	   GotAnyKey = TRUE;
	  }

	
	/* KJL 16:12:19 05/11/97 - numeric pad follows */
	if (DiKeybd[DIK_NUMPAD7] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD7] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD8] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD8] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD9] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD9] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_SUBTRACT] & DikOn)
	{
		KeyboardInput[KEY_NUMPADSUB] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD4] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD4] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD5] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD5] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD6] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD6] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_ADD] & DikOn)
	{
		KeyboardInput[KEY_NUMPADADD] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD1] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD1] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD2] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD2] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD3] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD3] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPAD0] & DikOn)
	{
		KeyboardInput[KEY_NUMPAD0] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_DECIMAL] & DikOn)
	{
		KeyboardInput[KEY_NUMPADDEL] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_NUMPADENTER] & DikOn)
	{
		KeyboardInput[KEY_NUMPADENTER] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_DIVIDE] & DikOn)
	{
		KeyboardInput[KEY_NUMPADDIVIDE] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_MULTIPLY] & DikOn)
	{
		KeyboardInput[KEY_NUMPADMULTIPLY] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_CAPITAL] & DikOn)
	{
		KeyboardInput[KEY_CAPITAL] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_LWIN] & DikOn)
	{
		KeyboardInput[KEY_LWIN] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_RWIN] & DikOn)
	{
		KeyboardInput[KEY_RWIN] = TRUE;
		GotAnyKey = TRUE;
	}
	if (DiKeybd[DIK_APPS] & DikOn)
	{
		KeyboardInput[KEY_APPS] = TRUE;
		GotAnyKey = TRUE;
	}
#endif
#if 0
	#if 0
	// Added 14/1/98 by DHM: Process the grave key (known to some as the tilde key)
	// Done this way as a bit of a hack to avoid touching PLATFORM.H
	// which would force big recompiles
	if (DiKeybd[DIK_GRAVE] & DikOn)
	{
		if ( !bGravePressed )
		{
			IOFOCUS_Toggle();
		}

		bGravePressed = Yes;

		GotAnyKey = TRUE;		
	}
	else
	{
		bGravePressed = No;
	}
	#else
	if (DiKeybd[DIK_GRAVE] & DikOn)
	{
		KeyboardInput[KEY_GRAVE] = TRUE;
	}
	#endif
#endif
#if 0
	/* mouse keys */
	if (MouseButton & LeftButton)
	{
		KeyboardInput[KEY_LMOUSE] = TRUE;
		GotAnyKey = TRUE;
	}
	if (MouseButton & MiddleButton)
	{
		KeyboardInput[KEY_MMOUSE] = TRUE;
		GotAnyKey = TRUE;
	}
	if (MouseButton & RightButton)
	{
		KeyboardInput[KEY_RMOUSE] = TRUE;
		GotAnyKey = TRUE;
	}
#endif
	/* mouse wheel - read using windows messages */
	{
		extern signed int MouseWheelStatus;
		if (MouseWheelStatus>0)
		{
			KeyboardInput[KEY_MOUSEWHEELUP] = TRUE;
			GotAnyKey = TRUE;
		}
		else if (MouseWheelStatus<0)
		{
			KeyboardInput[KEY_MOUSEWHEELDOWN] = TRUE;
			GotAnyKey = TRUE;
		}
	}


	/* joystick buttons */
	if (GotJoystick)
	{
		unsigned int n,bit;

		for (n=0,bit=1; n<16; n++,bit*=2)
		{
			if(JoystickData.dwButtons&bit)
			{
				KeyboardInput[KEY_JOYSTICK_BUTTON_1+n]=TRUE;
				GotAnyKey = TRUE;
			}
		}
	}
	 
	/* xbox gamepad buttons */
	for (int i = 0; i < NUMPADBUTTONS; i++)
	{
		if (GamePadButtons[i])
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_1+i] = TRUE;
			GotAnyKey = TRUE;
		}
	}

	/* update debounced keys array */
	{
		for (int i=0;i<MAX_NUMBER_OF_INPUT_KEYS;i++)
		{ 
			DebouncedKeyboardInput[i] =
			(
				KeyboardInput[i] && !LastFramesKeyboardInput[i]
			);
		}
		DebouncedGotAnyKey = GotAnyKey && !LastGotAnyKey;
	}
}

/*

 Clean up direct keyboard objects

*/

void ReleaseDirectKeyboard(void)
{
	return;
	if (DIKeyboardOkay)
	{
		lpdiKeyboard->Unacquire();
		DIKeyboardOkay = FALSE;
	}

	if (lpdiKeyboard != NULL)
	{
		lpdiKeyboard->Release();
		lpdiKeyboard = NULL;
	}
}


BOOL InitialiseDirectMouse()
{
	return FALSE;
#if 0
    GUID    guid = GUID_SysMouse;
	HRESULT hres;

	//MouseButton = 0;
    
    // Obtain an interface to the system mouse device.
	if (FAILED(lpdi->CreateDevice(guid, &lpdiMouse, NULL)))
	{
		LogDxErrorString("Couldn't create DirectInput mouse device\n");
		return FALSE;
	}

	// Set the data format to "mouse format".
	if (FAILED(lpdiMouse->SetDataFormat(&c_dfDIMouse)))
	{
		LogDxErrorString("Couldn't set DirectInput mouse data format\n");
		return FALSE;
	}

    //  Set the cooperativity level.
    #if debug
	// This level should allow the debugger to actually work
	// not to mention drop 'n' drag in sub-window mode
    hres = lpdiMouse->SetCooperativeLevel(hWndMain,
                       DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	#else
	// this level is the most likely to work across
	// multiple mouse drivers
    hres = lpdiMouse->SetCooperativeLevel(hWndMain,
                       DISCL_EXCLUSIVE | DISCL_FOREGROUND);
	#endif
//    if (hres != DI_OK) return FALSE;
	if(FAILED(hres))
	{
		LogDxErrorString("Couldn't set DirectInput mouse cooperative level\n");
		return FALSE;
	}

    //  Set the buffer size for reading the mouse to
	//  DMouse_BufferSize elements
    //  mouse-type should be relative by default, so there
	//  is no need to change axis mode.
    DIPROPDWORD dipdw =
    {
        {
            sizeof(DIPROPDWORD),        // diph.dwSize
            sizeof(DIPROPHEADER),       // diph.dwHeaderSize
            0,                          // diph.dwObj
            DIPH_DEVICE,                // diph.dwHow
        },
        DMouse_BufferSize,              // dwData
    };

	if (FAILED(lpdiMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
	{
		LogDxErrorString("Couldn't set DirectInput mouse property\n");
		return FALSE;
	}

	// try to acquire the mouse
	hres = lpdiMouse->Acquire();
	
	return TRUE;
#endif
}

extern int xPosRelative;
extern int yPosRelative;
extern int mouseMoved;

void DirectReadMouse(void)
{
#if 1
	int OldMouseX, OldMouseY, OldMouseZ;

	GotMouse = No;

	if(mouseMoved == 0) return;

	MouseVelX = 0;
	MouseVelY = 0;
	MouseVelZ = 0;

	// Save mouse x and y for velocity determination
	OldMouseX = MouseX;
	OldMouseY = MouseY;
	OldMouseZ = MouseZ;

	GotMouse = Yes;

	MouseX += xPosRelative * 4;
	MouseY += yPosRelative * 4;

//	char buf[100];
//	sprintf(buf, "x: %d, y: %d\n", xPosRelative, yPosRelative);
//	OutputDebugString(buf);

	MouseVelX = DIV_FIXED(MouseX-OldMouseX,NormalFrameTime);
	MouseVelY = DIV_FIXED(MouseY-OldMouseY,NormalFrameTime);

	mouseMoved = 0;

#else
    DIDEVICEOBJECTDATA od[DMouse_RetrieveSize];
    DWORD dwElements = DMouse_RetrieveSize;
	HRESULT hres;
	int OldMouseX, OldMouseY, OldMouseZ;

 	GotMouse = No;
	MouseVelX = 0;
	MouseVelY = 0;
	MouseVelZ = 0;

    hres = lpdiMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),&od[0],&dwElements, 0);

    if (hres == DIERR_INPUTLOST || hres==DIERR_NOTACQUIRED)
	{
		// We had acquisition, but lost it.  Try to reacquire it.
		hres = lpdiMouse->Acquire();
		// No data this time
    	return;
    }

    // Unable to read data
	if (hres != DI_OK) return;

    // Check for any data being picked up
	GotMouse = Yes;
	if (dwElements == 0) return;

    // Save mouse x and y for velocity determination
	OldMouseX = MouseX;
	OldMouseY = MouseY;
	OldMouseZ = MouseZ;

    // Process all recovered elements and
	// make appropriate modifications to mouse
	// status variables.

    int i;
	char buf[100];

    for (i=0; i<dwElements; i++)
	{
	// Look at the element to see what happened

		switch (od[i].dwOfs)
		{
			// DIMOFS_X: Mouse horizontal motion
			case DIMOFS_X://DIMouseXOffset:
			    MouseX += od[i].dwData;
//				sprintf(buf, "x: %d\n", od[i].dwData);
//				OutputDebugString(buf);
			    break;

			// DIMOFS_Y: Mouse vertical motion
			case DIMOFS_Y://DIMouseYOffset: 
			    MouseY += od[i].dwData;
//				sprintf(buf, "y: %d\n", od[i].dwData);
//				OutputDebugString(buf);
			    break;

			case DIMOFS_Z://DIMouseZOffset: 
			    MouseZ += od[i].dwData;
				textprint("z info received %d\n",MouseZ);
			    break;

			// DIMOFS_BUTTON0: Button 0 pressed or released
			case DIMOFS_BUTTON0://DIMouseButton0Offset:
			    if (od[i].dwData & DikOn) 
			      // Button pressed
				  MouseButton |= LeftButton;
				else
			      // Button released
				  MouseButton &= ~LeftButton;
			    break;

			// DIMOFS_BUTTON1: Button 1 pressed or released
			case DIMOFS_BUTTON1://DIMouseButton1Offset:
			  if (od[i].dwData & DikOn)
			      // Button pressed
				  MouseButton |= RightButton;
			  else
			      // Button released
				  MouseButton &= ~RightButton;
			  break;

			case DIMOFS_BUTTON2://DIMouseButton2Offset:
			case DIMOFS_BUTTON3://DIMouseButton3Offset:
			  if (od[i].dwData & DikOn)
			      // Button pressed
				  MouseButton |= MiddleButton;
			  else
			      // Button released
				  MouseButton &= ~MiddleButton;
			  break;
			
			default:
			  break;
		}
	}

    MouseVelX = DIV_FIXED(MouseX-OldMouseX,NormalFrameTime);
    MouseVelY = DIV_FIXED(MouseY-OldMouseY,NormalFrameTime);

    //MouseVelZ = DIV_FIXED(MouseZ-OldMouseZ,NormalFrameTime);
	
    #if 0
	textprint("MouseNormalFrameTime %d\n",MouseNormalFrameTime);
	textprint("Got Mouse %d\n", GotMouse);
	textprint("Vel X %d\n", MouseVelX);
	textprint("Vel Y %d\n", MouseVelY);
	#endif
#endif
}

void ReleaseDirectMouse(void)
{
	return;
	if (lpdiMouse != NULL)
	{
		lpdiMouse->Release();
		lpdiMouse = NULL;
	}
}



/*KJL****************************************************************
*                                                                   *
*    JOYSTICK SUPPORT - I've moved all joystick support to here.    *
*                                                                   *
****************************************************************KJL*/


/* KJL 11:32:46 04/30/97 - 

	Okay, this has been changed for the sake of AvP. I know that this
	isn't in AvP\win95\..., but moving this file probably isn't worth
	the trouble.

	This code is designed to read only one joystick.

*/


/*
  Decide which (if any) joysticks
  exist, access capabilities,
  initialise internal variables.
*/
#if 1
void InitJoysticks(void)
{
	JoystickData.dwFlags = (JOY_RETURNALL |	JOY_RETURNCENTERED | JOY_USEDEADZONE);
	JoystickData.dwSize = sizeof(JoystickData);

//    GotJoystick = CheckForJoystick();

	/* maybe move this somewhere else? */
	ZeroMemory( g_Controllers, sizeof( CONTROLER_STATE ) * MAX_CONTROLLERS );
}

int UpdateControllerState()
{
    DWORD dwResult;
	int numConnected = 0;
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
    {
        // Simply get the state of the controller from XInput.
        dwResult = XInputGetState( i, &g_Controllers[i].state );

        if( dwResult == ERROR_SUCCESS )
		{
            g_Controllers[i].bConnected = true;
			numConnected++;
		}
        else
            g_Controllers[i].bConnected = false;
    }

    return numConnected;
}

char *GetGamePadButtonTextString(enum TEXTSTRING_ID stringID)
{
	switch(stringID)
	{
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_1:
			return "XPAD_X";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_2:
			return "XPAD_A";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_3:
			return "XPAD_Y";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_4:
			return "XPAD_B";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_5:
			return "XPAD_LEFT_TRIGGER";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_6:
			return "XPAD_RIGHT_TRIGGER";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_7:
			return "XPAD_LEFT_BUMPER";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_8:
			return "XPAD_RIGHT_BUMPER";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_9:
			return "XPAD_BACK";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_10:
			return "XPAD_START";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_11:
			return "XPAD_LEFT_CLICK";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_12:
			return "XPAD_RIGHT_CLICK";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_13:
			return "XPAD_DPAD_UP";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_14:
			return "XPAD_DPAD_DOWN";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_15:
			return "XPAD_DPAD_LEFT";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_16:
			return "XPAD_DPAD_RIGHT";
		default:
			return "NO_BIND";
	}
}

void ReadJoysticks(void)
{
	int tempX = 0;
	int tempY = 0;
	int oldxPadLookX, oldxPadLookY;

	xPadLookX = 0;
	xPadLookY = 0;
	xPadMoveX = 0;
	xPadMoveY = 0;

	/* Save right x and y for velocity determination */
	oldxPadLookX = xPadRightX;
	oldxPadLookY = xPadRightY;

	GotJoystick = 0;//= ReadJoystick();

	/* check XInput pads */
	GotXPad = UpdateControllerState();

	memset(&GamePadButtons[0], 0, sizeof(GamePadButtons));

	for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
	{
		/* check buttons if the controller is actually connected */
		if( g_Controllers[i].bConnected )
		{
			/* handle d-pad */
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP )
			{
				GamePadButtons[DUP] = Yes;
				//OutputDebugString("xinput DPAD UP\n");
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN )
			{
				GamePadButtons[DDOWN] = Yes;
				//OutputDebugString("xinput DPAD DOWN\n");
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT )
			{
				GamePadButtons[DLEFT] = Yes;
				//OutputDebugString("xinput DPAD LEFT\n");
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT )
			{
				GamePadButtons[DRIGHT] = Yes;
				//OutputDebugString("xinput DPAD RIGHT\n");
			}

			/* handle coloured buttons - X A Y B */
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
			{
				GamePadButtons[X] = Yes;
//				OutputDebugString("xinput button X pressed\n");
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
			{
				GamePadButtons[A] = Yes;
				//OutputDebugString("xinput button A pressed\n");
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
			{
				GamePadButtons[Y] = Yes;
				//OutputDebugString("xinput button Y pressed\n");
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
			{
				GamePadButtons[B] = Yes;
				//OutputDebugString("xinput button B pressed\n");
			}

			/* handle triggers */
			if (g_Controllers[0].state.Gamepad.bLeftTrigger && 
				g_Controllers[0].state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
			{
				GamePadButtons[LT] = Yes;
				//OutputDebugString("xinput left trigger pressed\n");
			}

			if (g_Controllers[0].state.Gamepad.bRightTrigger && 
				g_Controllers[0].state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
			{
				GamePadButtons[RT] = Yes;
				//OutputDebugString("xinput right trigger pressed\n");
			}

			/* handle bumper buttons */
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
			{
				GamePadButtons[LB] = Yes;
				//OutputDebugString("xinput left bumper pressed\n");
			}

			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
			{
				GamePadButtons[RB] = Yes;
				//OutputDebugString("xinput right bumper pressed\n");
			}

			/* handle back and start */
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
			{
				GamePadButtons[START] = Yes;
				//OutputDebugString("xinput start pressed\n");
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
			{
				GamePadButtons[BACK] = Yes;
				//OutputDebugString("xinput back pressed\n");
			}

			/* handle stick clicks */
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
			{
				GamePadButtons[LEFTCLICK] = Yes;
				//OutputDebugString("xinput left stick clicked\n");
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
			{
				GamePadButtons[RIGHTCLICK] = Yes;
				//OutputDebugString("xinput right stick clicked\n");
			}

			/* handle analogue stick movement */

			/* Zero value if thumbsticks are within the dead zone */
			if( ( g_Controllers[i].state.Gamepad.sThumbLX < INPUT_DEADZONE &&
				  g_Controllers[i].state.Gamepad.sThumbLX > -INPUT_DEADZONE ) &&
				( g_Controllers[i].state.Gamepad.sThumbLY < INPUT_DEADZONE &&
				  g_Controllers[i].state.Gamepad.sThumbLY > -INPUT_DEADZONE ) )
			{
				g_Controllers[i].state.Gamepad.sThumbLX = 0;
				g_Controllers[i].state.Gamepad.sThumbLY = 0;
			}

			if( ( g_Controllers[i].state.Gamepad.sThumbRX < INPUT_DEADZONE &&
				  g_Controllers[i].state.Gamepad.sThumbRX > -INPUT_DEADZONE ) &&
				( g_Controllers[i].state.Gamepad.sThumbRY < INPUT_DEADZONE &&
				  g_Controllers[i].state.Gamepad.sThumbRY > -INPUT_DEADZONE ) )
			{
				g_Controllers[i].state.Gamepad.sThumbRX = 0;
				g_Controllers[i].state.Gamepad.sThumbRY = 0;
			}

			/* Right Stick */
			tempX = g_Controllers[i].state.Gamepad.sThumbRX;
			tempY = g_Controllers[i].state.Gamepad.sThumbRY;

			/* scale it down a bit */
			xPadRightX += (int)(tempX / 600.0f);
			xPadRightY += (int)(tempY / 600.0f);

			xPadLookX = DIV_FIXED(xPadRightX - oldxPadLookX, NormalFrameTime);
			xPadLookY = DIV_FIXED(xPadRightY - oldxPadLookY, NormalFrameTime);
/*
			char buf[100];
			sprintf(buf,"xpad x: %d xpad y: %d\n", xPadLookX, xPadLookY);
			OutputDebugString(buf);
*/
			/* Left Stick - can grab this value directly */
			xPadMoveX = g_Controllers[i].state.Gamepad.sThumbLX;
			xPadMoveY = g_Controllers[i].state.Gamepad.sThumbLY;
		}
	}
}

int ReadJoystick(void)
{
	return No;
	MMRESULT joyreturn;

	if(!JoystickControlMethods.JoystickEnabled) return No;

	joyreturn = joyGetPosEx(JOYSTICKID1,&JoystickData);

	if (joyreturn == JOYERR_NOERROR) return Yes;

	return No;
}

int CheckForJoystick(void)
{
	return No;
	MMRESULT joyreturn;

    joyreturn = joyGetDevCaps(JOYSTICKID1, 
                &JoystickCaps,
                sizeof(JOYCAPS));

    if (joyreturn == JOYERR_NOERROR) return Yes;

	return No;
}
#else
// Eleventh hour rewrite of joystick code, to support PantherXL trackerball.
// Typical. KJL, 99/5/1
BOOL CALLBACK EnumJoysticksCallback( LPCDIDEVICEINSTANCE pInst, LPVOID lpvContext );


void InitJoysticks(void)
{
	HRESULT hr;
	GotJoystick = No;
	g_pJoystick = NULL;

	hr = lpdi->EnumDevices(DIDEVTYPE_JOYSTICK, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);
    if ( FAILED(hr) ) return; 

    hr = g_pJoystick->SetDataFormat( &c_dfDIJoystick );
    if ( FAILED(hr) ) return; 

    hr = g_pJoystick->SetCooperativeLevel( hWndMain, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    if ( FAILED(hr) ) return; 

	GotJoystick = Yes;	
}

void ReadJoysticks(void)
{

	GotJoystick = No;

	if(!JoystickControlMethods.JoystickEnabled || g_pJoystick==NULL)
	{
		return;
	}

    HRESULT hr = DIERR_INPUTLOST;

    // if input is lost then acquire and keep trying 
    while ( DIERR_INPUTLOST == hr ) 
    {
        // poll the joystick to read the current state
        hr = g_pJoystickDevice2->Poll();
        if ( FAILED(hr) ) return;

        // get the input's device state, and put the state in dims
        hr = g_pJoystick->GetDeviceState( sizeof(DIJOYSTATE), &JoystickState );

        if ( hr == DIERR_INPUTLOST )
        {
            // DirectInput is telling us that the input stream has
            // been interrupted.  We aren't tracking any state
            // between polls, so we don't have any special reset
            // that needs to be done.  We just re-acquire and
            // try again.
            hr = g_pJoystick->Acquire();
            if ( FAILED(hr) ) return;
        }
    }

    if ( FAILED(hr) ) return;

	GotJoystick = Yes;
	PrintDebuggingText("%d %d\n",JoystickState.rglSlider[0],JoystickState.rglSlider[1]);
	
}
//-----------------------------------------------------------------------------
// Function: EnumJoysticksCallback
//
// Description: 
//      Called once for each enumerated joystick. If we find one, 
//       create a device interface on it so we can play with it.
//
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticksCallback( LPCDIDEVICEINSTANCE pInst, 
                                     LPVOID lpvContext )
{
    HRESULT             hr;
    LPDIRECTINPUTDEVICE pDevice;

    // obtain an interface to the enumerated force feedback joystick.
    hr = lpdi->CreateDevice( pInst->guidInstance, &pDevice, NULL );

    // if it failed, then we can't use this joystick for some
    // bizarre reason.  (Maybe the user unplugged it while we
    // were in the middle of enumerating it.)  So continue enumerating
    if ( FAILED(hr) ) 
        return DIENUM_CONTINUE;

    // we successfully created an IDirectInputDevice.  So stop looking 
    // for another one.
    g_pJoystick = pDevice;

    // query for IDirectInputDevice2 - we need this to poll the joystick 
    pDevice->QueryInterface( IID_IDirectInputDevice2, (LPVOID *)&g_pJoystickDevice2 );

    return DIENUM_STOP;
}
#endif


extern void IngameKeyboardInput_KeyDown(unsigned char key)
{
	IngameKeyboardInput[key] = 1;
}

extern void IngameKeyboardInput_KeyUp(unsigned char key)
{
	IngameKeyboardInput[key] = 0;
}

extern void Mouse_ButtonDown(unsigned char button)
{
	MouseButtons[button] = 1;
}

extern void Mouse_ButtonUp(unsigned char button)
{
	MouseButtons[button] = 0;
}

extern void IngameKeyboardInput_ClearBuffer(void)
{
	int i;

	for (i=0; i<=255; i++)
	{
		IngameKeyboardInput[i] = 0;
	}

	for (i = 0; i < 5; i++)
	{
		MouseButtons[i] = 0;
	}
}

// For extern "C"
};



