#ifndef _included_AvP_Menus_h_
#define _included_AvP_Menus_h_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

	extern int mainMenu;

#ifdef __cplusplus
}
#endif // __cplusplus


#define MARINE_DEMO 0
#define PREDATOR_DEMO 0
#define ALIEN_DEMO 0
 // Edmond modified for Mplayer Demo
 #ifdef MPLAYER_DEMO
 #define DEATHMATCH_DEMO 1
 #else
 #define DEATHMATCH_DEMO 0 // more multiplayer-only demo really
 #endif

#ifdef AVP_DEBUG_VERSION
	#define CONSOLE_DEBUGGING_COMMANDS_ACTIVATED 1

	/* bjd 24/11/08 - load all sounds for debug mode.. */
	#define LOAD_SCREAMS_FROM_FASTFILES 1
	#define LOAD_USING_FASTFILES 1

#else //AVP_DEBUG_VERSION

	#ifdef AVP_DEBUG_FOR_FOX
		#define CONSOLE_DEBUGGING_COMMANDS_ACTIVATED 1
	#else
		#define CONSOLE_DEBUGGING_COMMANDS_ACTIVATED 0
	#endif

	#define LOAD_SCREAMS_FROM_FASTFILES 1
	#define LOAD_USING_FASTFILES 1
#endif //AVP_DEBUG_VERSION

#define PLAY_INTRO 1
#define ALLOW_SKIP_INTRO 1

#define SAVE_GAME_ON 1

#include "AvP_MenuGfx.hpp"
#include "language.h"

enum MENUSSTATE_ID
{
	MENUSSTATE_MAINMENUS,
	MENUSSTATE_INGAMEMENUS,
	MENUSSTATE_OUTSIDEMENUS,
	MENUSSTATE_STARTGAME,
};

int GetAvPMenuState();

enum AVPMENU_ID
{
	AVPMENU_MAIN,
	AVPMENU_MAIN_WITHCHEATS,
	AVPMENU_DEBUG_MAIN,
	AVPMENU_DEBUG_MAIN_WITHCHEATS,

	AVPMENU_EXITGAME,

	AVPMENU_USERPROFILESELECT,
	AVPMENU_USERPROFILEENTERNAME,
	AVPMENU_USERPROFILEDELETE,

	AVPMENU_SINGLEPLAYER,
	AVPMENU_MARINELEVELS,
	AVPMENU_ALIENLEVELS,
	AVPMENU_PREDATORLEVELS,

	AVPMENU_MULTIPLAYER,
	AVPMENU_MULTIPLAYER_LOBBIEDSERVER,
	AVPMENU_MULTIPLAYER_LOBBIEDCLIENT,
	AVPMENU_MULTIPLAYER_SKIRMISH,
	AVPMENU_MULTIPLAYERSTARTGAME,
	AVPMENU_MULTIPLAYERJOINGAME,
	AVPMENU_MULTIPLAYERSELECTSESSION,
	AVPMENU_MULTIPLAYERJOINGAME2,
	AVPMENU_MULTIPLAYEROPENADDRESS,
	AVPMENU_MULTIPLAYER_LOADIPADDRESS,
	AVPMENU_MULTIPLAYER_CONNECTION,
	AVPMENU_MULTIPLAYER_CONFIG,
	AVPMENU_MULTIPLAYER_CONFIG_JOIN,
	AVPMENU_SKIRMISH_CONFIG,
	AVPMENU_MULTIPLAYER_SPECIES_HOST,
	AVPMENU_MULTIPLAYER_SPECIES_JOIN,

	AVPMENU_MULTIPLAYER_SAVECONFIG,
	AVPMENU_MULTIPLAYER_LOADCONFIG,
	AVPMENU_MULTIPLAYER_DELETECONFIG,

	AVPMENU_MULTIPLAYER_JOINING,

	AVPMENU_CONTROLS,
	AVPMENU_JOYSTICKCONTROLS,

	AVPMENU_INGAMEAVOPTIONS,
	AVPMENU_MAINMENUAVOPTIONS,
	AVPMENU_VIDEOMODE,


	AVPMENU_INGAME,
	AVPMENU_INNETGAME,

