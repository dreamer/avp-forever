#ifdef WIN32
/****

Project specific (or potentially project specific) windows functionality

****/

// To link code to main C functions 

extern "C" {

extern unsigned char KeyboardInput[256];
extern unsigned char GotAnyKey;

#include "3dc.h"
#include "inline.h"
#include "cd_player.h"
#include "psndplat.h"

#include "console.h"
#include "onscreenKeyboard.h"

#include "rentrntq.h"
	// Added 21/11/97 by DHM: support for a queue of Windows
	// messages to avoid problems with re-entrancy due to WinProc()
	
#include "alt_tab.h"

	
#include "dxlog.h"
#include "zmouse.h"

void MakeToAsciiTable(void);

// mousewheel msg id
UINT const RWM_MOUSEWHEEL = RegisterWindowMessage(MSH_MOUSEWHEEL);
signed int MouseWheelStatus;

unsigned char ksarray[256];
unsigned char ToAsciiTable[256][256];

// Dubious
#define grabmousecapture FALSE

/*
	Name of project window etc for Win95 interface
	Project specific (fairly obviously...).
	Determines the default menu in which the application
	appears (altho' other code will undoubtedly be needed
	as well...), so that a NULL here should ensure no menu.
*/

#define NAME "AvP"
#define TITLE "AvP"

//	Necessary globals

HWND 		hWndMain;
BOOL        bActive = TRUE;        // is application active?

// Parameters for main (assumed full screen) window
int WinLeftX, WinRightX, WinTopY, WinBotY;
int WinWidth, WinHeight;

// Externs



extern int VideoMode;
extern int WindowMode;
extern WINSCALEXY TopLeftSubWindow;
extern WINSCALEXY ExtentXYSubWindow;

// Window procedure (to run continuously while WinMain is active).
// Only necessary functions are handling keyboard input (for the moment
// at least - cf. DirectInput) and dealing with important system 
// messages, e.g. WM_PAINT.

// Remember to support all the keys you need for your project
// for both KEYUP and KEYDOWN messages!!!

// IMPORTANT!!! The WindowProc is project specific
// by default, since various nifty hacks can always
// be implemented directly via the windows procedure

extern void KeyboardEntryQueue_Add(char c);
extern void IngameKeyboardInput_KeyDown(unsigned char key);
extern void IngameKeyboardInput_KeyUp(unsigned char key);
extern void IngameKeyboardInput_ClearBuffer(void);
extern void Mouse_ButtonUp(unsigned char button);
extern void Mouse_ButtonDown(unsigned char button);

int xPosRelative = 0;
int yPosRelative = 0;
int mouseMoved = 0;


LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC         hdc;
	RECT		NewWindCoord;

 	if (message == RWM_MOUSEWHEEL)
	{
		message = WM_MOUSEWHEEL;
		wParam <<= 16;
	}

	switch (message)
    {
		case WM_MOUSEWHEEL:
		{
			MouseWheelStatus = wParam;
			MouseWheelStatus>>=16;
			return 0;
		}	

		case WM_INPUT: 
		{
			UINT dwSize = 40;
			static BYTE lpb[40];

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

			RAWINPUT* raw = (RAWINPUT*)lpb;

			if (raw->header.dwType == RIM_TYPEMOUSE) 
			{
				mouseMoved = 1;
				xPosRelative = raw->data.mouse.lLastX;
				yPosRelative = raw->data.mouse.lLastY;
			}
			break;
		}
	
		// 21/11/97 DHM: Added processing of WM_CHAR messages:
		case WM_CHAR:
		{
			if (Con_IsActive())
			{
				Con_AddTypedChar((char)wParam);
				break;
			}

			RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR((char)wParam);
			KeyboardEntryQueue_Add((char)wParam);

			break;
		}
		case WM_KEYDOWN:
		{
			RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_KEYDOWN(wParam);
		}
		// it's intentional for this case to fall through to WM_SYSKEYDOWN
		case WM_SYSKEYDOWN:
		{	
/*
			char buf[100];
			sprintf(buf, "key: %x\n", wParam);
			OutputDebugString(buf);
*/

			/* handle left/right alt keys */
			if (wParam == VK_MENU)
			{			
				if (lParam&(1<<24))
					wParam = VK_RMENU;
				else
					wParam = VK_LMENU;
			}

			/* handle left/right control keys */
			if (wParam == VK_CONTROL)
			{			
				if (lParam&(1<<24))
					wParam = VK_RCONTROL;
				else
					wParam = VK_LCONTROL;
			}

			/* handle left/right shift keys */	
			if (wParam == VK_SHIFT)
			{
				if ((GetKeyState(VK_RSHIFT) & 0x8000) && (KeyboardInput[KEY_RIGHTSHIFT] == FALSE))
				{
//					OutputDebugString("caught right shift!\n");
					wParam = VK_RSHIFT;
				}
				else if ((GetKeyState(VK_LSHIFT) & 0x8000) && (KeyboardInput[KEY_LEFTSHIFT] == FALSE))
				{
//					OutputDebugString("caught left shift!\n");
					wParam = VK_LSHIFT;
				}
			}

			IngameKeyboardInput_KeyDown(wParam);
#if 0
			int scancode = (lParam>>16)&255;
			unsigned char vkcode = (wParam&255);

			// ignore the status of caps lock
			//ksarray[VK_CAPITAL] = 0;	
		 	//ksarray[VK_CAPITAL] = GetKeyState(VK_CAPITAL);	
			if (vkcode!=VK_CAPITAL && vkcode!=VK_SCROLL)
			{
			 	#if 0
				WORD output;
			 	if (ToAscii(vkcode,scancode,&ksarray[0],&output,0))
				{
					IngameKeyboardInput_KeyDown((unsigned char)(output));
				}
				#else
				if (ToAsciiTable[vkcode][scancode])
				{
					char buf[100];
					sprintf(buf, "vkcode: %d scancode: %d\n", vkcode ,scancode);
					OutputDebugString(buf);
//					IngameKeyboardInput_KeyDown(ToAsciiTable[vkcode][scancode]);
				}
				#endif
			}
#endif
			// reset caps lock status
			//ksarray[VK_CAPITAL] = GetKeyState(VK_CAPITAL);	
			//ToAscii(wParam&255,scancode,&ksarray[0],&output,0);
			//return 0;
			break;
		}

		case WM_SYSKEYUP:
		case WM_KEYUP:						
		{
/*
			// Handle on screen keyboard input
			if (Osk_IsActive())
			{
				switch (wParam)
				{
					case VK_LEFT:
						Osk_MoveLeft();
						break;
					case VK_RIGHT:
						Osk_MoveRight();
						break;
					case VK_UP:
						Osk_MoveUp();
						break;
					case VK_DOWN:
						Osk_MoveDown();
						break;
					case VK_RETURN: // handle this as if it's a WM_CHAR
						//char key = Osk_GetSelectedKeyChar();
						//RE_ENTRANT_QUEUE_WinProc_AddMessage_WM_CHAR(key);
						//KeyboardEntryQueue_Add(key);
						break;
				}
			}
*/
			/* handle left/right alt keys */
			if (wParam == VK_MENU)
			{	
				if (lParam&(1<<24))
					wParam = VK_RMENU;
				else
					wParam = VK_LMENU;
			}

			/* handle left/right control keys */
			if (wParam == VK_CONTROL)
			{			
				if (lParam&(1<<24))
					wParam = VK_RCONTROL;
				else
					wParam = VK_LCONTROL;
			}

			/* handle left/right shift keys */	
			if (wParam == VK_SHIFT)
			{
				if ((!(GetKeyState(VK_RSHIFT) & 0x8000)) && (KeyboardInput[KEY_RIGHTSHIFT] == TRUE))
				{
					wParam = VK_RSHIFT;
				}
				else if ((!(GetKeyState(VK_LSHIFT) & 0x8000)) && (KeyboardInput[KEY_LEFTSHIFT] == TRUE))
				{
					wParam = VK_LSHIFT;
				}
			}

			IngameKeyboardInput_KeyUp(wParam);
#if 0
			int scancode = (lParam>>16)&255;
			unsigned char vkcode = (wParam&255);

			
			// ignore the status of caps lock
			//ksarray[VK_CAPITAL] = 0;	
//MakeToAsciiTable();			
		  	//ksarray[VK_CAPITAL] = GetKeyState(VK_CAPITAL);	
			if (vkcode!=VK_CAPITAL && vkcode!=VK_SCROLL)
			{
				#if 0
			 	WORD output;
			 	unsigned char z = ToAscii(vkcode,scancode,&ksarray[0],&output,0);
				unsigned char a = (unsigned char)output;
				unsigned char b = ToAsciiTable[vkcode][scancode];
				#endif
				#if 0
				WORD output;
			 	if (ToAscii(vkcode,scancode,&ksarray[0],&output,0))
				{
					IngameKeyboardInput_KeyUp((unsigned char)(output));
				}
				#else
				if (ToAsciiTable[vkcode][scancode])
				{
					IngameKeyboardInput_KeyUp(ToAsciiTable[vkcode][scancode]);
				}
				#endif
			}
			// reset caps lock status
			//ksarray[VK_CAPITAL] = GetKeyState(VK_CAPITAL);	
			//ToAscii(wParam&255,scancode,&ksarray[0],&output,0);
#endif
			//return 0;
			break;
		}
		case WM_LBUTTONDOWN:
		{
			Mouse_ButtonDown(LeftMouse);
			break;
		}
        case WM_LBUTTONUP:
        {
			Mouse_ButtonUp(LeftMouse);
			break;
        }
		case WM_RBUTTONDOWN:
		{
			Mouse_ButtonDown(RightMouse);
			break;
		}
        case WM_RBUTTONUP:
        {
			Mouse_ButtonUp(RightMouse);
            break;
        }
		case WM_MBUTTONDOWN:
		{
			Mouse_ButtonDown(MiddleMouse);
			break;
		}
        case WM_MBUTTONUP:
        {
			Mouse_ButtonUp(MiddleMouse);
            break;
        }
		case WM_XBUTTONDOWN:
        {
			Mouse_ButtonDown(GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? ExtraMouse1 : ExtraMouse2);
            break;
        }
		case WM_XBUTTONUP:
        {
			Mouse_ButtonUp(GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? ExtraMouse1 : ExtraMouse2);
            break;
        }

	 // This, in combination with code in win_func,
	 // will hopefully disable Alt-Tabbing...
		case WM_ACTIVATEAPP:
		{
			bActive = (BOOL) wParam;

			//LOGDXFMT(("WM_ACTIVATEAPP msg: bActive = %d",(int)bActive));
			if (bActive)
			{
        		// need to restore all surfaces - do the special ones first
				/* BJD ALT TAB TEXTURE RESTORE STUFF HERE
 				RESTORE_SURFACE(lpDDSPrimary)
        		RESTORE_SURFACE(lpDDSBack)
        		RESTORE_SURFACE(lpZBuffer)
        		// dodgy, this is meant to be graphic, so it'll really need to be reloaded
        		RESTORE_SURFACE(lpDDBackdrop)
        		// now do all the graphics surfaces and textures, etc.
				*/
        		ATOnAppReactivate();
			}
			IngameKeyboardInput_ClearBuffer();
			return 0;
		}

     // Three below are for safety, to turn off
	 // as much as possible of the more annoying 
	 // functionality of the default Windows 
	 // procedure handler

		case WM_ACTIVATE:
			return 0;
		case WM_CREATE:
			break;

		case WM_MOVE:
		{
			// Necessary to stop it crashing in 640x480
			// FullScreen modes on window initialisation
			if (WindowMode == WindowModeSubWindow)
			{
				GetWindowRect(hWndMain, &NewWindCoord);
				WinLeftX = NewWindCoord.left;
				WinTopY = NewWindCoord.top;
				WinRightX = NewWindCoord.right;
				WinBotY = NewWindCoord.bottom;
			}
			break;
		}

		case WM_SIZE:
		{
			// Necessary to stop it crashing in 640x480
			// FullScreen modes on window initialisation
			if (WindowMode == WindowModeSubWindow)
			{
				GetWindowRect(hWndMain, &NewWindCoord);
				WinLeftX = NewWindCoord.left;
				WinTopY = NewWindCoord.top;
				WinRightX = NewWindCoord.right;
				WinBotY = NewWindCoord.bottom;
			}
			break;
		}
		case WM_SETCURSOR:
			SetCursor(NULL);
			return TRUE;

		case WM_ERASEBKGND:
			return TRUE;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			return TRUE;

		/* Patrick 11/6/97: this to detects the end of a cdda track */
		case MM_MCINOTIFY:
			PlatCDDAManagementCallBack(wParam, lParam);
			break;

		case WM_DESTROY:
		{
			// Calls ReleaseDirect3D DIRECTLY,
			// so as to avoid calling ExitSystem and exiting the
			// Windows system inside the windows procedure
			// IMPORTANT!!! For this to work, release functions 
			// must be re-entrant.  Since this may be causing
			// problems under obscure cirumstances, I am removing 
			// this now (25/7/96).
			// And putting it back... (20/9/96)
			ReleaseDirect3D();

	 		/* patrick 9/6/97: hmmmmm.... */  	
	   		PlatEndSoundSys();

	   		PostQuitMessage(0);
			break;
		}
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

/*
	Persuade Win95 to give us the window we want and, like,
	TOTAL CONTROL, and then shut up, stop whinging and
	go away.
	Or at least as much control as we can get.. safely...
	elegantly... ummm...
*/

// IMPORTANT!!! Windows initialisation is project specific,
// because of the project name and title if nothing else

// This function now takes a mode which is
// set to full or change.  Full should be
// run ONLY when the system is starting.  
// Change is used to change the window 
// characteristics during a run, e.g. to
// change from SubWindow to FullScreen
// mode, and will not attempt to register
// the windows class.

BOOL InitialiseWindowsSystem(HINSTANCE hInstance, int nCmdShow, int WinInitMode)
{ 
	WNDCLASSEX	wcex;
	BOOL		rc;
	RECT		clientRect;

	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_DBLCLKS;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0; 
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	wcex.lpszMenuName = NAME;
	wcex.lpszClassName = NAME;

	rc = RegisterClassEx(&wcex);

	if (!rc)
		return FALSE;


//	MakeToAsciiTable();
/*
	Set up the width and height we want from
	the VideoMode, taking account of WindowMode.
*/

// This has now been modified to just set the
// size to the current system metrics, which
// may or may not be ideal.  Surprisingly, it
// seems not to make much difference.
	
	/* if "-w" passed to command line */
	if (WindowMode == WindowModeSubWindow)
	{
		//force window to be 640x480 to avoid stretch blits.
		WinWidth = 640;
		WinHeight = 480;

		clientRect.left = 0;
		clientRect.top = 0;
		clientRect.right = WinWidth;
		clientRect.bottom = WinHeight;

		hWndMain = CreateWindowEx(
			WS_EX_TOPMOST,
			NAME, //  Name of class (registered by RegisterClass call above) 
			TITLE, // Name of window 
			WS_OVERLAPPEDWINDOW,
			clientRect.left,
			clientRect.top,
			clientRect.right,
			clientRect.bottom,
			NULL,
			NULL,
			hInstance,
			NULL
		);

		AdjustWindowRect(&clientRect, GetWindowLong( hWndMain, GWL_STYLE ), FALSE );
		SetWindowPos(hWndMain, 0, 0, 0, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_SHOWWINDOW);
	}
	else if (WindowMode == WindowModeFullScreen)
	{
		hWndMain = CreateWindowEx(
			WS_EX_TOPMOST,
			NAME, //  Name of class (registered by RegisterClass call above) 
			TITLE, // Name of window 
			WS_POPUP,
			0,
			0,
			GetSystemMetrics(SM_CXSCREEN),
			GetSystemMetrics(SM_CYSCREEN),
			NULL,
			NULL,
			hInstance,
			0
		);
	}

    if (!hWndMain)
	{
		OutputDebugString("!hWndMain\n");
		return FALSE;
	}

// Experiment only!!!!

	// Set the window up to be displayed 
	ShowWindow(hWndMain, nCmdShow);
	// Update once (i.e. send WM_PAINT message to the window procedure)
	UpdateWindow(hWndMain);

// Grab ALL mouse messages for our window.
// Note this will only work if the window is
// foreground (as it is... ).  This ensures that
// we will still get MOUSEMOVE etc messages even
// if the mouse is out of the defined window area.

    #if grabmousecapture
    SetCapture(hWndMain);
// Load null cursor shape
	SetCursor(NULL);
	#endif

    return TRUE;
}

void ChangeWindowsSize(int width, int height)
{
	RECT testRect;
	RECT newWindowSize;

	newWindowSize.top = 0;
	newWindowSize.left = 0;
	newWindowSize.right = width;
	newWindowSize.bottom = height;

	AdjustWindowRect(&newWindowSize, GetWindowLong( hWndMain, GWL_STYLE ), FALSE );

	if (SetWindowPos(hWndMain, 0, 0, 0, newWindowSize.right - newWindowSize.left, newWindowSize.bottom - newWindowSize.top, SWP_SHOWWINDOW) == 0)
	{	
		OutputDebugString("SetWindowPos failed\n");
	}

	GetClientRect(hWndMain, &testRect);
}

void InitialiseRawInput()
{
	#ifndef HID_USAGE_PAGE_GENERIC
		#define HID_USAGE_PAGE_GENERIC         ((USHORT) 0x01)
    #endif
    #ifndef HID_USAGE_GENERIC_MOUSE
		#define HID_USAGE_GENERIC_MOUSE        ((USHORT) 0x02)
    #endif

    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
    Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
    Rid[0].dwFlags = RIDEV_INPUTSINK;   
    Rid[0].hwndTarget = hWndMain;
    RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
}

// Project specific to go with the initialiser

BOOL ExitWindowsSystem(void)
{
   BOOL rc = TRUE;

   // Release dedicated mouse capture
   #if grabmousecapture
   ReleaseCapture();
   #endif

   rc = DestroyWindow(hWndMain);

   return rc;
}

void MakeToAsciiTable(void)
{
	WORD output;
	for (int k=0; k<=255; k++)
	{
		ksarray[k]=0;
	}

	for (int i=0; i<=255; i++)
	{
		for (int s=0; s<=255; s++)
		{
			if(ToAscii(i,s,&ksarray[0],&output,0)!=0)
			{
				ToAsciiTable[i][s] = (unsigned char)output;
			}
			else 
			{
				ToAsciiTable[i][s] = 0;
			}
		}
	}
}
// End of extern C declaration 

};
#endif //ifdef WIN32