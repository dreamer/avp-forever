// Interface  functions (written in C++) for
// Direct3D immediate mode system

#define DIRECTINPUT_VERSION 0x0800

#include "logString.h"
#include <dinput.h>
#include <XInput.h> // XInput API
#include "io.h"
#include "Input.h"
#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "strategy_def.h"
#include "gamedef.h"
#include "gameplat.h"
#include "user_io.h"
#include "reentrant_queue.h"
#include "console.h"
#include "onscreenKeyboard.h"
#include "iofocus.h"
#include "showcmds.h"

extern void KeyboardEntryQueue_Add(char c);

// DirectInput key down value
#define DikOn 0x80

// Internal DirectInput driver 
// buffer Size for direct mouse read
#define DMouse_BufferSize         128

// Maximum number of buffered events retrievable 
// from a mouse data acquisition
#define DMouse_RetrieveSize       128

/*
	Globals
*/

/*
static LPDIRECTINPUT8           lpdi;          // DirectInput interface
static LPDIRECTINPUTDEVICE8     lpdiKeyboard;  // keyboard device interface
static LPDIRECTINPUTDEVICE8     lpdiMouse;     // mouse device interface
static BOOL						DIKeyboardOkay;  // Is the keyboard acquired?

static IDirectInputDevice*		g_pJoystick         = NULL;     
static IDirectInputDevice2*		g_pJoystickDevice2  = NULL;  // needed to poll joystick
*/

//static bool bGravePressed = false;
// added 14/1/98 by DHM as a temporary hack to debounce the GRAVE key

/*
	Externs for input communication
*/

//extern HINSTANCE hInst;
//extern HWND hWndMain;

bool GotMouse;
int MouseVelX;
int MouseVelY;
int MouseX;
int MouseY;

extern int16_t MouseWheelStatus;
extern unsigned char KeyboardInput[];
static bool LastGotAnyKey = false;
bool DebouncedGotAnyKey = false;

int GotJoystick;
JOYCAPS JoystickCaps;
JOYINFOEX JoystickData;
int JoystickEnabled;

//DIJOYSTATE JoystickState;          // DirectInput joystick state 

// XInput stuff from dx sdk samples
BOOL GotXPad = FALSE;

// XInput controller state
struct CONTROLER_STATE
{
    XINPUT_STATE state;
    bool bConnected;
};

const int MAX_CONTROLLERS = 4; // XInput handles up to 4 controllers
#define INPUT_DEADZONE  (0.24f * FLOAT(0x7FFF))  // Default to 24% of the +/- 32767 range.   This is a reasonable default value but can be altered if needed.

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
	X,				// KEY_JOYSTICK_BUTTON_1
	A,				// KEY_JOYSTICK_BUTTON_2
	Y,				// KEY_JOYSTICK_BUTTON_3
	B,				// KEY_JOYSTICK_BUTTON_4
	LT,				// KEY_JOYSTICK_BUTTON_5
	RT,				// KEY_JOYSTICK_BUTTON_6
	LB,				// KEY_JOYSTICK_BUTTON_7
	RB,				// KEY_JOYSTICK_BUTTON_8
	BACK,			// KEY_JOYSTICK_BUTTON_9
	START,			// KEY_JOYSTICK_BUTTON_10
	LEFTCLICK,		// KEY_JOYSTICK_BUTTON_11
	RIGHTCLICK,		// KEY_JOYSTICK_BUTTON_12
	DUP,			// KEY_JOYSTICK_BUTTON_13
	DDOWN,			// KEY_JOYSTICK_BUTTON_14
	DLEFT,			// KEY_JOYSTICK_BUTTON_15
	DRIGHT			// KEY_JOYSTICK_BUTTON_16
};

const int NUMPADBUTTONS = 16;
static unsigned char GamePadButtons[NUMPADBUTTONS];

fixed_t blockGamepadInputTimer = 0;