	AVPMENU_MARINEKEYCONFIG,
	AVPMENU_PREDATORKEYCONFIG,
	AVPMENU_ALIENKEYCONFIG,
	AVPMENU_OPTIONS,

	AVPMENU_LEVELBRIEFING_BASIC,
	AVPMENU_LEVELBRIEFING_BONUS,

	AVPMENU_CHEATOPTIONS,
	AVPMENU_DETAILLEVELS,

	AVPMENU_LOADGAME,
	AVPMENU_SAVEGAME,
};

enum AVPMENU_FONT_ID
{
	AVPMENU_FONT_SMALL,
	AVPMENU_FONT_BIG,
};

/* menu elements */
enum AVPMENU_ELEMENT_ID
{
	AVPMENU_ELEMENT_GOTOMENU,
	AVPMENU_ELEMENT_GOTOMENU_GFX,

	AVPMENU_ELEMENT_TOGGLE,
	AVPMENU_ELEMENT_SLIDER,

	AVPMENU_ELEMENT_USERPROFILE,
	AVPMENU_ELEMENT_ALIENEPISODE,
	AVPMENU_ELEMENT_MARINEEPISODE,
	AVPMENU_ELEMENT_PREDATOREPISODE,
	AVPMENU_ELEMENT_STARTMPGAME,
	AVPMENU_ELEMENT_JOINMPGAME,
	AVPMENU_ELEMENT_JOINLOBBIED,

	AVPMENU_ELEMENT_QUITGAME,

	AVPMENU_ELEMENT_TEXTFIELD,
	AVPMENU_ELEMENT_DUMMYTEXTFIELD,
	AVPMENU_ELEMENT_TEXTFIELD_SMALLWRAPPED,
	AVPMENU_ELEMENT_TEXTSLIDER,
	AVPMENU_ELEMENT_DUMMYTEXTSLIDER,
	AVPMENU_ELEMENT_SPECIES_TEXTSLIDER,

	AVPMENU_ELEMENT_TEXTSLIDER_POINTER,		 // text slider with a char** rather than a string index
	AVPMENU_ELEMENT_DUMMYTEXTSLIDER_POINTER, // text slider with a char** rather than a string index

	AVPMENU_ELEMENT_CHEATMODE_TEXTSLIDER,
	AVPMENU_ELEMENT_CHEATMODE_SPECIES_TEXTSLIDER,
	AVPMENU_ELEMENT_CHEATMODE_ENVIRONMENT_TEXTSLIDER,

	AVPMENU_ELEMENT_NUMBERFIELD,
	AVPMENU_ELEMENT_DUMMYNUMBERFIELD,

	AVPMENU_ELEMENT_LISTCHOICE,

	AVPMENU_ELEMENT_CONNECTIONCHOICE,

	AVPMENU_ELEMENT_VIDEOMODE,
	AVPMENU_ELEMENT_VIDEOMODEOK,

	AVPMENU_ELEMENT_KEYCONFIG,
	AVPMENU_ELEMENT_KEYCONFIGOK,
	AVPMENU_ELEMENT_RESETKEYCONFIG,

	AVPMENU_ELEMENT_ENDOFMENU,

	AVPMENU_ELEMENT_RESUMEGAME,
	AVPMENU_ELEMENT_RESTARTGAME,

	AVPMENU_ELEMENT_STARTMARINEDEMO,
	AVPMENU_ELEMENT_STARTPREDATORDEMO,
	AVPMENU_ELEMENT_STARTALIENDEMO,
	AVPMENU_ELEMENT_STARTLEVELWITHCHEAT,

	AVPMENU_ELEMENT_BUTTONSETTING,
	AVPMENU_ELEMENT_LOADMPCONFIG,
	AVPMENU_ELEMENT_SAVEMPCONFIG,
	AVPMENU_ELEMENT_DELETEMPCONFIG,
	AVPMENU_ELEMENT_RESETMPCONFIG,

	AVPMENU_ELEMENT_DIFFICULTYLEVEL,

	AVPMENU_ELEMENT_LOADIPADDRESS,

	AVPMENU_ELEMENT_USERPROFILE_DELETE,


	AVPMENU_ELEMENT_SAVESETTINGS,

