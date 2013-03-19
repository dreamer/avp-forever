/*******************************************************************
 *
 *    DESCRIPTION: 	equiputl.cpp
 *
 *    AUTHOR: David Malcolm
 *
 *    HISTORY:  Created 8/1/98
 *
 *******************************************************************/

/* Includes ********************************************************/
#include "3dc.h"
#include "equipment_util.hpp"

	#define UseLocalAssert TRUE
	#include "ourasert.h"

/* Version settings ************************************************/

/* Constants *******************************************************/

/* Macros **********************************************************/

/* Imported function prototypes ************************************/

/* Imported data ***************************************************/

/* Exported globals ************************************************/

/* Internal type definitions ***************************************/

/* Internal function prototypes ************************************/

/* Internal globals ************************************************/

/* Exported function definitions ***********************************/

// namespace EquipmentUtil

// Functions to get the ID for the text string "ROUNDS" and "MAGAZINES"
// Mostly returns TEXTSTRING_ROUNDS and TEXTSTRING_MAGAZINES,
// but some weapons have different strings

enum TEXTSTRING_ID EquipmentUtil :: GetTextID_Rounds
(
	enum WEAPON_ID WeaponID
)
{
	{
		return TEXTSTRING_ROUNDS;
	}
}

enum TEXTSTRING_ID EquipmentUtil :: GetTextID_Magazines
(
	enum WEAPON_ID WeaponID
)
{
	switch (WeaponID)
	{
		default:
			return TEXTSTRING_MAGAZINES;

		case WEAPON_FLAMETHROWER:
			return TEXTSTRING_MAGAZINES_FLAMETHROWER;

	}
}



		// Mostly returns TEXTSTRING_ROUNDS and TEXTSTRING_MAGAZINES,
		// but some weapons have different strings

/* Internal function definitions ***********************************/
