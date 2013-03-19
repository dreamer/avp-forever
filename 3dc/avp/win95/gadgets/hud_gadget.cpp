/*******************************************************************
 *
 *    DESCRIPTION: 	hudgadg.cpp
 *
 *    AUTHOR: David Malcolm
 *
 *    HISTORY:  Created 14/11/97
 *
 *******************************************************************/

/* Includes ********************************************************/
#include "3dc.h"
#include "hud_gadget.hpp"
//#include "mhudgadg.hpp"
#include "alien_hud_gadget.hpp"
//#include "phudgadg.hpp"
#include "text_report_gadget.hpp"
	
	#define UseLocalAssert TRUE
	#include "ourasert.h"

/* Version settings ************************************************/

/* Constants *******************************************************/

/* Macros **********************************************************/

/* Imported function prototypes ************************************/

/* Imported data ***************************************************/

/* Exported globals ************************************************/
#if UseGadgets
	// private:
	/*static*/ HUDGadget* HUDGadget :: pSingleton;
#endif

/* Internal type definitions ***************************************/

/* Internal function prototypes ************************************/

/* Internal globals ************************************************/

/* Exported function definitions ***********************************/
#if UseGadgets
// HUD Gadget is an abstract base class for 3 types of HUD; one for each species
// It's abstract because the Render() method remains pure virtual
// class HUDGadget : public Gadget
// public:

// Factory method:
/*static*/ HUDGadget* HUDGadget :: MakeHUD
(
	I_PLAYER_TYPE IPlayerType_ToMake
)
{
	/* PRECONDITION */
	{
	}

	/* CODE */
	{
		#if 0
		switch ( IPlayerType_ToMake )
		{
			case I_Marine:
				return new MarineHUDGadget();

			case I_Predator:
				return new PredatorHUDGadget();

			case I_Alien:
				return new AlienHUDGadget();

			default:
				GLOBALASSERT(0);
				return NULL;
		}
		#else
		return new AlienHUDGadget();
//		return NULL;
		#endif
	}
}


// Destructor:
/*virtual*/ HUDGadget :: ~HUDGadget()
{
	/* PRECONDITION */
	{
		GLOBALASSERT( this == pSingleton );
	}

	/* CODE */
	{
		pSingleton = NULL;
	}
}


// protected:
// Constructor is protected since an abstract class
HUDGadget :: HUDGadget
(
	#if debug
	char* DebugName
	#endif
) : Gadget
	(
		#if debug
		DebugName
		#endif		
	)
{
	/* PRECONDITION */
	{
		GLOBALASSERT( NULL == pSingleton );
	}

	/* CODE */
	{
		#if 0
		pSCString_Current = NULL;
		#endif

		pSingleton = this;
	}
}
#endif // UseGadgets

/* Internal function definitions ***********************************/
