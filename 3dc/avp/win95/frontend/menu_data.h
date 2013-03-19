#ifndef _AvP_MenuData_h_
#define _AvP_MenuData_h_

#include "pldnet.h"

#define MAX_MULTIPLAYER_NAME_LENGTH 10

extern int MP_Species;
extern int MP_GameStyle;
extern int MP_LevelNumber;

extern char MP_Config_Description[202];
extern char MP_PlayerName[NET_PLAYERNAMELENGTH+3];
extern char MP_SessionName[MAX_MULTIPLAYER_NAME_LENGTH+23];

#endif