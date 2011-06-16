/*KJL**************************************************************************
* HUDDEFS.H -                                                                 *
* 	                                                                          *
* 	this file contains all the prototypes of the platform-dependent functions *
* 	called in hud.c.                                                          *
**************************************************************************KJL*/

#ifndef _huddefs_h
#define _huddefs_h 1

/*KJL****************************************************************************************
* 										D E F I N E S 										*
****************************************************************************************KJL*/

/*KJL**************************************************************************************
* Motion Tracker defines - Range is currently 30 metres. Don't worry about those casts to *
* floats, they're only there to ensure accuracy and are preprocessed away. For example,   *
* 65536*65536 will probably cause an overflow in the preprocessor so I've used floats to  *
* avoid this.                                                                             *
**************************************************************************************KJL*/
#define MOTIONTRACKER_RANGE					((int)((float)30000*(float)GlobalScale))
#define MOTIONTRACKER_RANGE_SQUARED 		(MOTIONTRACKER_RANGE*MOTIONTRACKER_RANGE)
#define MOTIONTRACKER_SCALE					(int)((65536.0*65536.0)/(float)MOTIONTRACKER_RANGE)
#define MOTIONTRACKER_SPEED					(MUL_FIXED((65536*2),MotionTrackerSpeed))
#define MOTIONTRACKER_MAXBLIPS				10
#define MOTIONTRACKER_SMALLESTSCANLINESIZE	2200

typedef struct
{
	int X;
	int Y;
	int Brightness;
} BLIP_TYPE;
/*KJL*************************************************
* Speed at which gunsight moves when smart-targeting *
*************************************************KJL*/
#define SMART_TARGETING_SPEED 4

#define SMART_TARGETING_RANGE 1000000


/*KJL*********************************************
* Numerical digits which occur in the marine HUD *
*********************************************KJL*/
enum MARINE_HUD_DIGIT
{
	MARINE_HUD_MOTIONTRACKER_UNITS=0,
    MARINE_HUD_MOTIONTRACKER_TENS,
    MARINE_HUD_MOTIONTRACKER_HUNDREDS,
    MARINE_HUD_MOTIONTRACKER_THOUSANDS,

    MARINE_HUD_HEALTH_UNITS,
    MARINE_HUD_HEALTH_TENS,
    MARINE_HUD_HEALTH_HUNDREDS,

    MARINE_HUD_ENERGY_UNITS,
    MARINE_HUD_ENERGY_TENS,
    MARINE_HUD_ENERGY_HUNDREDS,

    MARINE_HUD_ARMOUR_UNITS,
    MARINE_HUD_ARMOUR_TENS,
    MARINE_HUD_ARMOUR_HUNDREDS,

    MARINE_HUD_PRIMARY_AMMO_ROUNDS_UNITS,
    MARINE_HUD_PRIMARY_AMMO_ROUNDS_TENS,
    MARINE_HUD_PRIMARY_AMMO_ROUNDS_HUNDREDS,

	MARINE_HUD_PRIMARY_AMMO_MAGAZINES_UNITS,
    MARINE_HUD_PRIMARY_AMMO_MAGAZINES_TENS,

    MARINE_HUD_SECONDARY_AMMO_ROUNDS_UNITS,
    MARINE_HUD_SECONDARY_AMMO_ROUNDS_TENS,
    MARINE_HUD_SECONDARY_AMMO_ROUNDS_HUNDREDS,

	MARINE_HUD_SECONDARY_AMMO_MAGAZINES_UNITS,
    MARINE_HUD_SECONDARY_AMMO_MAGAZINES_TENS,

	MAX_NO_OF_MARINE_HUD_DIGITS
};
/*KJL***********************************************
* Numerical digits which occur in the predator HUD *
***********************************************KJL*/
enum PREDATOR_HUD_DIGIT
{
    PREDATOR_HUD_ARMOUR_1,
    PREDATOR_HUD_ARMOUR_2,
    PREDATOR_HUD_ARMOUR_3,
    PREDATOR_HUD_ARMOUR_4,
    PREDATOR_HUD_ARMOUR_5,