void ClearAllKeyArrays();

/*
	8/4/98 DHM: A new array, analagous to KeyboardInput, except it's debounced
*/

unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];

// Implementation of the debounced KeyboardInput
// There's probably a more efficient way of getting it direct from DirectInput
// but it's getting late and I can't face reading any more Microsoft documentation...
static unsigned char LastFramesKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];

static unsigned char IngameKeyboardInput[256];

// mouse - 5 buttons?
unsigned char MouseButtons[5] = {0};
extern void IngameKeyboardInput_KeyDown(unsigned char key);
extern void IngameKeyboardInput_KeyUp(unsigned char key);
extern void IngameKeyboardInput_ClearBuffer(void);

// defines to make the keyboard buffer code a little more readable - taken from http://msdn.microsoft.com/en-us/library/ms645540.aspx
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
#if 0
	// try to create di object
	if(FAILED(DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&lpdi, NULL)))
	//if (DirectInputCreate(hInst, DIRECTINPUT_VERSION, &lpdi, NULL) != DI_OK)
	{
		LogErrorString("Can't create DirectInput8 object\n");
		#if debug
		ReleaseDirect3D();
		exit(0x4111);
		#else
		return FALSE;
		#endif
	}
	return TRUE;
#endif
}


/*

	Release DirectInput object

*/
void ReleaseDirectInput(void)
{
#if 0
	if (lpdi!= NULL)
	{
		lpdi->Release();
		lpdi = NULL;
	}
#endif
}


// see comments below

#define UseForegroundKeyboard TRUE//FALSE

//GUID     guid = GUID_SysKeyboard;
BOOL InitialiseDirectKeyboard()
{
	return FALSE;

#if 0
	// try to create keyboard device
	if (FAILED(lpdi->CreateDevice(guid, &lpdiKeyboard, NULL)))
	{
		LogErrorString("Couldn't create DirectInput keyboard\n");
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
		LogErrorString("Couldn't set DirectInput keyboard data format\n");
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
		LogErrorString("Couldn't set DirectInput cooperative level\n");
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
#endif
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

	// read gamepad/joystick input
	ReadJoysticks();

	// Take a copy of last frame's inputs:
	memcpy((void*)LastFramesKeyboardInput, (void*)KeyboardInput, MAX_NUMBER_OF_INPUT_KEYS);
	LastGotAnyKey = GotAnyKey;

    // Zero current inputs (i.e. set all keys to FALSE, or not pressed)
    memset((void*)KeyboardInput, FALSE, MAX_NUMBER_OF_INPUT_KEYS);
	GotAnyKey = false;

	// letters
	for (int i = LETTER_A; i <= LETTER_Z; i++)
	{
		if (IngameKeyboardInput[i])
		{
			KeyboardInput[KEY_A + i - LETTER_A] = TRUE;
			GotAnyKey = true;
		}
	}

	// numbers
	for (int i = NUMBER_0; i <= NUMBER_9; i++)
	{
		if (IngameKeyboardInput[i])
		{
			KeyboardInput[KEY_0 + i - NUMBER_0] = TRUE;
			GotAnyKey = true;
		}
	}

	if (IngameKeyboardInput[VK_OEM_7])
	{
		KeyboardInput[KEY_HASH] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_DIVIDE])
	{
		KeyboardInput[KEY_NUMPADDIVIDE] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_MULTIPLY])
	{
		KeyboardInput[KEY_NUMPADMULTIPLY] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_MINUS])
	{
		KeyboardInput[KEY_MINUS] = TRUE;
		GotAnyKey = true;
	}
	if (IngameKeyboardInput[VK_OEM_PLUS])
	{
		KeyboardInput[KEY_EQUALS] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_5])
	{
		KeyboardInput[KEY_BACKSLASH] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_2])
	{
		KeyboardInput[KEY_SLASH] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_4])
	{
		KeyboardInput[KEY_LBRACKET] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_6])
	{
		KeyboardInput[KEY_RBRACKET] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_1])
	{
		KeyboardInput[KEY_SEMICOLON] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_PERIOD])
	{
		KeyboardInput[KEY_FSTOP] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_COMMA])
	{
		KeyboardInput[KEY_COMMA] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_OEM_3])
	{
		KeyboardInput[KEY_APOSTROPHE] = TRUE;
		GotAnyKey = true;
	}


