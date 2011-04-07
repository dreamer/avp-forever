
#include "iofocus.h"
// keyboard queue stuff
#include "onscreenKeyboard.h"
#include <queue>
#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "stratdef.h"
#include "gamedef.h"
#include "gameplat.h"
#include "usr_io.h"
#include "rentrntq.h"
#include <xtl.h>
#include "showcmds.h"

/*
struct KEYPRESS
{
	char asciiCode;
	char keyCode;
};
*/

std::queue <KEYPRESS> keyboardQueue;

void AddKeyToQueue(char virtualKeyCode)
{
	KEYPRESS newKeyPress = {0};

	newKeyPress.keyCode = virtualKeyCode;

	keyboardQueue.push(newKeyPress);
}

void AddCharToQueue(char asciiCode)
{
	KEYPRESS newKeyPress = {0};

	newKeyPress.asciiCode = asciiCode;

	keyboardQueue.push(newKeyPress);
}

KEYPRESS GetQueueItem()
{
	KEYPRESS newKeyPress = {0};
	newKeyPress = keyboardQueue.front();
	keyboardQueue.pop();

	return newKeyPress;
}

int GetQueueSize()
{
	return keyboardQueue.size();
}

extern void KeyboardEntryQueue_Add(char c);

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

int GotXPad = 0;
int xPadRightX;
int xPadRightY;
int xPadLeftX;
int xPadLeftY;

int xPadLookX;
int xPadLookY;
int xPadMoveX;
int xPadMoveY;

extern unsigned char KeyboardInput[];
extern bool GotAnyKey;
static bool LastGotAnyKey = false;
bool DebouncedGotAnyKey = false;

int GotJoystick;
int JoystickEnabled;

#define MAX_CONTROLLERS 4  // XInput handles up to 4 controllers
#define INPUT_DEADZONE  ( 0.24f * FLOAT(0x7FFF) )  // Default to 24% of the +/- 32767 range.   This is a reasonable default value but can be altered if needed.

// XInput controller state
struct CONTROLER_STATE
{
    XINPUT_STATE state;
    bool bConnected;
	bool bInserted;
	bool bRemoved;
	HANDLE handle;
};

CONTROLER_STATE g_Controllers[MAX_CONTROLLERS];

enum
{
	X,
	A,
	Y,
	B,
	LT,
	RT,
	WHITE,
	BLACK,
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
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30

uint32_t blockGamepadInputTimerStart = 0;

/*
	8/4/98 DHM: A new array, analagous to KeyboardInput, except it's debounced
*/
unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];

// Implementation of the debounced KeyboardInput
// There's probably a more efficient way of getting it direct from DirectInput
// but it's getting late and I can't face reading any more Microsoft documentation...
static unsigned char LastFramesKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];

extern int NormalFrameTime;

static char IngameKeyboardInput[256];
extern void IngameKeyboardInput_KeyDown(unsigned char key);
extern void IngameKeyboardInput_KeyUp(unsigned char key);
extern void IngameKeyboardInput_ClearBuffer(void);

HANDLE gamePad;
DWORD devices;
bool usePad = false;

BOOL InitialiseDirectInput()
{
#if 0
	OutputDebugString("\n InitialiseDirectInput()");

	// device struct - just gamepads
	XDEVICE_PREALLOC_TYPE deviceTypes[] =
	{
		{XDEVICE_TYPE_GAMEPAD, 1},
	};

	// initialise devices using above struct
//	XInitDevices(sizeof(deviceTypes) / sizeof(XDEVICE_PREALLOC_TYPE),deviceTypes );
	XInitDevices(0, NULL);

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
			while (XGetDeviceEnumerationStatus() != XDEVICE_ENUMERATION_IDLE)
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
#endif
    return TRUE;
}

void ReleaseDirectInput()
{
}

BOOL InitialiseDirectKeyboard()
{
    // if we get here, all objects were created successfully
    return TRUE;
}

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689

