#include "3dc.h"
#include <math.h>
#include "inline.h"
#include "module.h"
#include "io.h"
#include "stratdef.h"
#include "gamedef.h"
#include "dynblock.h"
#include "dynamics.h"
#include "tables.h"
#include "bh_types.h"
#include "bh_alien.h"
#include "pheromon.h"
#include "pfarlocs.h"
#include "bh_gener.h"
#include "pvisible.h"
#include "lighting.h"
#include "bh_pred.h"
#include "bh_lift.h"
#include "avpview.h"
#include "psnd.h"
#include "psndplat.h"
#include "particle.h"
#include "sfx.h"
#include "version.h"
#include "bh_RubberDuck.h"
#include "bh_marin.h"
#include "dxlog.h"
#include "avp_menus.h"
#include "avp_userprofile.h"
#include "davehook.h"
#include "CDTrackSelection.h"
#include "savegame.h"
// Added 18/11/97 by DHM: all hooks for my code
#define UseLocalAssert TRUE
#include "ourasert.h"
#include "vision.h"
#include "cheat.h"	
#include "pldnet.h"								 
#include "kshape.h"
#define	VERSION_DisableStartupMenus 	TRUE
#define VERSION_DisableStartupCredits	TRUE
#include "avp_menus.h"
#include "FileStream.h"
#include "console.h"

// extern Varibles
extern int VideoMode;
extern int FrameRate;
extern int FrameRate;
extern int Resolution;
extern int PlaySounds;
extern int MotionTrackerScale;
extern int LeanScale;
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

// extern functions
extern void CheckCDStatus(void);
extern void CreateGameSpecificConsoleVariables(void);
extern void CreateGameSpecificConsoleCommands(void);
extern void CreateMoreGameSpecificConsoleVariables(void);
extern void create_strategies_from_list ();
extern void AssignAllSBNames();
extern BOOL Current_Level_Requires_Mirror_Image();
extern void InitialiseTriggeredFMVs();
void check_preplaced_decal_modules();

enum GameVersion
{
	eRetailStandard,
	eRetailGold,
	ePredatorDemo,
	eMarineDemo,
	eAlienDemo
};

static GameVersion gameVersion = eRetailGold; // default to retail Gold edition

/*******************************
EXPORTED GLOBALS
*******************************/

AVP_GAME_DESC AvP;		 /* game description */

char projectsubdirectory[] = {"avp/"};
int SavedFrameRate;
int SavedNormalFrameTime;

unsigned char Null_Name[8];


/* Andy 13/10/97

   This global is set by any initialisation routine if a call to AllocateMem fails.
   It can be checked during debugging after all game and level initialisation to see
   if we have run out of memory.
*/
int memoryInitialisationFailure = 0;


/* start inits for the game*/

void ProcessSystemObjects();

/*runtime maintainance*/

void FindObjectOfFocus();
void MaintainPlayer(void);
void CreatePlayersImageInMirror(void);
void CreateStarArray(void);
void MessageHistory_Initialise();
void TimeScaleThingy();
void MessageHistory_Maintain(void);

void DetermineGameVersion()
{
	// test for demo versions first
	if (DoesFileExist("ENGLISH.TXT")) {
		gameVersion = ePredatorDemo;
		Con_PrintMessage("Detected Predator Demo");
	}
	else if (DoesFileExist("AENGLISH.TXT")) {
		gameVersion = eAlienDemo;
		Con_PrintMessage("Detected Alien Demo");
	}
	else if (DoesFileExist("MENGLISH.TXT")) {
		gameVersion = eMarineDemo;
		Con_PrintMessage("Detected Marine Demo");
	}

	// retail standard or Gold?
	else if (DoesFileExist("avp_huds/mdisk.rif")) {
		gameVersion = eRetailGold;
		Con_PrintMessage("Detected Gold edition");
	}
	else {
		gameVersion = eRetailStandard;
		Con_PrintMessage("Detected Standard edition");
	}
}

bool IsDemoVersion()
{
	if ((gameVersion == eRetailStandard) || (gameVersion == eRetailGold))
		return false;
	else
		return true;
}

/*********************************************

Init Game and Start Game

*********************************************/