//	if (IngameKeyboardInput[/*249*/'~'])
	if (IngameKeyboardInput[0xDF])
	{
		KeyboardInput[KEY_GRAVE] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_LEFT])
	{
		KeyboardInput[KEY_LEFT] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_RIGHT])
	{
		KeyboardInput[KEY_RIGHT] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_UP])
	{
		KeyboardInput[KEY_UP] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_DOWN])
	{
		KeyboardInput[KEY_DOWN] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_ESCAPE])
	{
		KeyboardInput[KEY_ESCAPE] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_RETURN])
	{
		KeyboardInput[KEY_CR] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_TAB])
	{
		KeyboardInput[KEY_TAB] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F1])
	{
		KeyboardInput[KEY_F1] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F2])
	{
		KeyboardInput[KEY_F2] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F3])
	{
		KeyboardInput[KEY_F3] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F4])
	{
		KeyboardInput[KEY_F4] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F5])
	{
		KeyboardInput[KEY_F5] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F6])
	{
		KeyboardInput[KEY_F6] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F7])
	{
		KeyboardInput[KEY_F7] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F8])
	{
		KeyboardInput[KEY_F8] = TRUE;
		/* KJL 14:51:38 21/04/98 - F8 does screen shots, and so this is a hack
		to make F8 not count in a 'press any key' situation */
		//	   GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F9])
	{
		KeyboardInput[KEY_F9] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F10])
	{
		KeyboardInput[KEY_F10] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F11])
	{
		KeyboardInput[KEY_F11] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_F12])
	{
		KeyboardInput[KEY_F12] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_INSERT])
	{
		KeyboardInput[KEY_INS] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_DELETE])
	{
		KeyboardInput[KEY_DEL] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_END])
	{
		KeyboardInput[KEY_END] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_HOME])
	{
		KeyboardInput[KEY_HOME] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_PRIOR]) // page up virtual key
	{
		KeyboardInput[KEY_PAGEUP] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NEXT]) // page down virtual key
	{
		KeyboardInput[KEY_PAGEDOWN] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_BACK])
	{
		KeyboardInput[KEY_BACKSPACE] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_SPACE])
	{
		KeyboardInput[KEY_SPACE] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_LSHIFT])
	{
		KeyboardInput[KEY_LEFTSHIFT] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_RSHIFT])
	{
		KeyboardInput[KEY_RIGHTSHIFT] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_LCONTROL])
	{
		KeyboardInput[KEY_LEFTCTRL] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_RCONTROL])
	{
		KeyboardInput[KEY_RIGHTCTRL] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_CAPITAL])
	{
		KeyboardInput[KEY_CAPS] = TRUE;
		KeyboardInput[KEY_CAPITAL] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMLOCK])
	{
		KeyboardInput[KEY_NUMLOCK] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_SCROLL])
	{
		KeyboardInput[KEY_SCROLLOK] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_LMENU])
	{
		KeyboardInput[KEY_LEFTALT] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_RMENU])
	{
		KeyboardInput[KEY_RIGHTALT] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD0])
	{
		KeyboardInput[KEY_NUMPAD0] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD1])
	{
		KeyboardInput[KEY_NUMPAD1] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD2])
	{
		KeyboardInput[KEY_NUMPAD2] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD3])
	{
		KeyboardInput[KEY_NUMPAD3] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD4])
	{
		KeyboardInput[KEY_NUMPAD4] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD5])
	{
		KeyboardInput[KEY_NUMPAD5] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD6])
	{
		KeyboardInput[KEY_NUMPAD6] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD7])
	{
		KeyboardInput[KEY_NUMPAD7] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD8])
	{
		KeyboardInput[KEY_NUMPAD8] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_NUMPAD9])
	{
		KeyboardInput[KEY_NUMPAD9] = TRUE;
		GotAnyKey = true;
	}

	// subtract/minus symbol on keypad
	if (IngameKeyboardInput[VK_SUBTRACT])
	{
		KeyboardInput[KEY_NUMPADSUB] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_ADD])
	{
		KeyboardInput[KEY_NUMPADADD] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_DECIMAL])
	{
		KeyboardInput[KEY_NUMPADDEL] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_LWIN])
	{
		KeyboardInput[KEY_LWIN] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_RWIN])
	{
		KeyboardInput[KEY_RWIN] = TRUE;
		GotAnyKey = true;
	}

	if (IngameKeyboardInput[VK_APPS])
	{
		KeyboardInput[KEY_APPS] = TRUE;
		GotAnyKey = true;
	}

	// mouse buttons
	if (MouseButtons[LeftMouse])
	{
		KeyboardInput[KEY_LMOUSE] = TRUE;
		GotAnyKey = true;
	}
	if (MouseButtons[MiddleMouse])
	{
		KeyboardInput[KEY_MMOUSE] = TRUE;
		GotAnyKey = true;
	}
	if (MouseButtons[RightMouse])
	{
		KeyboardInput[KEY_RMOUSE] = TRUE;
		GotAnyKey = true;
	}
	if (MouseButtons[ExtraMouse1])
	{
		KeyboardInput[KEY_MOUSEBUTTON4] = TRUE;
		GotAnyKey = true;
	}
	if (MouseButtons[ExtraMouse2])
	{
		KeyboardInput[KEY_MOUSEBUTTON4] = TRUE;
		GotAnyKey = true;
	}

	// mouse wheel - read using windows messages
	if (MouseWheelStatus > 0)
	{
		KeyboardInput[KEY_MOUSEWHEELUP] = TRUE;
		GotAnyKey = true;
	}
	else if (MouseWheelStatus < 0)
	{
		KeyboardInput[KEY_MOUSEWHEELDOWN] = TRUE;
		GotAnyKey = true;
	}

	// joystick buttons
	if (GotJoystick)
	{
		unsigned int n, bit;

		for (n = 0, bit = 1; n < 16; n++, bit *= 2)
		{
			if (JoystickData.dwButtons&bit)
			{
				KeyboardInput[KEY_JOYSTICK_BUTTON_1+n] = TRUE;
				GotAnyKey = true;
			}
		}
	}

	/*  
		ignore gamepad input if this timer is still running down. The game normally keeps processing input as it returns to game from main menu. This timer ensures any button presses on the menu
		aren't carried into the game actions until timer elapses. eg if I bound 'A' to jump, then used 'A' button to select "Return to game" player would jump when ingame appeared as action carried through
		Very hackish but couldn't think of a better way to handle this.
	*/
	if (blockGamepadInputTimer >= 0)
	{
		blockGamepadInputTimer -= ONE_FIXED / 10;
	}
	// ok to process the input
	else
	{
		// xbox gamepad buttons
		for (int i = 0; i < NUMPADBUTTONS; i++)
		{
			if (GamePadButtons[i])
			{
				KeyboardInput[KEY_JOYSTICK_BUTTON_1+i] = TRUE;
				GotAnyKey = true;
			}
		}
	}

	// update debounced keys array
	for (int i = 0; i < MAX_NUMBER_OF_INPUT_KEYS; i++)
	{ 
		DebouncedKeyboardInput[i] =
		(
			KeyboardInput[i] && !LastFramesKeyboardInput[i]
		);
	}

	DebouncedGotAnyKey = GotAnyKey && !LastGotAnyKey;
	if (DebouncedGotAnyKey)
	{
		//printf("DebouncedGotAnyKey is set\n");
	}
