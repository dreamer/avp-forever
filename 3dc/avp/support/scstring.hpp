/*
	
	scstring.hpp

*/

#ifndef _scstring
#define _scstring 1

	#define SupportHHStuff	FALSE

	#ifndef _refobj
	#include "refobj.hpp"
	#endif

	#ifndef _ourbool
	#include "ourbool.h"
	#endif

	#if SupportHHStuff
		#ifndef HHSTRING
		#include "hhstring.h"
		#endif

		#ifndef FDIPUB_INCLUDED
		#include "fdipub.h"
		#endif
	#else
		#ifndef _projtext
		#include "projtext.h"
		#endif

		#ifndef _projfont
		#include "projfont.h"
		#endif

		#ifndef _r2base
		#include "r2base.h"
		#endif

	#endif

	#ifndef list_template_hpp
	#include "list_tem.hpp"
	#endif

	#define EnableSizeData	TRUE

	class SCString : public RefCountObject
	{
	public:
		ProjChar* pProjCh(void) const;

		SCString
		(
			const ProjChar* pProjCh_Init
		); // you get an automatic reference when you construct it.

		SCString
		(
			signed int Number
		);

		SCString
		(
			unsigned int Number
		);
			// forms a new string object that describes the number passed
			// standard decimal representation

		SCString
		(
			float Number
		);
			// the string is to a standard number of decimal places


		SCString
		(
			ProjChar* pProjCh_Init,
			unsigned int Length
		);
			// Forms a string of length at most Length (with 1 extra for NULL-terminator)

		SCString
		(
			SCString* pStringObj_0,
			SCString* pStringObj_1
		);
			// forms a new string object by concatenating the strings in 0
			// and 1

		SCString
		(
			SCString* pStringObj_0,
			SCString* pStringObj_1,
			SCString* pStringObj_2
		);
			// forms a new string object by concatenating the strings in 0, 1 and 2

		SCString
		(
			SCString* pStringObj_0,
			SCString* pStringObj_1,
			SCString* pStringObj_2,
			SCString* pStringObj_3
		);
			// forms a new string object by concatenating the strings;
			// implementation is _slow_

		SCString
		(
			SCString* pStringObj_0,
			SCString* pStringObj_1,
			SCString* pStringObj_2,
			SCString* pStringObj_3,
			SCString* pStringObj_4
		);
			// forms a new string object by concatenating the strings;
			// implementation is _slow_

		SCString
		(
			List<ProjChar> List_ProjChar
		);

		#if SupportHHStuff
		void GetSizeOfSingleLineGivenStandardFont
		(
			FDIBMAP* pFDIBMap_Out
		);
		FDIBMAP GetSizeOfSingleLineGivenStandardFont(void);
		int GetWidthOfSingleLineGivenStandardFont(void);

		FDIQUAD MinFDIQuadForSCStringAtPos
		(
			FDIPOS FDIPos
		);
		#else
		r2size CalcSize
		(
			FontIndex I_Font
		);
			// Size for this string drawn on a single line in the specified font
			// Behaviour is not fully defined when unprintable characters occur in the string

		OurBool bCanRenderFully( FontIndex I_Font );
			// can this string be fully rendered by the font (not all fonts support
			// all characters) ?

		static void UpdateAfterFontChange( FontIndex I_Font_Changed );
			// called by the font code whenever fonts are loaded/unloaded
		#endif

		unsigned int GetNumChars(void);

		void ProcessAnyCheatCodes(void);

		void SendToScreen(void);
			// adds this as a new on-screen message

		static List<SCString*> Parse
		(
			ProjChar* pProjChar_In
		);
			// takes a string and builds a list of new SCStrings, in which
			// each string in the list consists of non-whitespace characters from
			// the input, and the whitespace is used to separate individual
			// strings

		#if TrackReferenceCounted
		void DumpIDForReferenceDump(R_DumpContext& theContext) const;
		#endif

	private:
		~SCString();
			// private destructor; only called when reference count hits zero

		ProjChar* pProjCh_Val;
			// this is always "owned" by the String execpt when
			// fAllocFailure is set

		int NumberOfCharacters;
			// doesn't include NULL terminator

		#if SupportHHStuff
		int WidthGivenStandardFont;
			// width in pixels, to save constant recalc
		#else
			#if EnableSizeData
			r2size R2Size[ IndexedFonts_MAX_NUMBER_OF_FONTS ];
			#endif

		OurBool bCanRender[ IndexedFonts_MAX_NUMBER_OF_FONTS ];			
		#endif

		size_t AllocatedSize;
			// this includes the NULL terminator

		// Maintain an intrusive doubly-linked list
		// so that you can find all the strings when a font is loaded/unloaded
		// and hence update their data
		// Strings are added to the front of the list upon construction
		SCString* pNxt;
		SCString* pPrv;
		static SCString* pFirst;
	}; // Naming: "StringObj"

	// Inline methods:
	#if !SupportHHStuff
		#if EnableSizeData
		inline r2size SCString::CalcSize
		(
			FontIndex I_Font
		)
		{
			return R2Size[ I_Font ];
		}
		#endif

		inline OurBool SCString::bCanRenderFully( FontIndex I_Font )
		{
			return bCanRender[ I_Font ];
		}
	#endif // !SupportHHStuff		

		inline unsigned int SCString::GetNumChars(void)
		{
			return NumberOfCharacters;
		}

	#if SupportHHStuff
	extern SCString* HHSTRING_GetSCString
	(
		HHStringTable* pHHST,
		unsigned int StrNum
	);
		// The result is guaranteed to be non-NULL if you pick a valid
		// StrNum.

		// Remember to call R_AddRef() if you use the result
		// NO LONGER THE CASE!!!!
	#endif


#ifdef __cplusplus
	extern "C" {
#endif

/* Version settings *****************************************************/

/* Macros ***************************************************************/

/* Type definitions *****************************************************/

/* Exported globals *****************************************************/

/* Function prototypes **************************************************/



/* End of the header ****************************************************/


#ifdef __cplusplus
	};
#endif

#endif


