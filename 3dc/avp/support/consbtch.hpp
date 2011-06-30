/*
	
	consbtch.hpp

	Console batch file support

*/

#ifndef _consbtch
#define _consbtch 1

	#if ( defined( __WATCOMC__ ) || defined( _MSC_VER ) )
		#pragma once
	#endif

	#ifndef _scstring
		#include "scstring.hpp"
	#endif

/* Version settings *****************************************************/

/* Constants  ***********************************************************/

/* Macros ***************************************************************/

/* Type definitions *****************************************************/
	class BatchFileProcessing
	{
	public:
		static bool Run(char* Filename);
			// Tries to find the file, if it finds it it reads it,
			// adds the non-comment lines to the pending list, and returns TRUE
			// If it can't find the file, it returns FALSE

	public:
		static int bEcho;
	};

/* Exported globals *****************************************************/

/* Function prototypes **************************************************/



/* End of the header ****************************************************/

#endif