/*
	if (GotAnyKey)
	{
		printf("GotAnyKey is set\n");
	}
*/
	if (!LastGotAnyKey)
	{
	//	printf("LastGotAnyKey is not set\n");
	}

	if (Osk_IsActive())
	{
		if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_15]) {
			Osk_MoveLeft();
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_16]) {
			Osk_MoveRight();
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_13]) {
			Osk_MoveUp();
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_14]) {
			Osk_MoveDown();
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_2])
		{
			KEYPRESS keyPress = Osk_HandleKeypress();

			if (keyPress.asciiCode)
			{
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR(keyPress.asciiCode);
				KeyboardEntryQueue_Add(keyPress.asciiCode);
				DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_2] = 0;
			}
			else if (keyPress.keyCode)
			{
				DebouncedKeyboardInput[keyPress.keyCode] = TRUE;
			}
		}

		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_4])
		{
			DebouncedKeyboardInput[KEY_ESCAPE] = TRUE;
		}
	}

	if ((KeyboardInput[KEY_LEFTSHIFT]) && (DebouncedKeyboardInput[KEY_ESCAPE]))
	{
		Con_Toggle();
		DebouncedKeyboardInput[KEY_ESCAPE] = 0;
		KeyboardInput[KEY_LEFTSHIFT] = 0;
	}
}

