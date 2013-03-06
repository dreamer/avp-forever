
// only for windows
#ifdef WIN32
#include <windows.h>
#include <process.h>

extern bool bActive;
extern int WinLeftX, WinRightX, WinTopY, WinBotY;

static HANDLE mouseThreadHandle = 0;
static volatile bool EndMouseThread = false;
static bool threadInited = false;

// thread continually moves the mouse cursor to the centre of the window
// so you don't accidently click outside it.
unsigned __stdcall MouseThread(void*)
{
	while (!EndMouseThread)
	{
		::Sleep(10);

		if (!bActive) { 
			continue;
		}

		::SetCursorPos((WinLeftX+WinRightX)>>1,(WinTopY+WinBotY)>>1);
	}
	EndMouseThread = false;
	_endthreadex(0);
	return 0;
}

void InitCentreMouseThread()
{
	mouseThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, &MouseThread, NULL, 0, NULL));
	threadInited = true;
}

void FinishCentreMouseThread()
{
	EndMouseThread = true;
	if (threadInited) {
		::WaitForSingleObject(mouseThreadHandle, INFINITE);
		::CloseHandle(mouseThreadHandle);
		threadInited = false;
	}
	mouseThreadHandle = 0;
}

#endif
