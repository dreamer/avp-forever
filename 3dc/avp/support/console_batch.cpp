/*******************************************************************
 *
 *    DESCRIPTION: 	consbtch.cpp
 *
 *    AUTHOR: David Malcolm
 *
 *    HISTORY:  Created 8/4/98
 *
 *******************************************************************/

/* Includes ********************************************************/
#include "3dc.h"

	#include "console_batch.hpp"
	#include "reflist.hpp"

	#define UseLocalAssert TRUE
	#include "ourasert.h"

/* Version settings ************************************************/

/* Constants *******************************************************/

	enum
	{
		MaxBatchFileLineLength=300,
		MaxBatchFileLineSize=(MaxBatchFileLineLength+1)
	};

/* Macros **********************************************************/

/* Imported function prototypes ************************************/

/* Imported data ***************************************************/

/* Exported globals ************************************************/

/* Internal type definitions ***************************************/

/* Internal function prototypes ************************************/

/* Internal globals ************************************************/

/* Exported function definitions ***********************************/
// class BatchFileProcessing
// public:
// static
bool
BatchFileProcessing :: Run(char* Filename)
{
	// Tries to find the file, if it finds it it reads it,
	// adds the non-comment lines to the pending list, and returns TRUE
	// If it can't find the file, it returns FALSE

	// LOCALISEME
	// This code makes several uses of the assumption that char is type-equal
	// to ProjChar

	RefList<SCString> PendingList;

	{
		FILE* pFile = avp_fopen(Filename,"r");

		if (NULL==pFile)
		{
			return FALSE;
		}
		
		

		// Read the file, line by line.  
		{
			// We impose a maximum length on lines that will be valid:
			char LineBuffer[MaxBatchFileLineSize];

			int CharsReadInLine = 0;

			while (1)
			{
				int Char = fgetc(pFile);

				if (Char==EOF)
				{
					break;
				}
				else
				{
					if
					(
						Char=='\n'
					)
					{
						// Flush the buffer into the pending queue:
						GLOBALASSERT(CharsReadInLine<=MaxBatchFileLineLength);
						LineBuffer[CharsReadInLine] = '\0';

						SCString* pSCString_Line = new SCString(&LineBuffer[0]);
						
						PendingList . AddToEnd
						(
							*pSCString_Line
						);

						pSCString_Line -> R_Release();

						CharsReadInLine = 0;
					}
					else
					{
						// Add to buffer; silently reject characters beyond the length limit
						if ( CharsReadInLine < MaxBatchFileLineLength )
						{
							LineBuffer[CharsReadInLine++]=toupper((char)Char);
						}
					}
				}
			}

			// Flush anything still in the buffer into the pending queue:
			{
				GLOBALASSERT(CharsReadInLine<=MaxBatchFileLineLength);
				LineBuffer[CharsReadInLine] = '\0';
				
				SCString* pSCString_Line = new SCString(&LineBuffer[0]);
				
				PendingList . AddToEnd
				(
					*pSCString_Line
				);

				pSCString_Line -> R_Release();
			}
		}

		fclose(pFile);
	}

	// Feedback:
	{
		SCString* pSCString_1 = new SCString("EXECUTING BATCH FILE ");
			// LOCALISEME
		SCString* pSCString_2 = new SCString(Filename);
		SCString* pSCString_Feedback = new SCString
		(
			pSCString_1,
			pSCString_2
		);

		pSCString_Feedback -> SendToScreen();

		pSCString_Feedback -> R_Release();
		pSCString_2 -> R_Release();
		pSCString_1 -> R_Release();

	}

	// Now process the pending queue:
	{
		// Iterate through the pending list, destructively reading the
		// "references" from the front:
		{
			SCString* pSCString;

			// The assignment in this boolean expression is deliberate:
			while
			(
				NULL != (pSCString = PendingList . GetYourFirst())
			)
			{
				if (pSCString->pProjCh()[0] != '#')
				{
					// lines beginning with hash are comments
					if (bEcho)
					{
						pSCString -> SendToScreen();
					}

					pSCString -> ProcessAnyCheatCodes();
				}
				pSCString -> R_Release();
			}
		}
	}

	return TRUE;
}

// public:
// static
int BatchFileProcessing :: bEcho = FALSE;


/* Internal function definitions ***********************************/