/*

 Clean up direct keyboard objects

*/

void ReleaseDirectKeyboard(void)
{
	return;
#if 0
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
#endif
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
		LogErrorString("Couldn't create DirectInput mouse device\n");
		return FALSE;
	}

	// Set the data format to "mouse format".
	if (FAILED(lpdiMouse->SetDataFormat(&c_dfDIMouse)))
	{
		LogErrorString("Couldn't set DirectInput mouse data format\n");
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
		LogErrorString("Couldn't set DirectInput mouse cooperative level\n");
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
		LogErrorString("Couldn't set DirectInput mouse property\n");
		return FALSE;
	}

	// try to acquire the mouse
	hres = lpdiMouse->Acquire();
	
	return TRUE;
#endif
}

extern int xPosRelative;
extern int yPosRelative;
extern bool mouseMoved;

void DirectReadMouse(void)
{
#if 1
	int OldMouseX, OldMouseY;

	GotMouse = false;

	if (mouseMoved == false) {
		return;
	}

	MouseVelX = 0;
	MouseVelY = 0;

//	int tempX = (OldMouseX + MouseX) * 0.5f;
//	int tempY = (OldMouseY + MouseY) * 0.5f;

	// Save mouse x and y for velocity determination
	OldMouseX = MouseX;
	OldMouseY = MouseY;

	GotMouse = true;

	MouseX += xPosRelative * 4;
	MouseY += yPosRelative * 4;

//	char buf[100];
//	sprintf(buf, "x: %d, y: %d\n", xPosRelative, yPosRelative);
//	OutputDebugString(buf);

	MouseVelX = DIV_FIXED(MouseX-OldMouseX, NormalFrameTime);
	MouseVelY = DIV_FIXED(MouseY-OldMouseY, NormalFrameTime);

	mouseMoved = false;

#else
    DIDEVICEOBJECTDATA od[DMouse_RetrieveSize];
    DWORD dwElements = DMouse_RetrieveSize;
	HRESULT hres;
	int OldMouseX, OldMouseY, OldMouseZ;

 	GotMouse = false;
	MouseVelX = 0;
	MouseVelY = 0;
	MouseVelZ = 0;

    hres = lpdiMouse->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),&od[0],&dwElements, 0);

    if (hres == DIERR_INPUTLOST || hres==DIERR_NOTACQUIRED)
	{
		// We had acquisition, but lost it.  Try to reacquire it.
		hres = lpdiMouse->Acquire();
		// FALSE data this time
    	return;
    }

    // Unable to read data
	if (hres != DI_OK) return;

    // Check for any data being picked up
	GotMouse = true;
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
//	textprint("Got Mouse %d\n", GotMouse);
	textprint("Vel X %d\n", MouseVelX);
	textprint("Vel Y %d\n", MouseVelY);
	#endif
