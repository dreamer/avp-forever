
#ifdef WIN32 // windows only
/****

Windows functionality that is definitely
not project specific.

****/

#include "3dc.h"

// Externs

extern BOOL bActive;
extern int16_t MouseWheelStatus;

void CheckForWindowsMessages(void)
{
	MSG msg;
	
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
		}
		
		// JH 13/2/98 - if the app is not active we should not return from the message lopp
		// until the app is re-activated
		if (!bActive)
		{
			ResetFrameCounter();
			Sleep(0);
		}
	}
	while (!bActive);
}

#endif // ifdef WIN32
