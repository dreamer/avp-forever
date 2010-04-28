/*******************************************************************
 *
 *    DESCRIPTION: 	r2base.cpp
 *
 *    AUTHOR: David Malcolm
 *
 *    HISTORY:  Created 14/11/97
 *
 *******************************************************************/

/* Includes ********************************************************/
#include "3dc.h"
#include "r2base.h"
#include "inline.h"

	
	#define UseLocalAssert TRUE
	#include "ourasert.h"

/* Version settings ************************************************/

/* Constants *******************************************************/

/* Macros **********************************************************/

/* Imported function prototypes ************************************/

/* Imported data ***************************************************/
#ifdef __cplusplus
	extern "C"
	{
#endif

		extern int D3DDriverMode;

		#if 0
		extern OurBool			DaveDebugOn;
		extern FDIEXTENSIONTAG	FDIET_Dummy;
		extern IFEXTENSIONTAG	IFET_Dummy;
		extern FDIQUAD			FDIQuad_WholeScreen;
		extern FDIPOS			FDIPos_Origin;
		extern FDIPOS			FDIPos_ScreenCentre;
		extern IFOBJECTLOCATION IFObjLoc_Origin;
		extern UncompressedGlobalPlotAtomID UGPAID_StandardNull;
		extern IFCOLOUR			IFColour_Dummy;
 		extern IFVECTOR			IFVec_Zero;
		#endif
#ifdef __cplusplus
	};
#endif



/* Exported globals ************************************************/
	/*static*/ const r2pos r2pos :: Origin = r2pos(0,0);
	/*static*/ r2rect r2rect :: R2Rect_PhysicalScreen = r2rect(0,0,640,480);

/* Internal type definitions ***************************************/

/* Internal function prototypes ************************************/

/* Internal globals ************************************************/

/* Exported function definitions ***********************************/
r2pos r2pos :: FixP_Scale
(
	int FixP_ScaleFactor
) const
{
	// assumes the position to be in 16:16 fixed point,
	// returns the position scaled by the fixed pt factor

	return r2pos
	(
		MUL_FIXED(x, FixP_ScaleFactor),
		MUL_FIXED(y, FixP_ScaleFactor)
	);
}

r2pos operator+ ( const r2pos& R2Pos_1, const r2pos& R2Pos_2 )
{
	return r2pos
	(
		R2Pos_1 . x + R2Pos_2 . x,
		R2Pos_1 . y + R2Pos_2 . y
	);
}


extern void R2BASE_ScreenModeChange_Setup(void)
{
	/* PRECONDITION */
	{
	}

	/* CODE */
	{
	}
}

extern "C" extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

extern void R2BASE_ScreenModeChange_Cleanup(void)
{
	/* PRECONDITION */
	{
	}

	/* CODE */
	{

		r2rect :: R2Rect_PhysicalScreen .x1 = ScreenDescriptorBlock.SDB_Width;
		r2rect :: R2Rect_PhysicalScreen .y1 = ScreenDescriptorBlock.SDB_Height;
	}
}


/* Internal function definitions ***********************************/