	AVPMENU_ELEMENT_SAVEGAME,
	AVPMENU_ELEMENT_LOADGAME,
};


enum AVPMENU_ELEMENT_INTERACTION_ID
{
	AVPMENU_ELEMENT_INTERACTION_SELECT,
	AVPMENU_ELEMENT_INTERACTION_DECREASE,
	AVPMENU_ELEMENT_INTERACTION_INCREASE,
};

typedef struct
{
	enum AVPMENU_ELEMENT_ID ElementID;

	union
	{
		enum TEXTSTRING_ID TextDescription;
//		enum AVPMENUGFX_ID GfxID;
		uint32_t textureID;
	};

	union
	{
		enum AVPMENU_ID MenuToGoTo;
		int MaxSliderValue;
		int MaxTextLength;
		int MaxValue; //for number fields
	};

	union
	{
		int *ToggleValuePtr;
		int *SliderValuePtr;
		char *TextPtr;
		int *NumberPtr;
		int Value;
	};

	union
	{
		enum TEXTSTRING_ID FirstTextSliderString;
		enum TEXTSTRING_ID NumberFieldUnitsString;
		char** TextSliderStringPointer;
	};

	enum TEXTSTRING_ID HelpString;

	union
	{
		enum TEXTSTRING_ID NumberFieldZeroString; //special string for 0
	};
	int Brightness;

} AVPMENU_ELEMENT;

typedef struct
{
	enum AVPMENU_FONT_ID FontToUse;
	enum TEXTSTRING_ID	MenuTitle;

	AVPMENU_ELEMENT		*MenuElements;
	enum AVPMENU_ID		ParentMenu;
	int					DefaultElement;

} AVPMENU;


typedef struct
{
	enum MENUSSTATE_ID	MenusState;

	enum AVPMENU_ID		CurrentMenu;
	AVPMENU_ELEMENT		*MenuElements;
	int					NumberOfElementsInMenu;
	int					CurrentlySelectedElement;
	int					MenuHeight;

	enum AVPMENU_FONT_ID FontToUse;

	size_t				PositionInTextField;

	int 				WidthLeftForText; //thing for checking whether user input will fit on screen

	unsigned int		UserEnteringText :1;
	unsigned int		UserEnteringNumber :1;
	unsigned int		UserChangingKeyConfig :1;
	unsigned int		ChangingPrimaryConfig :1;

} AVP_MENUS;


#define BRIGHTNESS_OF_HIGHLIGHTED_ELEMENT (ONE_FIXED)
#define BRIGHTNESS_OF_DARKENED_ELEMENT (ONE_FIXED/4)
#define BRIGHTNESS_CHANGE_SPEED (RealFrameTime*2)

#define MENU_TOPY		(150)
#define MENU_CENTREY	(280)
#define MENU_HEIGHT		(255)
#define MENU_CENTREX	(320)

#define MENU_ELEMENT_SPACING	(9)
#define MENU_FONT_HEIGHT		(21)
#define MENU_LEFTXEDGE			(30)
#define MENU_RIGHTXEDGE			(610)
#define MENU_BOTTOMYEDGE		(390)
#define MENU_TOPYEDGE			(130)
#define MENU_WIDTH (MENU_RIGHTXEDGE-MENU_LEFTXEDGE)


/* KJL 18:05:51 05/07/98 - multiplayer stuff */
typedef struct
{
	char Name[40];
	char levelIndex; //local level index
	char hostAddress[16];
	GUID Guid;
	BOOL AllowedToJoin;
} SESSION_DESC;

#define MAX_NO_OF_SESSIONS 10
extern SESSION_DESC SessionData[];
extern int NumberOfSessionsFound;

extern char IP_Address_Name[];



typedef struct
{
	unsigned char	SlotUsed;
	unsigned char	SavesLeft;
	unsigned char	Species;
	unsigned char	Episode;
	unsigned char	ElapsedTime_Hours;
	unsigned char	ElapsedTime_Minutes;
	unsigned char	ElapsedTime_Seconds;
	unsigned char	Difficulty;

	SYSTEMTIME 		TimeStamp;

} SAVE_SLOT_HEADER;

#define NUMBER_OF_SAVE_SLOTS 8

#endif
