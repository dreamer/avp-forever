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

// DHM 12 Nov 97: hooks for C++ string handling code:
#include "string_table.hpp"

#define UseLocalAssert TRUE
#include "ourasert.h"
#include "menus.h"

void ChangeWindowsSize(uint32_t width, uint32_t height);

static char EmptyString[]="";

static char *TextStringPtr[MAX_NO_OF_TEXTSTRINGS]={EmptyString};
static char *TextBufferPtr;

// moved here from langplat.c
char *LanguageFilename[] = 
{
	ENGLISH_TEXT_FILENAME,	/* I_English */ 
	ENGLISH_TEXT_FILENAME,	/* I_French  */
	ENGLISH_TEXT_FILENAME,	/* I_Spanish */ 
	ENGLISH_TEXT_FILENAME,	/* I_German  */
	ENGLISH_TEXT_FILENAME,	/* I_Italian */
	ENGLISH_TEXT_FILENAME,	/* I_Swedish */
}; 

void InitTextStrings(void)
{
	char *textPtr;
	char *filenamePtr;

	GLOBALASSERT(AvP.Language >= 0);
	GLOBALASSERT(AvP.Language < I_MAX_NO_OF_LANGUAGES);
	
	#if MARINE_DEMO
	filenamePtr = "menglish.txt";
	#elif ALIEN_DEMO
	filenamePtr = "aenglish.txt";
	#elif PREDATOR_DEMO
	filenamePtr = "english.txt";
	#else
	filenamePtr = "language.txt";
	#endif

	TextBufferPtr = LoadTextFile(filenamePtr);

	if (TextBufferPtr == NULL) 
	{
		// have to quit if this file isn't available
		char message[100];
		sprintf(message, "Unable to load language file: %s\n", filenamePtr);

		ChangeWindowsSize(1, 1);
		avp_MessageBox(message, MB_OK);
		avp_exit(1);
		return;
	}

	if (!strncmp(TextBufferPtr, "REBCRIF1", 8))
	{
		textPtr = (char*)HuffmanDecompress((HuffmanPackage*)(TextBufferPtr));
		DeallocateMem(TextBufferPtr);
		TextBufferPtr = textPtr;
	}
	else
	{
		textPtr = TextBufferPtr;
	}

	// check if language file is not from Gold version
	if (strncmp(textPtr, "REBINFF2", 8))
	{
		char message[100];
		sprintf(message,"File %s is not compatible with Gold Edition\n",filenamePtr);

		ChangeWindowsSize(1, 1);
		avp_MessageBox(message, MB_OK+MB_SYSTEMMODAL);
		avp_exit(1);
	}

	AddToTable(EmptyString);

	for (int i = 1; i < MAX_NO_OF_TEXTSTRINGS; i++)
	{
		// scan for a quote mark
		while (*textPtr++ != '"');

		// now pointing to a text string after quote mark
		TextStringPtr[i] = textPtr;

		// scan for a quote mark
		while (*textPtr != '"')
		{
			textPtr++;
		}

		// change quote mark to zero terminator
		*textPtr = 0;

		AddToTable(TextStringPtr[i]);
	}
}

void KillTextStrings(void)
{
	UnloadTextFile(LanguageFilename[AvP.Language], TextBufferPtr);

	UnloadTable();
}

char *GetTextString(enum TEXTSTRING_ID stringID)
{
	LOCALASSERT(stringID < MAX_NO_OF_TEXTSTRINGS);

	return TextStringPtr[stringID];
}