void DirectReadKeyboard()
{
	// read gamepad/joystick input
	ReadJoysticks();

	// Take a copy of last frame's inputs:
	memcpy((void*)LastFramesKeyboardInput, (void*)KeyboardInput, MAX_NUMBER_OF_INPUT_KEYS);
	LastGotAnyKey = GotAnyKey;

    // Zero current inputs (i.e. set all keys to FALSE,
	// or not pressed)
    memset((void*)KeyboardInput, FALSE, MAX_NUMBER_OF_INPUT_KEYS);
	GotAnyKey = false;

	/*
		ignore gamepad input if this timer is still running down. The game normally keeps processing input as it returns to game from main menu. This timer ensures any button presses on the menu
		aren't carried into the game actions until timer elapses. eg if I bound 'A' to jump, then used 'A' button to select "Return to game" player would jump when ingame appeared as action carried through
		Very hackish but couldn't think of a better way to handle this.
	*/

	if ((timeGetTime() - blockGamepadInputTimerStart) >= 1000)
//	{
//		blockGamepadInputTimer -= ONE_FIXED / 10;
//	}
//	else
	{
		// xbox gamepad buttons
		for (uint32_t i = 0; i < NUMPADBUTTONS; i++)
		{
			if (GamePadButtons[i])
			{
				KeyboardInput[KEY_JOYSTICK_BUTTON_1+i] = TRUE;
				GotAnyKey = true;
			}
		}
	}

	// check the queue
	uint32_t queueSize = GetQueueSize();

	KEYPRESS newKeyPress = {0};

	for (uint32_t i = 0; i < queueSize; i++)
	{
		newKeyPress = GetQueueItem();

		if (newKeyPress.asciiCode)
		{

		}
		else if (newKeyPress.keyCode)
		{
			KeyboardInput[newKeyPress.keyCode] = TRUE;
			GotAnyKey = true;
		}
	}

	// update debounced keys array
	{
		for (uint32_t i = 0;i < MAX_NUMBER_OF_INPUT_KEYS;i++)
		{
			DebouncedKeyboardInput[i] =
			(
				KeyboardInput[i] && !LastFramesKeyboardInput[i]
			);
		}
		DebouncedGotAnyKey = GotAnyKey && !LastGotAnyKey;
	}

	// handle the on screen keyboard input
	if (Osk_IsActive())
	{
		if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_2]) // if osk active and user presses xbox A..
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
/*
			if (key)
			{
				RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR((char)key);
				KeyboardEntryQueue_Add((char)key);
				DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_2] = 0;
			}
*/
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_4]) // users presses B so we go back
		{
			DebouncedKeyboardInput[KEY_ESCAPE] = TRUE;
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_13]) // up
		{
			Osk_MoveUp();
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_14]) // down
		{
			Osk_MoveDown();
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_15]) // left
		{
			Osk_MoveLeft();
		}
		else if (DebouncedKeyboardInput[KEY_JOYSTICK_BUTTON_16]) // right
		{
			Osk_MoveRight();
		}
	}
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
			return "XPAD_WHITE";
		case TEXTSTRING_KEYS_JOYSTICKBUTTON_8:
			return "XPAD_BLACK";
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

void InitJoysticks()
{
	OutputDebugString("InitJoysticks()\n");
	XInitDevices(0, NULL);

	// Get a mask of all currently available devices
	DWORD dwDeviceMask = XGetDevices(XDEVICE_TYPE_GAMEPAD);

	// Open the devices
	for (uint32_t i = 0; i < XGetPortCount(); i++)
	{
//		ZeroMemory( &g_InputStates[i], sizeof(XINPUT_STATE) );
//		ZeroMemory( &g_Gamepads[i], sizeof(XBGAMEPAD) );
        if (dwDeviceMask & (1<<i))
        {
			OutputDebugString("opening a pad\n");
			// Get a handle to the device
			g_Controllers[i].handle = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);

			// Store capabilities of the device
//			XInputGetCapabilities( g_Gamepads[i].hDevice, &g_Gamepads[i].caps );

            // Initialize last pressed buttons
//			XInputGetState( g_Gamepads[i].hDevice, &g_InputStates[i] );

//			g_Gamepads[i].wLastButtons = g_InputStates[i].Gamepad.wButtons;
/*
            for( DWORD b=0; b<8; b++ )
            {
                g_Gamepads[i].bLastAnalogButtons[b] =
                    // Turn the 8-bit polled value into a boolean value
                    ( g_InputStates[i].Gamepad.bAnalogButtons[b] > XINPUT_GAMEPAD_MAX_CROSSTALK );
            }
*/
        }
    }
}

int UpdateControllerState()
{
	DWORD dwInsertions, dwRemovals;
	int numConnected = 0;

	XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &dwInsertions, &dwRemovals);

	// Loop through all gamepads
    for (DWORD i = 0; i < XGetPortCount(); i++ )
	{
		g_Controllers[i].bRemoved = (dwRemovals & (1<<i)) ? true : false;

		if (g_Controllers[i].bRemoved)
        {
            // If the controller was removed after XGetDeviceChanges but before
            // XInputOpen, the device handle will be NULL
            if (g_Controllers[i].handle)
            {
                XInputClose(g_Controllers[i].handle);
				g_Controllers[i].handle = NULL;
			}
            //pGamepads[i].Feedback.Rumble.wLeftMotorSpeed  = 0;
			//pGamepads[i].Feedback.Rumble.wRightMotorSpeed = 0;
        }

		// Handle inserted devices
        g_Controllers[i].bInserted = ( dwInsertions & (1<<i) ) ? true : false;
        if (g_Controllers[i].bInserted)
        {
			OutputDebugString("opening pad\n");
			// TCR Device Types
            g_Controllers[i].handle = XInputOpen( XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL );
		}

		if (g_Controllers[i].handle)
		{
			XInputGetState(g_Controllers[i].handle, &g_Controllers[i].state);
			numConnected++;
		}
	}
