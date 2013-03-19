/*
	
	conscmnd.hpp

*/

#ifndef _conscmnd
#define _conscmnd 1

	#if ( defined( __WATCOMC__ ) || defined( _MSC_VER ) )
		#pragma once
	#endif

	#ifndef _conssym_hpp
	#include "console_symbol.hpp"
	#endif

/* Version settings *****************************************************/

/* Constants  ***********************************************************/

/* Macros ***************************************************************/

/* Type definitions *****************************************************/
	class ConsoleCommand : public ConsoleSymbol
	{
	public:
		static void CreateAll(void);

		// Various factory methods:
		static void Make
		(
			ProjChar* pProjCh_ToUse,
			ProjChar* pProjCh_Description_ToUse,
			void (&f) (void),
			bool Cheat = false
		);
		static void Make
		(
			ProjChar* pProjCh_ToUse,
			ProjChar* pProjCh_Description_ToUse,
			void (&f) (int),
			bool Cheat = false
		);
		static void Make
		(
			ProjChar* pProjCh_ToUse,
			ProjChar* pProjCh_Description_ToUse,
			int (&f) (void),
			bool Cheat = false
		);
		static void Make
		(
			ProjChar* pProjCh_ToUse,
			ProjChar* pProjCh_Description_ToUse,
			int (&f) (int),
			bool Cheat = false
		);
		static void Make
		(
			ProjChar* pProjCh_ToUse,
			ProjChar* pProjCh_Description_ToUse,
			void (&f) (char*),
			bool Cheat = false
		);

		static bool Process( ProjChar* pProjCh_In );
			// used for proccesing input text.
			// return value = was any processing performed?

		static void ListAll(void);

		virtual void Execute( ProjChar* pProjCh_In ) = 0;

		virtual ~ConsoleCommand();

		void Display(void) const;


	protected:
		ConsoleCommand
		(
			ProjChar* pProjCh_ToUse,
			ProjChar* pProjCh_Description_ToUse,
			bool Cheat = false
		);

		void EchoResult(int Result);
		int GetArg(ProjChar* pProjCh_Arg);

	private:		
		SCString* pSCString_Description;

		static List <ConsoleCommand*> List_pConsoleCommand;

	};
	

/* Exported globals *****************************************************/

/* Function prototypes **************************************************/



/* End of the header ****************************************************/

#endif
