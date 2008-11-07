// Interface  functions (written in C++) for
// Direct3D immediate mode system

// Must link to C code in main engine system

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

#include <xtl.h>

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

#define DIMouseXOffset 0

#define DIMouseYOffset 4
#define DIMouseZOffset 8

#define DIMouseButton0Offset 12

#define DIMouseButton1Offset 13

#define DIMouseButton2Offset 14

#define DIMouseButton3Offset 15


/*
	Globals
*/

#if 0
static LPDIRECTINPUT            lpdi;          // DirectInput interface
static LPDIRECTINPUTDEVICE      lpdiKeyboard;  // keyboard device interface
static LPDIRECTINPUTDEVICE      lpdiMouse;     // mouse device interface
static BOOL                     DIKeyboardOkay;  // Is the keyboard acquired?

static IDirectInputDevice*     g_pJoystick         = NULL;
static IDirectInputDevice2*    g_pJoystickDevice2  = NULL;  // needed to poll joystick
#endif

	static char bGravePressed = No;
		// added 14/1/98 by DHM as a temporary hack to debounce the GRAVE key

/*
	Externs for input communication
*/

extern HINSTANCE hInst;
extern HWND hWndMain;

int GotMouse;
unsigned int MouseButton;
int MouseVelX;
int MouseVelY;
int MouseVelZ;
int MouseX;
int MouseY;
int MouseZ;

int MovementVelX;
int MovementVelY;
int MovementX;
int MovementY;

extern unsigned char KeyboardInput[];
extern unsigned char GotAnyKey;
static unsigned char LastGotAnyKey;
unsigned char DebouncedGotAnyKey;

int GotJoystick;
//JOYCAPS JoystickCaps;
//JOYINFOEX JoystickData;
int JoystickEnabled;

//DIJOYSTATE JoystickState;          // DirectInput joystick state


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
extern void IngameKeyboardInput_KeyDown(unsigned char key);
extern void IngameKeyboardInput_KeyUp(unsigned char key);
extern void IngameKeyboardInput_ClearBuffer(void);

/*

 Create DirectInput via CoCreateInstance

*/

HANDLE gamePad;
DWORD devices;
bool usePad = false;

BOOL InitialiseDirectInput()
{
	OutputDebugString("\n InitialiseDirectInput()");
	
	// device struct - just gamepads
	XDEVICE_PREALLOC_TYPE deviceTypes[] =
	{
		{XDEVICE_TYPE_GAMEPAD, 1},
	};

	// initialise devices using above struct
//	XInitDevices(sizeof(deviceTypes) / sizeof(XDEVICE_PREALLOC_TYPE),deviceTypes );
	XInitDevices(0 ,NULL);

	devices = XGetDevices(XDEVICE_TYPE_GAMEPAD);

	char buf[100];
	sprintf(buf, "\n devices: %d", devices);
	OutputDebugString(buf);

	if (devices == 0)
	{
		if (XGetDeviceEnumerationStatus() == XDEVICE_ENUMERATION_IDLE)
		{
			OutputDebugString("\n nothing connected..");
		}
		else
		{
			while(XGetDeviceEnumerationStatus() != XDEVICE_ENUMERATION_IDLE)
			{
				DWORD devices = XGetDevices(XDEVICE_TYPE_GAMEPAD);
			}
		}
	}

	if (devices == 0)
	{
		OutputDebugString("\n shit, still nothing..");
	}

	gamePad = XInputOpen( XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT0, XDEVICE_NO_SLOT, NULL );

	if(gamePad)
	{
		usePad = true;
		OutputDebugString("\n init gamepad OK");
	}
	else
	{
		OutputDebugString("\n couldn't use gamepad");
		usePad = false;
	}

/*
	#define NUM_DEVICE_STATES (sizeof(g_dsDevices)/sizeof(*g_dsDevices))

	// Get initial state of all connected devices.
    for( int i = 0; i < NUM_DEVICE_STATES; i++ )
    {
        g_dsDevices[i].dwState = XGetDevices( g_dsDevices[i].pxdt );
        HandleDeviceChanges( g_dsDevices[i].pxdt, g_dsDevices[i].dwState, 0 );
    }
*/

    return TRUE;
}