#if 0
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
#endif
    return numConnected;
}

void ReadJoysticks()
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

	// check XInput pads
	GotXPad = UpdateControllerState();

	memset(&GamePadButtons[0], 0, sizeof(GamePadButtons));

	for (uint32_t i = 0; i < MAX_CONTROLLERS; i++)
	{
		// check buttons if the controller is actually connected
		if (g_Controllers[i].handle)
		{
			// handle d-pad
			if (g_Controllers[i].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
			{
				GamePadButtons[DUP] = TRUE;
				//OutputDebugString("xinput DPAD UP\n");
			}
			if (g_Controllers[i].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
			{
				GamePadButtons[DDOWN] = TRUE;
				//OutputDebugString("xinput DPAD DOWN\n");
			}
			if (g_Controllers[i].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
			{
				GamePadButtons[DLEFT] = TRUE;
				//OutputDebugString("xinput DPAD LEFT\n");
			}
			if (g_Controllers[i].state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
			{
				GamePadButtons[DRIGHT] = TRUE;
				//OutputDebugString("xinput DPAD RIGHT\n");
			}

			// handle coloured buttons - X A Y B
			if (g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_X] > XINPUT_GAMEPAD_MAX_CROSSTALK)
			{
				GamePadButtons[X] = TRUE;
				//OutputDebugString("xinput button X pressed\n");
			}
			if (g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_A] > XINPUT_GAMEPAD_MAX_CROSSTALK)
			{
				GamePadButtons[A] = TRUE;
				//OutputDebugString("xinput button A pressed\n");
			}
			if (g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_Y] > XINPUT_GAMEPAD_MAX_CROSSTALK)
			{
				GamePadButtons[Y] = TRUE;
				//OutputDebugString("xinput button Y pressed\n");
			}
			if (g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > XINPUT_GAMEPAD_MAX_CROSSTALK)
			{
				GamePadButtons[B] = TRUE;
				//OutputDebugString("xinput button B pressed\n");
			}

			// handle triggers
			if (g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] &&
				g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
			{
				GamePadButtons[LT] = TRUE;
				//OutputDebugString("xinput left trigger pressed\n");
			}

			if (g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] &&
				g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
			{
				GamePadButtons[RT] = TRUE;
				//OutputDebugString("xinput right trigger pressed\n");
			}

			// handle black and white buttons
			if (g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_WHITE] > XINPUT_GAMEPAD_MAX_CROSSTALK)
			{
				GamePadButtons[WHITE] = TRUE;
				//OutputDebugString("xinput white button pressed\n");
			}

			if (g_Controllers[i].state.Gamepad.bAnalogButtons[XINPUT_GAMEPAD_BLACK] > XINPUT_GAMEPAD_MAX_CROSSTALK)
			{
				GamePadButtons[BLACK] = TRUE;
				//OutputDebugString("xinput black button pressed\n");
			}

			// handle back and start */
			if (g_Controllers[i].state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
			{
				GamePadButtons[START] = TRUE;
				//OutputDebugString("xinput start button clicked\n");
			}

			if (g_Controllers[i].state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
			{
				GamePadButtons[BACK] = TRUE;
				//OutputDebugString("xinput back button clicked\n");
			}

			// handle stick clicks
			if (g_Controllers[i].state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
			{
				GamePadButtons[LEFTCLICK] = TRUE;
				//OutputDebugString("xinput left stick clicked\n");
			}
			if (g_Controllers[i].state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
			{
				GamePadButtons[RIGHTCLICK] = TRUE;
				//OutputDebugString("xinput right stick clicked\n");
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
			xPadRightX += static_cast<int>(tempX * 0.002);
			xPadRightY += static_cast<int>(tempY * 0.002);

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

int ReadJoystick()
{
	return FALSE;
}

int CheckForJoystick()
{
	return FALSE;
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

	for (i = 0; i <= 255; i++)
	{
		IngameKeyboardInput[i] = 0;
	}

	for (i = 0; i < NUMPADBUTTONS; i++)
	{
		GamePadButtons[i] = 0;
	}

	// set timer to current time
	blockGamepadInputTimerStart = timeGetTime();
}


