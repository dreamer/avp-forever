#include <XInput.h>
#pragma comment(lib, "XInput.lib");
#include <winbase.h>

XINPUT_STATE state;
DWORD dwResult = XInputGetState( 0, &state );

/*
#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000
*/

void checkXInput()
{
//	GotAnyKey = 0;

	if( dwResult == ERROR_SUCCESS )
	{
//		OutputDebugString("\n Found controller");
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)
		{
//			KeyboardInput[KEY_UP] = 1;
//			GotAnyKey = 1;
//			OutputDebugString("\n pressed up");
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
		{
//			KeyboardInput[KEY_DOWN] = 1;
//			GotAnyKey = 1;
//			OutputDebugString("\n pressed down");
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
		{
		
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
		{
				
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
		{
						
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)
		{
								
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
		{
										
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
		{
												
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
		{
														
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
		{
																
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
		{
//			KeyboardInput[KEY_LMOUSE] = 1;
//			GotAnyKey = 1;
//			OutputDebugString("\n pressed A");
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_B)
		{
																		
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
		{
																				
		}
		if(state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
		{
																						
		}

	}
	else
	{
 //     OutputDebugString("\n No controller found");
	}
}

// call this to init XInput
void clearXInputState()
{
	// set all state values to zero
	ZeroMemory( &state, sizeof(XINPUT_STATE));
	
}
