
#ifdef WIN32 // windows only
/****

Windows functionality that is definitely
not project specific.

****/

// To link code to main C functions 

extern "C" {

#include "3dc.h"

// For modifications necessary to make Alt-Tabbing
// behaviour (WM_ACTIVATEAPP) work full screen.
// This is necessary to support full screen
// ActiveMovie play.

#define SupportAltTab TRUE

// Externs

extern BOOL bActive;

// These function are here solely to provide a clean
// interface layer, since Win32 include files are fully
// available in both C and C++.
// All functions linking to standard windows code are
// in win_func.cpp or win_proj.cpp, and all DirectX 
// interface functions
// should be in dd_func.cpp (in the Win95 directory)
// or d3_func.cpp, dp_func.cpp, ds_func.cpp etc.
// Project specific platfrom functionality for Win95
// should be in project/win95, in files called 
// dd_proj.cpp etc.


// GetTickCount is the standard windows return
// millisecond time function, which isn't actually
// accurate to a millisecond.  In order to get FRI
// to work properly with GetTickCount at high frame 
// rates, you will have to switch KalmanTimer to TRUE
// at the start of io.c to turn on a filtering algorithm
// in the frame counter handler.  
// Alternately, we can use the mm function 
// timeGetTime to get the time accurate to a millisecond.
// There is still enough variation in this to make
// the kalman filter probably worthwhile, however.

long GetWindowsTickCount(void)
{
	return timeGetTime();
}

// This function is set up using a PeekMessage check,
// with a return on a failure of GetMessage, on the
// grounds that it might be more stable than just
// GetMessage.  But then again, maybe not.  
// PM_NOREMOVE means do not take this message out of
// the queue.  The while loop is designed to ensure
// that all messages are sent through to the Windows
// Procedure are associated with a maximum of one frame's
// delay in the main engine cycle, ensuring that e.g.
// keydown messages do not build up in the queue.

// if necessary, one could extern this flag
// to determine if a task-switch has occurred which might
// have trashed a static display, to decide whether to
// redraw the screen. After doing so, one should reset
// the flag

BOOL g_bMustRedrawScreen = FALSE;

void CheckForWindowsMessages(void)
{
	MSG	msg;
	extern int16_t MouseWheelStatus;
	
	MouseWheelStatus = 0;

	// Initialisation for the current embarassingly primitive mouse 
	// handler...

	do
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
				return;

			TranslateMessage(&msg);
			DispatchMessage(&msg);

			#if (!SupportAltTab)
			// Panic
			if (!bActive)
			{
				// Dubious hack...
				#if 0
				ExitSystem();
				#else
				ReleaseDirect3D();
				exit(0x00);
				#endif
			}
			#endif
		}
		
		// JH 13/2/98 - if the app is not active we should not return from the message lopp
		// until the app is re-activated
		
		if (!bActive)
		{
			ResetFrameCounter();
			Sleep(0);
			g_bMustRedrawScreen = TRUE;
		}
	}
	while (!bActive);
}

// End of extern C declaration 

};

#endif // ifdef WIN32