    PREDATOR_HUD_HEALTH_1,
	PREDATOR_HUD_HEALTH_2,
    PREDATOR_HUD_HEALTH_3,
    PREDATOR_HUD_HEALTH_4,
    PREDATOR_HUD_HEALTH_5,

	/*
	PREDATOR_HUD_THREATDISPLAY_1,
	PREDATOR_HUD_THREATDISPLAY_2,
	PREDATOR_HUD_THREATDISPLAY_3,
	PREDATOR_HUD_THREATDISPLAY_4,
	PREDATOR_HUD_THREATDISPLAY_5,
	PREDATOR_HUD_THREATDISPLAY_6,
	PREDATOR_HUD_THREATDISPLAY_7,
	PREDATOR_HUD_THREATDISPLAY_8,
	*/
	MAX_NO_OF_PREDATOR_HUD_DIGITS
};

enum ALIEN_HUD_DIGIT
{
    ALIEN_HUD_HEALTH_UNITS,
    ALIEN_HUD_HEALTH_TENS,
    ALIEN_HUD_HEALTH_HUNDREDS,

	MAX_NO_OF_ALIEN_HUD_DIGITS
};

extern char ValueOfHUDDigit[];

enum GUNSIGHT_SHAPE
{
	GUNSIGHT_CROSSHAIR=0,
    GUNSIGHT_GREENBOX,
    GUNSIGHT_REDBOX,
    GUNSIGHT_REDDIAMOND,

    MAX_NO_OF_GUNSIGHT_SHAPES
};

enum COMMON_HUD_DIGIT_ID
{
	COMMON_HUD_DIGIT_HEALTH_UNITS,
	COMMON_HUD_DIGIT_HEALTH_TENS,
	COMMON_HUD_DIGIT_HEALTH_HUNDREDS,

	COMMON_HUD_DIGIT_ARMOUR_UNITS,
	COMMON_HUD_DIGIT_ARMOUR_TENS,
	COMMON_HUD_DIGIT_ARMOUR_HUNDREDS,

	MAX_NO_OF_COMMON_HUD_DIGITS
};

/*KJL****************************************************************************************
*                                    P R O T O T Y P E S	                                *
****************************************************************************************KJL*/
extern void PlatformSpecificInitMarineHUD(void);
/*KJL****************************************************************************************
* Okay. From now on everyone will call the fn above which loads and initialises ALL the gfx *
* required for a marine, eg. weapons, motion tracker stuff, gun sights, et al.              *
* And sets up the riff mode RWH
****************************************************************************************KJL*/
extern void PlatformSpecificInitPredatorHUD(void);
/*KJL******************
* Ditto for predator. *
******************KJL*/
extern void PlatformSpecificInitAlienHUD(void);
/*RWH*****************
* Ditto for alien. *
******************REH*/

extern void BLTMotionTrackerToHUD(int scanLineSize);
/*KJL******************************************************************************************
* draw motion tracker with its expanding scanline                                             *
* 0 <= scanLineSize <= 65536 and denotes the scanline's on-screen radius (65536 = full size). *
******************************************************************************************KJL*/

extern void BLTMotionTrackerBlipToHUD(int x, int y, int brightness);
/*KJL********************************************************************
* -65536 <= x <= 65536, 0 <= y <= 65536, 0 <= brightness <= 65536  		*
* (x=0,y=0) refers to the motiontracker's centre. (ie. centre hotspot)  *
* brightness=65536 means brightest blip, 0 means darkest blip      		*
********************************************************************KJL*/

extern void InitHUD(void);

/* KJL 11:00:22 05/20/97 - On-screen messaging system */
#define ON_SCREEN_MESSAGE_LIFETIME (ONE_FIXED*2)

extern void NewOnScreenMessage(char *messagePtr);
/*KJL********************************************************************
* The text pointed to by the messagePtr will be displayed on screen for *
* the time defined in ON_SCREEN_MESSAGE_LIFETIME. Any previous message  *
* still being displayed will be overwritten.                            *
********************************************************************KJL*/

#endif /* one-time only guard */
