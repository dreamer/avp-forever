/*******************************************************************
 *
 *    DESCRIPTION: 	strtab.cpp
 *
 *    AUTHOR: David Malcolm
 *
 *    HISTORY:  Created 12 Nov 97
 *
 *******************************************************************/

/* Includes ********************************************************/
#include "3dc.h"
#include "strtab.hpp"

	#define UseLocalAssert TRUE
	#include "ourasert.h"

/* Version settings ************************************************/

/* Constants *******************************************************/

/* Macros **********************************************************/

/* Imported function prototypes ************************************/

/* Imported data ***************************************************/

/* Exported globals ************************************************/
#if OnlyOneStringTable
	/*static*/ SCString* StringTable :: pSCString[ MAX_NO_OF_TEXTSTRINGS ];
	/*static*/ unsigned int StringTable :: NumStrings = 0;
#endif

/* Internal type definitions ***************************************/

/* Internal function prototypes ************************************/

/* Internal globals ************************************************/

/* Exported function definitions ***********************************/
// class StringTable
// public:
/*STRINGTABLE_DECL_SPECIFIER*/ SCString& StringTable :: GetSCString
(
	enum TEXTSTRING_ID stringID
)
{
	/* PRECONDITION */
	{
		GLOBALASSERT( stringID < MAX_NO_OF_TEXTSTRINGS );
		GLOBALASSERT( pSCString[ stringID ] );
	}

	/* CODE */
	{
		pSCString[ stringID ] -> R_AddRef();
		return *( pSCString[ stringID ] );
	}
}


/*STRINGTABLE_DECL_SPECIFIER*/ void StringTable :: Add( ProjChar* pProjChar )
{
	/* PRECONDITION */
	{
		GLOBALASSERT( NumStrings < MAX_NO_OF_TEXTSTRINGS );
		GLOBALASSERT( pSCString[ NumStrings ] == 0 );
	}

	/* CODE */
	{
		pSCString[ NumStrings++ ] = new SCString( pProjChar );
	}
}

#if OnlyOneStringTable
/*static*/ void StringTable :: Unload(void)
{
	/* PRECONDITION */
	{
	}

	/* CODE */
	{
		while ( NumStrings )
		{
			pSCString[ --NumStrings ] -> R_Release();
		}
	}
}
#else
StringTable :: StringTable()
{
	NumStrings = 0;
}

StringTable :: ~StringTable()
{
	while ( NumStrings )
	{
		pSCString[ --NumStrings ] -> R_Release();
	}
}
#endif


// private:

// C-callable functions:
#if OnlyOneStringTable
extern void AddToTable( ProjChar* pProjChar )
{
	/* PRECONDITION */
	{
		GLOBALASSERT( pProjChar );
	}

	/* CODE */
	{
		StringTable :: Add( pProjChar );
	}
}

extern void UnloadTable(void)
{
	/* PRECONDITION */
	{
	}

	/* CODE */
	{
		StringTable :: Unload();
	}
}
#endif



/* Internal function definitions ***********************************/