#endif
}

void ReleaseDirectMouse(void)
{
#if 0
	return;
	if (lpdiMouse != NULL)
	{
		lpdiMouse->Release();
		lpdiMouse = NULL;
	}
#endif
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
    for (DWORD i = 0; i < MAX_CONTROLLERS; i++)
    {
        // Simply get the state of the controller from XInput.
        dwResult = XInputGetState(i, &g_Controllers[i].state);

        if (dwResult == ERROR_SUCCESS)
		{
            g_Controllers[i].bConnected = true;
			numConnected++;
		}
        else
		{
            g_Controllers[i].bConnected = false;
		}
    }

    return numConnected;
}

char *GetGamePadButtonTextString(enum TEXTSTRING_ID stringID)
{
	switch (stringID)
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

	// Save right x and y for velocity determination
	oldxPadLookX = xPadRightX;
	oldxPadLookY = xPadRightY;

	GotJoystick = 0;//= ReadJoystick();

	// check XInput pads
	GotXPad = UpdateControllerState();

	memset(&GamePadButtons[0], 0, NUMPADBUTTONS);

	for (int i = 0; i < MAX_CONTROLLERS; i++)
	{
		// check buttons if the controller is actually connected
		if (g_Controllers[i].bConnected)
		{
			// handle d-pad
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
			{
				GamePadButtons[DUP] = TRUE;
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
			{
				GamePadButtons[DDOWN] = TRUE;
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
			{
				GamePadButtons[DLEFT] = TRUE;
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
			{
				GamePadButtons[DRIGHT] = TRUE;
			}

			// handle coloured buttons - X A Y B
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
			{
				GamePadButtons[X] = TRUE;
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
			{
				GamePadButtons[A] = TRUE;
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
			{
				GamePadButtons[Y] = TRUE;
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
			{
				GamePadButtons[B] = TRUE;
			}

			// handle triggers
			if (g_Controllers[0].state.Gamepad.bLeftTrigger && 
				g_Controllers[0].state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
			{
				GamePadButtons[LT] = TRUE;
			}

			if (g_Controllers[0].state.Gamepad.bRightTrigger && 
				g_Controllers[0].state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
			{
				GamePadButtons[RT] = TRUE;
			}

			// handle bumper buttons
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
			{
				GamePadButtons[LB] = TRUE;
			}

			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
			{
				GamePadButtons[RB] = TRUE;
			}

			// handle back and start
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
			{
				GamePadButtons[START] = TRUE;
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
			{
				GamePadButtons[BACK] = TRUE;
			}

			// handle stick clicks
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
			{
				GamePadButtons[LEFTCLICK] = TRUE;
			}
			if (g_Controllers[0].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
			{
				GamePadButtons[RIGHTCLICK] = TRUE;
			}

			// handle analogue stick movement

			// Zero value if thumbsticks are within the dead zone
			if ((g_Controllers[i].state.Gamepad.sThumbLX < INPUT_DEADZONE &&
				  g_Controllers[i].state.Gamepad.sThumbLX > -INPUT_DEADZONE) &&
				 (g_Controllers[i].state.Gamepad.sThumbLY < INPUT_DEADZONE &&
				  g_Controllers[i].state.Gamepad.sThumbLY > -INPUT_DEADZONE))
			{
				g_Controllers[i].state.Gamepad.sThumbLX = 0;
				g_Controllers[i].state.Gamepad.sThumbLY = 0;
			}

			if ((g_Controllers[i].state.Gamepad.sThumbRX < INPUT_DEADZONE &&
				  g_Controllers[i].state.Gamepad.sThumbRX > -INPUT_DEADZONE) &&
				 (g_Controllers[i].state.Gamepad.sThumbRY < INPUT_DEADZONE &&
				  g_Controllers[i].state.Gamepad.sThumbRY > -INPUT_DEADZONE))
			{
				g_Controllers[i].state.Gamepad.sThumbRX = 0;
				g_Controllers[i].state.Gamepad.sThumbRY = 0;
			}

			// Right Stick
			tempX = g_Controllers[i].state.Gamepad.sThumbRX;
			tempY = g_Controllers[i].state.Gamepad.sThumbRY;

			// scale it down a bit
			xPadRightX += static_cast<int>(tempX * 0.002f);
			xPadRightY += static_cast<int>(tempY * 0.002f);

			xPadLookX = DIV_FIXED(xPadRightX - oldxPadLookX, NormalFrameTime);
			xPadLookY = DIV_FIXED(xPadRightY - oldxPadLookY, NormalFrameTime);
/*
			char buf[100];
			sprintf(buf,"xpad x: %d xpad y: %d\n", xPadLookX, xPadLookY);
			OutputDebugString(buf);
*/
			// Left Stick - can grab this value directly
			xPadMoveX = g_Controllers[i].state.Gamepad.sThumbLX;
			xPadMoveY = g_Controllers[i].state.Gamepad.sThumbLY;
		}
	}
}

BOOL ReadJoystick(void)
{
	return FALSE;
	MMRESULT joyreturn;

	if (!JoystickControlMethods.JoystickEnabled) 
		return FALSE;

	joyreturn = joyGetPosEx(JOYSTICKID1, &JoystickData);

	if (joyreturn == JOYERR_NOERROR) 
		return TRUE;

	return FALSE;
}

BOOL CheckForJoystick(void)
{
	return FALSE;
	MMRESULT joyreturn;

    joyreturn = joyGetDevCaps(JOYSTICKID1, &JoystickCaps, sizeof(JOYCAPS));

    if (joyreturn == JOYERR_NOERROR)
	{
		return TRUE;
	}

	return FALSE;
}
#else
// Eleventh hour rewrite of joystick code, to support PantherXL trackerball.
// Typical. KJL, 99/5/1
BOOL CALLBACK EnumJoysticksCallback( LPCDIDEVICEINSTANCE pInst, LPVOID lpvContext );


void InitJoysticks(void)
{
	HRESULT hr;
	GotJoystick = FALSE;
	g_pJoystick = NULL;

	hr = lpdi->EnumDevices(DIDEVTYPE_JOYSTICK, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY);
    if ( FAILED(hr) ) return; 

    hr = g_pJoystick->SetDataFormat( &c_dfDIJoystick );
    if ( FAILED(hr) ) return; 

    hr = g_pJoystick->SetCooperativeLevel( hWndMain, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    if ( FAILED(hr) ) return; 

	GotJoystick = TRUE;	
}

void ReadJoysticks(void)
{

	GotJoystick = FALSE;

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

	GotJoystick = TRUE;
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

void ClearAllKeyArrays()
{
	memset(KeyboardInput, 0, MAX_NUMBER_OF_INPUT_KEYS);
	memset(DebouncedKeyboardInput, 0, MAX_NUMBER_OF_INPUT_KEYS);
	GotAnyKey = false;
	LastGotAnyKey = false;
	DebouncedGotAnyKey = false;
}

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
	for (int i = 0; i <= 255; i++)
	{
		IngameKeyboardInput[i] = 0;
	}

	for (int i = 0; i < 5; i++)
	{
		MouseButtons[i] = 0;
	}

	for (int i = 0; i < NUMPADBUTTONS; i++)
	{
		GamePadButtons[i] = 0;
	}

	// start timer to ignore gamepad input
	blockGamepadInputTimer = ONE_FIXED;
}