/*

	Release DirectInput object

*/


void ReleaseDirectInput()
{

}


// see comments below

#define UseForegroundKeyboard No

//GUID     guid = GUID_SysKeyboard;

BOOL InitialiseDirectKeyboard()
{
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

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689

void DirectReadKeyboard()
{
	DWORD inserts, removals;
	XINPUT_STATE inputState;

	// Take a copy of last frame's inputs:
	memcpy((void*)LastFramesKeyboardInput, (void*)KeyboardInput, MAX_NUMBER_OF_INPUT_KEYS);
	LastGotAnyKey = GotAnyKey;

    // Zero current inputs (i.e. set all keys to FALSE,
	// or not pressed)
    memset((void*)KeyboardInput, FALSE, MAX_NUMBER_OF_INPUT_KEYS);
	GotAnyKey = FALSE;
	GotMouse = FALSE;

	if (usePad) 
	{
		/* check for pad removals or inserts */
		XGetDeviceChanges( XDEVICE_TYPE_GAMEPAD, &inserts, &removals );

		if (devices != (removals & (1 << 0))) // check removal of pad at port 0
		{
			//OutputDebugString("Gamepad removed!\n");
			if(gamePad)
				XInputClose(gamePad);
		}

		if (devices != (inserts & (1 << 0)))
		{
			//OutputDebugString("Gamepad inserted!\n");
			gamePad = XInputOpen( XDEVICE_TYPE_GAMEPAD, XDEVICE_PORT0, XDEVICE_NO_SLOT, NULL );
		}

		// Query latest state.
		XInputGetState( gamePad, &inputState );

		/* d-pad */
		if (inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP )
		{
			KeyboardInput[KEY_UP] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN )
		{
			KeyboardInput[KEY_DOWN] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT )
		{
			KeyboardInput[KEY_LEFT] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT )
		{
			KeyboardInput[KEY_RIGHT] = TRUE;
			GotAnyKey = TRUE;
		}

		/* analogue buttons */
		if (inputState.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] > XINPUT_GAMEPAD_MAX_CROSSTALK)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_1] = TRUE;
//			KeyboardInput[KEY_CR] = TRUE; // need to use something for return key for menus
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > XINPUT_GAMEPAD_MAX_CROSSTALK)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_2] = TRUE;
//			KeyboardInput[KEY_ESCAPE] = TRUE; // this can be our back/escape key?
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_X] > XINPUT_GAMEPAD_MAX_CROSSTALK)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_3] = TRUE;
			GotAnyKey = TRUE;
		}
			
		if (inputState.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] > XINPUT_GAMEPAD_MAX_CROSSTALK)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_4] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > XINPUT_GAMEPAD_MAX_CROSSTALK)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_5] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] > XINPUT_GAMEPAD_MAX_CROSSTALK)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_6] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 60)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_7] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 60)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_8] = TRUE;
			GotAnyKey = TRUE;
		}

		/* clicky thumbsticks */
		if (inputState.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_9] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
		{
			KeyboardInput[KEY_JOYSTICK_BUTTON_10] = TRUE;
			GotAnyKey = TRUE;
		}

		/* start and back buttons */
		if (inputState.Gamepad.wButtons & XINPUT_GAMEPAD_START)
		{
//			KeyboardInput[KEY_JOYSTICK_BUTTON_11] = TRUE;
			KeyboardInput[KEY_CR] = TRUE;
			GotAnyKey = TRUE;
		}

		if (inputState.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
		{
//			KeyboardInput[KEY_JOYSTICK_BUTTON_12] = TRUE;
			KeyboardInput[KEY_ESCAPE] = TRUE;
			GotAnyKey = TRUE;
		}
		
		// going to treat the right gamepad stick as our mouse
		int OldMouseX, OldMouseY, OldMouseZ;
 		GotMouse = Yes;
		MouseVelX = 0;
		MouseVelY = 0;
		MouseVelZ = 0;

		// Save mouse x and y for velocity determination
		OldMouseX = MouseX;
		OldMouseY = MouseY;
		OldMouseZ = MouseZ;

		// do the same for movement
		int OldMouseMovementX, OldMouseMovementY;
		MovementVelX = 0;
		MovementVelY = 0;

		// Save mouse x and y for velocity determination
		OldMouseMovementX = MovementX;
		OldMouseMovementY = MovementY;
	
		int x = 0;
		int y = 0;

		// check if we're in the dead zone for x axis
		if (inputState.Gamepad.sThumbRX < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
			inputState.Gamepad.sThumbRX > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		{
			x = 0; // in dead zone, don't update
		}
		else
		{
			x = inputState.Gamepad.sThumbRX;
		}

		// check if we're in the dead zone for y axis
		if (inputState.Gamepad.sThumbRY < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE &&
			inputState.Gamepad.sThumbRY > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
		{
			y = 0; // in dead zone, don't update
		}
		else
		{
			y = inputState.Gamepad.sThumbRY;
		}

		// scale it down a bit
		MouseX += (int)(x / 400.0f);
		MouseY += (int)(y / 400.0f);

	    MouseVelX = DIV_FIXED(MouseX-OldMouseX,NormalFrameTime);
		MouseVelY = DIV_FIXED(MouseY-OldMouseY,NormalFrameTime);

		// use left stick to move?
		// move up
#if 0
		if (inputState.Gamepad.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			KeyboardInput[KEY_UP] = TRUE;
			GotAnyKey = TRUE;
		}

		// move down
		if (inputState.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			KeyboardInput[KEY_DOWN] = TRUE;
			GotAnyKey = TRUE;
		}

		// strafe right
		if (inputState.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			KeyboardInput[KEY_D] = TRUE;
			GotAnyKey = TRUE;
		}

		// strafe left
		if (inputState.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			KeyboardInput[KEY_A] = TRUE;
			GotAnyKey = TRUE;
		}
#endif

		// check if we're in the dead zone for x axis
		if (inputState.Gamepad.sThumbLX < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
			inputState.Gamepad.sThumbLX > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			x = 0; // in dead zone, don't update
		}
		else
		{
			x = inputState.Gamepad.sThumbLX;
		}

		// check if we're in the dead zone for y axis
		if (inputState.Gamepad.sThumbLY < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE &&
			inputState.Gamepad.sThumbLY > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
		{
			y = 0; // in dead zone, don't update
		}
		else
		{
			y = inputState.Gamepad.sThumbLY;
		}

		// scale it down a bit
		MovementX += (int)(x / 400.0f);
		MovementY += (int)(y / 400.0f);

	    MovementVelX = DIV_FIXED(MovementX-OldMouseMovementX,NormalFrameTime);
		MovementVelY = DIV_FIXED(MovementY-OldMouseMovementY,NormalFrameTime);
	}

	#if 1
	{
		int c;

		for (c='a'; c<='z'; c++)
		{
			if (IngameKeyboardInput[c])
			{
				KeyboardInput[KEY_A + c - 'a'] = TRUE;
				GotAnyKey = TRUE;
			}
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

#endif
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

void ReleaseDirectKeyboard()
{

}


BOOL InitialiseDirectMouse()
{
	return TRUE;
}

void DirectReadMouse()
{

}

void ReleaseDirectMouse()
{

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

void InitJoysticks()
{

}

void ReadJoysticks()
{

}

int ReadJoystick()
{
	return No;
}

int CheckForJoystick()
{
	return No;
}

extern void IngameKeyboardInput_KeyDown(unsigned char key)
{
	IngameKeyboardInput[key] = 1;
}

extern void IngameKeyboardInput_KeyUp(unsigned char key)
{
	IngameKeyboardInput[key] = 0;
}

extern void IngameKeyboardInput_ClearBuffer(void)
{
	int i;

	for (i=0; i<=255; i++)
	{
		IngameKeyboardInput[i] = 0;
	}
}

// For extern "C"
};



