/*
	
	iofocus.h

	Created 21/11/97 by DHM: is input focus set to normal controls,
	or to typing?
*/

#ifndef _iofocus
#define _iofocus 1

	#ifndef _gadget
	#include "gadget.h"
	#endif

/* Version settings *****************************************************/

/* Constants  ***********************************************************/

/* Macros ***************************************************************/

/* Type definitions *****************************************************/

/* Exported globals *****************************************************/

/* Function prototypes **************************************************/
	#if UseGadgets
		extern bool IOFOCUS_AcceptControls(void);
		extern bool IOFOCUS_AcceptTyping(void);

		extern void IOFOCUS_Toggle(void);
	#else
		/* Otherwise: the functions collapse to macros: */
		#define IOFOCUS_AcceptControls() (1)
		#define IOFOCUS_AcceptTyping() (0)

		#define IOFOCUS_Toggle() ((void) 0)
	#endif /* UseGadgets */

	// bjd
	#define IOFOCUS_GAME		0x0001
	#define	IOFOCUS_NEWCONSOLE	0x0002
	#define	IOFOCUS_OLDCONSOLE	0x0004
	#define	IOFOCUS_OSK			0x0008

	void IOFOCUS_Set(int focus);
	int IOFOCUS_Get();


/* End of the header ****************************************************/
#endif
