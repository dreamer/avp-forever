/*KJL***************************************
*    Language Internationalization Code    *
***************************************KJL*/
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "gamedef.h"


#include "langenum.h"
#include "language.h"
#include "huffman.hpp"

#if SupportWindows95
	// DHM 12 Nov 97: hooks for C++ string handling code:
	#include "strtab.hpp"
#endif

#define UseLocalAssert Yes
#include "ourasert.h"
#include "avp_menus.h"


#ifdef AVP_DEBUG_VERSION
	// bjd - this'll look for ENGLISH.TXT which doesn't exist in the retail data files
	// so i'll set it to '1' to use language.txt
	#define USE_LANGUAGE_TXT 1
#else
	#define USE_LANGUAGE_TXT 1
#endif

static char EmptyString[]="";

static char *TextStringPtr[MAX_NO_OF_TEXTSTRINGS]={EmptyString};
static char *TextBufferPtr;

void InitTextStrings(void)
{
	char *textPtr;
	int i;

	/* language select here! */
	GLOBALASSERT(AvP.Language>=0);
	GLOBALASSERT(AvP.Language<I_MAX_NO_OF_LANGUAGES);
	
	#if MARINE_DEMO
	TextBufferPtr = LoadTextFile("menglish.txt");
	#elif ALIEN_DEMO
	TextBufferPtr = LoadTextFile("aenglish.txt");
	#elif USE_LANGUAGE_TXT
#ifdef _XBOX
	TextBufferPtr = LoadTextFile("D:\\language.txt"); // xbox
#endif
#ifdef WIN32 
	TextBufferPtr = LoadTextFile("language.txt"); // win32
#endif
	#else
	TextBufferPtr = LoadTextFile(LanguageFilename[AvP.Language]);
	#endif
	
	if(TextBufferPtr == NULL) {
		/* NOTE:
		   if this load fails, then most likely the game is not 
		   installed correctly. 
		   SBF
		  */ 
		OutputDebugString("cant find language.txt\n");
//		fprintf(stderr, "ERROR: unable to load %s language text file\n",
//			 filename);
		exit(1);
	}

	if (!strncmp (TextBufferPtr, "REBCRIF1", 8))
	{
		textPtr = (char*)HuffmanDecompress((HuffmanPackage*)(TextBufferPtr)); 		
		DeallocateMem(TextBufferPtr);
		TextBufferPtr=textPtr;
	}
	else
	{
		textPtr = TextBufferPtr;
	}

	AddToTable( EmptyString );

	for (i=1; i<MAX_NO_OF_TEXTSTRINGS; i++)
	{	
		/* scan for a quote mark */
		while (*textPtr++ != '"');

		/* now pointing to a text string after quote mark*/
		TextStringPtr[i] = textPtr;

		/* scan for a quote mark */
		while (*textPtr != '"')
		{	
			textPtr++;
		}

		/* change quote mark to zero terminator */
		*textPtr = 0;

		AddToTable( TextStringPtr[i] );
	}
}
void KillTextStrings(void)
{
	UnloadTextFile(LanguageFilename[AvP.Language],TextBufferPtr);

	UnloadTable();
}

char *GetTextString(enum TEXTSTRING_ID stringID)
{
	LOCALASSERT(stringID<MAX_NO_OF_TEXTSTRINGS);

	return TextStringPtr[stringID];
}