void InitGame(void)
{
	/*
		RWH 
		InitGame is to be used only to set platform independent
		varibles. It will be called ONCE only by the game
	*/

	/***** Set up default game settings*/

	AvP.Language = I_English;
	AvP.GameMode = I_GM_Playing;
	AvP.Network = I_No_Network;
	AvP.Difficulty = I_Medium;

 	// Modified by Edmond for Mplayer demo
 	#ifdef MPLAYER_DEMO
 		AvP.StartingEnv = I_Dml1;
 	#else
 		AvP.StartingEnv = I_Entrance;
 	#endif
	AvP.CurrentEnv = AvP.StartingEnv;
	AvP.PlayerType = I_Marine;

	AvP.ElapsedSeconds = 0;
	AvP.ElapsedMinutes = 0;
	AvP.ElapsedHours = 0;
	
	AvP.NetworkAIServer = 0;

	// Added by DHM 18/11/97: Hook for my initialisation code:
	DAVEHOOK_Init();

	/* KJL 12:03:18 30/01/98 - Init console variables and commands */
	CreateGameSpecificConsoleVariables();
	CreateGameSpecificConsoleCommands();
	
	/* Next one is CDF's */
	CreateMoreGameSpecificConsoleVariables();

	#if DEATHMATCH_DEMO
	SetToMinimalDetailLevels();
	#else
	SetToDefaultDetailLevels();
	#endif
}

void StartGame(void)
{
	/* called whenever we start a game (NOT when we change */
	/* environments - destroy anything from a previous game*/

	/*
	Temporarily disable sounds while loading. Largely to avoid 
	some irritating teletext sounds starting up
	*/

	int playSoundsStore=PlaySounds;
	PlaySounds=0;

	// bjd 08/02/10 - crash fix. Re-check this?
	/*	surely we should init squad data here?
	*	funtion calls in create_strategies_from_list(); test data in squad struct
	*	but if this hasn't been initialised, it surely just contains garbage?
	*	keep an eye on this! 
	*/
	InitSquad();

	//get the cd to start again at the beginning of the play list.
	ResetCDPlayForLevel();

	ProcessSystemObjects();
	
	create_strategies_from_list();
	AssignAllSBNames();
	
	SetupVision();
	/*-------------- Patrick 11/1/97 ----------------
	  Initialise visibility system and NPC behaviour 
	  systems for new level.	  
	  -----------------------------------------------*/
	
	InitObjectVisibilities();
	InitPheromoneSystem();
	BuildFarModuleLocs();
	InitHive();
//	InitSquad();

	InitialiseParticleSystem();
	InitialiseSfxBlocks();
	InitialiseLightElementSystem();

	AvP.DestructTimer=-1;

	// DHM 18/11/97: I've put hooks for screen mode changes here for the moment:
	DAVEHOOK_ScreenModeChange_Setup();
	DAVEHOOK_ScreenModeChange_Cleanup();

	/* KJL 11:46:42 30/03/98 - I thought it'd be nice to display the version details
	when you start a game */
	#if PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO
	#else
//	GiveVersionDetails();
	#endif

	#if MIRRORING_ON
	if (Current_Level_Requires_Mirror_Image())
	{
		CreatePlayersImageInMirror();
	}
	#endif

	/* KJL 16:13:30 01/05/98 - rubber ducks! */
	CreateRubberDucks();
	
	CheckCDStatus();

	if (AvP.PlayerType==I_Alien)
	{
		LeanScale=ONE_FIXED*3;
	}
	else
	{
		LeanScale=ONE_FIXED;
	}
	
	MotionTrackerScale = DIV_FIXED(ScreenDescriptorBlock.SDB_Width, 640);

	InitialiseTriggeredFMVs();
	CreateStarArray();

	//check the containing modules for preplaced decals
	check_preplaced_decal_modules();

	CurrentGameStats_Initialise();
	MessageHistory_Initialise();

	if (DISCOINFERNO_CHEATMODE || TRIPTASTIC_CHEATMODE)
	{
		MakeLightElement(&Player->ObWorld,LIGHTELEMENT_ROTATING);
	}

	//restore the play sounds setting
	PlaySounds=playSoundsStore;

  	//make sure the visibilities are up to date
  	Global_VDB_Ptr->VDB_World = Player->ObWorld;
  	AllNewModuleHandler();
  	DoObjectVisibilities();
}


#define FIXED_MINUTE ONE_FIXED*60

