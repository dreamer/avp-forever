/*
	
	strutil.h

	Created 13/11/97 by David Malcolm: more carefully specified
	versions of the C string library functions, to act on ProjChars
	rather than chars

*/

#ifndef _strutil
#define _strutil 1

	#ifndef _projtext
	#include "projtext.h"
	#endif

/* Version settings *****************************************************/

/* Constants  ***********************************************************/

/* Macros ***************************************************************/

/* Type definitions *****************************************************/

/* Exported globals *****************************************************/

/* Function prototypes **************************************************/
	/* String manipulation **********************************************/
		extern void STRUTIL_SC_WriteTerminator
		(
			ProjChar* pProjCh
		);

		extern bool STRUTIL_SC_fIsTerminator
		(
			const ProjChar* pProjCh
		);

	/* Emulation of <string.h> *******************************************/
		extern size_t STRUTIL_SC_Strlen
		(
			const ProjChar* pProjCh_In
		);

		extern ProjChar* STRUTIL_SC_StrCpy
		(
			ProjChar* pProjCh_Dst,
			const ProjChar* pProjCh_Src
		);

		extern void STRUTIL_SC_FastCat
		(
			ProjChar* pProjCh_Dst,
			const ProjChar* pProjCh_Src_0,
			const ProjChar* pProjCh_Src_1			
		);
			/* This function assumes the destination area is large enough;
			it copies Src0 followed by Src1 to the dest area.
			*/

		extern bool STRUTIL_SC_Strequal
		(
			const ProjChar* pProjCh_1,
			const ProjChar* pProjCh_2
		);

		extern bool STRUTIL_SC_Strequal_Insensitive
		(
			const ProjChar* pProjCh_1,
			const ProjChar* pProjCh_2
		);
		/*
			these functions copy at most MaxSize chars from Src to Dst; this INCLUDES
			space used by a NULL terminator (so that you can pass it an array
			for the destination together with its size.  The return value is
			whether the	entire string was copied.
		*/
		extern bool STRUTIL_SC_SafeCopy
		(
			ProjChar* pProjCh_Dst,
			size_t MaxSize,

			const ProjChar* pProjCh_Src
		);

		extern void STRUTIL_SC_SafeCat
		(
			ProjChar* pProjCh_Dst,
			size_t MaxSize,

			const ProjChar* pProjCh_Add
		);
		
		extern size_t STRUTIL_SC_NumBytes
		(
			const ProjChar* pProjCh
		);



/* End of the header ****************************************************/

#endif
