/*
	
	gadget.h

	Base header file for Dave Malcolm's user interface "gadget" code.

	Note to "C" programmers: look at the bottom of this file

*/

#ifndef _gadget
#define _gadget 1


/* Version settings *****************************************************/
	#define UseGadgets TRUE
		/* If this is set to FALSE all gadget code collapses to void macros */

	#define EnableStatusPanels	FALSE

/* Constants  ***********************************************************/
	#define HUD_SPACING 20

/* Macros ***************************************************************/

/* Type definitions *****************************************************/
	#if UseGadgets

		#ifndef _projtext
		#include "projtext.h"
		#endif

	#ifdef __cplusplus

		#ifndef _r2base
		#include "r2base.h"
		#endif

	class Gadget
	{
	public:
		// Pure virtual render method:
		virtual void Render
		(
			const struct r2pos& R2Pos,
			const struct r2rect& R2Rect_Clip,
			int FixP_Alpha
		) = 0;
			// Render yourself at the coordinates given, clipped by the clipping rectangle
			// Note that the position need not be at all related to the clipping rectangle;
			// it's up to the implementation to behave for these cases.
			// Both the coordinate and the clipping rectangle are in absolute screen coordinates
			// The alpha value to use is "absolute"

		virtual ~Gadget();
			// ensure virtual destructor

		#if debug
		char* GetDebugName(void);
		void Render_Report
		(
			const struct r2pos& R2Pos,
			const struct r2rect& R2Rect_Clip,
			int FixP_Alpha			
		);
			// use to textprint useful information about a call to "Render"
		#endif

	protected:
		// Protected constructor since abstract base class
		#if debug
		Gadget
		(
			char* DebugName_New
		) : DebugName( DebugName_New )
		{
			// empty
		}
		#else
		Gadget(){}
		#endif

	private:
		#if debug
		char* DebugName;
		#endif

	}; // end of class Gadget

	// Inline methods:
	#if debug
	inline char* Gadget::GetDebugName(void)
	{
		return DebugName;
	}
	#endif

	#endif /* __cplusplus */
	#endif /* UseGadgets */

/* Exported globals *****************************************************/

/* Function prototypes **************************************************/

	#if UseGadgets

	extern void GADGET_Init(void);
		/* expects to be called at program boot-up time */

	extern void GADGET_UnInit(void);
		/* expects to be called at program shutdown time */

	extern void GADGET_Render(void);
		/* expects to be called within the rendering part of the main loop */

	extern void GADGET_ScreenModeChange_Setup(void);
		/* expects to be called immediately before anything happens to the screen
		mode */

	extern void GADGET_ScreenModeChange_Cleanup(void);
		/* expects to be called immediately after anything happens to the screen
		mode */

	extern void GADGET_NewOnScreenMessage( ProjChar* messagePtr );

	extern void RemoveTheConsolePlease(void);

	#else /* UseGadgets */

		#define GADGET_Init() ((void) 0)
		#define GADGET_UnInit() ((void) 0)
		#define GADGET_Render() ((void) 0)
		#define GADGET_ScreenModeChange_Setup() ((void) 0)
		#define GADGET_ScreenModeChange_Cleanup() ((void) 0)
		#define GADGET_NewOnScreenMessage(x) ((void) 0)

	#endif /* UseGadgets */


/* End of the header ****************************************************/

#endif
