
// only for windows
#ifdef WIN32
#include <windows.h>
#include <process.h>

extern BOOL bActive;
extern int WinLeftX, WinRightX, WinTopY, WinBotY;

static volatile int EndMouseThread = 0;

// thread continually moves the mouse cursor to the centre of the window
// so you don't accidently click outside it.
void MouseThread(void*)
{
	while (!EndMouseThread)
	{
		Sleep(10);

		if (!bActive) 
			continue;

		SetCursorPos((WinLeftX+WinRightX)>>1,(WinTopY+WinBotY)>>1);
	}
	EndMouseThread = 0;
}

void InitCentreMouseThread()
{
	_beginthread(MouseThread,10000,0);
}


void FinishCentreMouseThread()
{
	EndMouseThread=1;
}

#endif
