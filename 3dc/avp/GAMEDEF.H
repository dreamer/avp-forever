#ifndef GAMEDEF_INCLUDED

/*
	Contains game specific defininitions of structures
*/

#include "module.h"

/***********************************************

							AVP game description stuff

***********************************************/

/* game modes*/

typedef enum gamemode
{
	I_GM_Playing,
	I_GM_DataBase,
	I_GM_Menus,
	I_GM_Paused,

} I_GAME_MODE;
	
typedef enum gamedrawmode
{
	I_GDM_GameOperations,
	I_GDM_DrawWorld,
	I_GDM_DrawHud,
	I_GDM_Flippin,

} I_GAMEDRAW_MODE;

/* Languages*/

typedef enum languages
{
	I_English, 
	I_French, 
	I_Spanish, 
	I_German, 
	I_Italian,
	I_Swedish,
	I_MAX_NO_OF_LANGUAGES

}I_LANGUAGE;	
 
extern char* LanguageDirNames[];


typedef enum playertypes
{
	I_Marine,
	I_Predator,
	I_Alien,

}I_PLAYER_TYPE;

extern char* PlayerNames[];


typedef enum networktype
{
	I_No_Network,
	I_Host,
	I_Peer,
	I_DedicatedServer,

}I_NETWORK;


typedef enum gamedifficulty
{
	I_Easy = 0,
	I_Medium,
	I_Hard,
	I_Impossible,
	I_MaxDifficulties

}I_HARDANUFF;

typedef enum environments
{
 //#ifndef MPLAYER_DEMO
  	I_Gen1 = 0,
  	I_Gen2,
  	I_Gen3,
  	I_Gen4,
 	I_Medlab,	// 5
  	I_Cmc1,
  	I_Cmc2,
  	I_Cmc3,
  	I_Cmc4,
 	I_Cmc5,		// 10
  	I_Cmc6,
  	I_Sp1,
  	I_Sp2,
  	I_Sp3,
 	I_Rnd1,		// 15
  	I_Rnd2,
  	I_Rnd3,
  	I_Rnd4,
  	I_Mps1,
 	I_Mps2,		// 20
  	I_Mps3,
  	I_Mps4,
  	I_Surface,
  	I_Entrance,

 	I_Dml1,	// 25- Vertigo.rif for Al
 // #ifndef MPLAYER_DEMO
 	I_Dml2, // KJL 16:59:58 05/1/97 - fruitbat.rif for Al 
 	I_Dml3, // KJL 16:59:58 05/19/97 - kipper.rif for George 
 	I_Dml4, // KJL 16:59:58 05/19/97 - mu.rif for Jake 
 	I_Dml5, // KJL 16:59:58 05/19/97 - mu.rif for Jake 
 	I_Dml6, // 30- KJL 16:59:58 05/19/97 - mu.rif for Jake 
 	I_Dml7, // KJL 16:59:58 05/19/97 - mu.rif for Jake 
 	I_Dml8, // KJL 16:59:58 05/19/97 - mu.rif for Jake 
 	I_Dml9, // KJL 16:59:58 05/19/97 - mu.rif for Jake 
 	I_Dml10, // KJL 16:59:58 05/19/97 - mu.rif for Jake 
 // #endif

 	I_Num_Environments  // 34

}I_AVP_ENVIRONMENTS;

extern char* LevelNames[];

#define GAME_NAME_LENGTH 30

typedef struct avpgamedesc{

	I_LANGUAGE Language;
	I_GAME_MODE GameMode;
	I_GAMEDRAW_MODE GameDrawMode;
	int DatabaseAccessNum;		// to make it easier to pass this around
	I_PLAYER_TYPE PlayerType;
	I_NETWORK Network;
	I_HARDANUFF Difficulty;
	I_AVP_ENVIRONMENTS CurrentEnv;
	I_AVP_ENVIRONMENTS StartingEnv;
	char GameName[GAME_NAME_LENGTH];
	int GameVideoRequestMode;
	int MenuVideoRequestMode;
	int DestructTimer;
	unsigned int ElapsedMinutes;
	unsigned int ElapsedSeconds; 
	unsigned int ElapsedHours;
	/* KJL 15:36:53 03/11/97 - set to zero
	   on death, pressing quit, etc. */
	unsigned char MainLoopRunning :1;
	/* set to 1 when player dies*/
	unsigned char RestartLevel :1; 

	/* set to 1 if you manage to complete the level */
	unsigned char LevelCompleted :1;

	/* If network game, disable generators if unset */
	unsigned char NetworkAIServer	:1;	

}AVP_GAME_DESC;



	
extern AVP_GAME_DESC AvP;	 /*game.c*/

extern DISPLAYBLOCK *Player;

/***************************************************************/
/************************ AVP high level control **************/

/* KJL 15:42:23 10/02/96 - These two are mine. */
extern void MaintainPlayer(void);
extern void MaintainHUD(void);


/*********************************************************/
/*********************** PLAYER CONTROL STUFF ************/
extern int AnyUserInput();
extern int IDemandGoBackward();
extern int IDemandGoForward();
extern int IDemandTurnRight();
extern int IDemandTurnLeft();

/*************************************************
************* ENVIRONMENT STUFF
**************************************************/

/* some old funnier stuff*/
// kept as this struct so we can compile in environmnet 
// changes


typedef struct environment_list_object{
		char* main;
}ELO;


extern ELO* Env_List[];
extern int EnvToLoad;

extern void ChangeEnvironmentToEnv(I_AVP_ENVIRONMENTS);
extern int Destroy_CurrentEnvironment();
extern void LoadGameFromFile();
extern void SaveGameToFile();
extern void InitCharacter();

/****************** MISC Game Externs ****/

extern MODULEMAPBLOCK TempModuleMap;
extern SHAPEHEADER** mainshapelist;

extern int memoryInitialisationFailure;

/*******************MISC extern functions ***/

extern void DealWithElapsedTime();

#define GAMEDEF_INCLUDED

#endif