void DealWithElapsedTime()
{
	AvP.ElapsedSeconds += NormalFrameTime;
	
	if(AvP.ElapsedSeconds  >= FIXED_MINUTE)
	{
		AvP.ElapsedSeconds -= FIXED_MINUTE;
		AvP.ElapsedMinutes ++;
	}
	
	if(AvP.ElapsedMinutes >= 60)
	{
		AvP.ElapsedMinutes -= 60;
		AvP.ElapsedHours ++;
	}		
}


/**********************************************

 Main Loop Game Functions

**********************************************/
void UpdateGame(void)
{
	/* Read Keyboard, Keypad, Joystick etc. */
	ReadUserInput();

	/* DHM 18/11/97: hook for my code */
	DAVEHOOK_Maintain();

	/*-------------- Patrick 14/11/96 ----------------
	 call the pheronome system maintainence functions
	-------------------------------------------------*/
	PlayerPheromoneSystem();
	AiPheromoneSystem();

	/*-------------- Patrick 11/1/97 ----------------
	Call the alien hive management function
	-------------------------------------------------*/
	DoHive();
	DoSquad();

	ObjectBehaviours();

	/* KJL 10:32:55 09/24/96 - update player */
	MaintainPlayer();

	/* KJL 12:54:08 21/04/98 - make sure the player's matrix is always normalised */
	MNormalise(&(Player->ObStrategyBlock->DynPtr->OrientMat));

	/* netgame support: it seems necessary to collect all our messages here, as some
	things depend on the player's behaviour running before anything else... 
	including firing the player's weapon */
	if (AvP.Network != I_No_Network) NetCollectMessages();

	RemoveDestroyedStrategyBlocks();
	{
		if (SaveGameRequest != SAVELOAD_REQUEST_NONE)
		{
			SaveGame();
		}
		else if (LoadGameRequest != SAVELOAD_REQUEST_NONE)
		{
			LoadSavedGame();
		}
	}

	ObjectDynamics();

	// now for the env teleports
	if (RequestEnvChangeViaLift)
	{
		CleanUpLiftControl();
	}

	/* netgame support */
	if (AvP.Network != I_No_Network) NetSendMessages();

	/* KJL 11:50:18 03/21/97 - cheat modes */
	HandleCheatModes();

	/*------------Patrick 1/6/97---------------
	New sound system
	-------------------------------------------*/
	
	if (playerPherModule)
	{
		PlatSetEnviroment(playerPherModule->m_sound_env_index, playerPherModule->m_sound_reverb);
	}

	SoundSys_Management();
	DoPlayerSounds();

	MessageHistory_Maintain();

	if (AvP.LevelCompleted)
	{
		/*
		If player is dead and has also completed level , then cancel
		level completion.
		*/
		PLAYER_STATUS* PlayerStatusPtr = (PLAYER_STATUS*) Player->ObStrategyBlock->SBdataptr;
		if (!PlayerStatusPtr->IsAlive)
		{
			AvP.LevelCompleted=0;
		}
	}

	if (TRIPTASTIC_CHEATMODE)
	{
		int a = GetSin(CloakingPhase&4095);
		TripTasticPhase = MUL_FIXED(MUL_FIXED(a,a),128)+64;
	}
	if (JOHNWOO_CHEATMODE)
	{
		TimeScaleThingy();
		
		//in john woo mode leanscale is dependent on the TimeScale
		if (AvP.PlayerType==I_Alien)
		{
			LeanScale=ONE_FIXED*3;
		}
		else
		{
			LeanScale=ONE_FIXED;
		}

		LeanScale+=(ONE_FIXED-TimeScale)*5;
	}
}





/* MODULE CALL BACK FUNCTIONS .... */



void ModuleObjectJustAllocated(MODULE *mptr)
{
}


void ModuleObjectAboutToBeDeallocated(MODULE *mptr)
{
}


void NewAndOldModules(int num_new, MODULE **m_new, int num_old, MODULE **m_old, char *m_currvis)
{
	/* this is the important bit */
	DoObjectVisibilities();
}

extern void CheckCDStatus(void)
{
	#if PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO
//	CDCommand_PlayLoop(2);
	#endif	
}

void TimeStampedMessage(char *stringPtr)
{
	static int time=0;
	int t=timeGetTime();
	LOGDXFMT(("%s %fs\n",stringPtr,(float)(t-time)/1000.0f ));
	time = t;
}
