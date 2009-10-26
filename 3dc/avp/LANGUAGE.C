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

#define UseLocalAssert TRUE
#include "ourasert.h"
#include "avp_menus.h"

extern HWND hWndMain;

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
	char *filenamePtr;

	int i;

	/* language select here! */
	GLOBALASSERT(AvP.Language>=0);
	GLOBALASSERT(AvP.Language<I_MAX_NO_OF_LANGUAGES);
	
	#if MARINE_DEMO
	filenamePtr = "menglish.txt";
//	TextBufferPtr = LoadTextFile("menglish.txt");
	#elif ALIEN_DEMO
	filenamePtr = "aenglish.txt";
//	TextBufferPtr = LoadTextFile("aenglish.txt");
	#elif USE_LANGUAGE_TXT
	filenamePtr = "language.txt";
	#else
	filenamePtr = LanguageFilename[AvP.Language];
//	TextBufferPtr = LoadTextFile(LanguageFilename[AvP.Language]);
	#endif

	TextBufferPtr = LoadTextFile(filenamePtr);

	if (TextBufferPtr == NULL) 
	{
		/* have to quit if this file isn't available */
		char message[100];
		sprintf(message,"Unable to load language file: %s\n",filenamePtr);

#ifdef WIN32
		MessageBox(hWndMain, message, "AvP Error", MB_OK/*+MB_SYSTEMMODAL*/);
		exit(1);
#else

#endif
		
	}

	if (!strncmp (TextBufferPtr, "REBCRIF1", 8))
	{
		textPtr = (char*)HuffmanDecompress((HuffmanPackage*)(TextBufferPtr)); 		
		DeallocateMem(TextBufferPtr);
		TextBufferPtr = textPtr;
	}
	else
	{
		textPtr = TextBufferPtr;
	}

	/* check if language file is not from Gold version */
	if (strncmp (textPtr, "REBINFF2", 8))
	{
		char message[100];
		sprintf(message,"File %s is not compatible with Gold Edition\n",filenamePtr);
#ifdef WIN32
		MessageBox(hWndMain, message, "AvP Error", MB_OK+MB_SYSTEMMODAL);
#endif
		exit(1);
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