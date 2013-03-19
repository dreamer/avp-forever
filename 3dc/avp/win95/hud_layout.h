/* KJL 17:58:47 18/04/98 - layout defines */

#if (MARINE_DEMO || ALIEN_DEMO || PREDATOR_DEMO)
#define HUD_FONT_WIDTH  5 // 5 for demo - smallfont.rim
#define HUD_FONT_HEIGHT 8 // 8 for demo - smallfont.rim
#else
#define HUD_FONT_WIDTH  15 // 5 for demo - smallfont.rim
#define HUD_FONT_HEIGHT 15 // 8 for demo - smallfont.rim
#endif

#define HUD_DIGITAL_NUMBERS_WIDTH		14
#define HUD_DIGITAL_NUMBERS_HEIGHT		22

#define HUDLayout_RightmostTextCentre	-40

#define	HUDLayout_Health_TopY			10
#define	HUDLayout_Armour_TopY			60

/* KJL 15:28:12 09/06/98 - the following are pixels from the bottom of the screen */
#define HUDLayout_Rounds_TopY			40
#define HUDLayout_Magazines_TopY 		90
#define HUDLayout_AmmoDesc_TopY 		115


#define HUDLayout_Colour_BrightWhite	0xffffffff
#define HUDLayout_Colour_MarineGreen	((255<<24)+(95<<16)+(179<<8)+(39))
#define HUDLayout_Colour_MarineRed		((255<<24)+(255<<16))
#define HUDLayout_Linespacing			16

extern char AAFontWidths[];

