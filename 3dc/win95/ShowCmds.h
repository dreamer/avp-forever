#ifndef _included_showcmds_h_ /* Is this your first time? */
#define _included_showcmds_h_ 1

/* KJL 14:42:10 29/03/98 - ShowCmds.h

	defines the variables that describe the status of the debugging text 


	eg. if ShowDebuggingText.FPS == 1, then the frame-rate show be displayed.

*/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct DEBUGGINGTEXTOPTIONS
{
	unsigned int FPS :1;
	unsigned int Environment :1;
	unsigned int Coords :1;
	unsigned int Module :1;
	unsigned int Target :1;

	unsigned int Networking: 1;
	unsigned int Dynamics :1;
	unsigned int GunPos :1;
	unsigned int Tears :1;
	unsigned int PolyCount :1;
	unsigned int Sounds :1;
};

extern struct DEBUGGINGTEXTOPTIONS ShowDebuggingText;

#ifdef AVP_DEBUG_VERSION
	#define DEBUGGING_TEXT_ON 1
#else
	#define DEBUGGING_TEXT_ON 0
#endif

#if 0
#if DEBUGGING_TEXT_ON
extern int PrintDebuggingText(const char* t, ...);
#else
#define PrintDebuggingText(ignore)
#endif
#endif

extern int ReleasePrintDebuggingText(const char* t, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif