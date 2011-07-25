/* inits the system and controls environment and player loading */

#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "gameplat.h"
#define UseLocalAssert TRUE
#include "ourasert.h"

/* patrick 5/12/96 */
#include "bh_far.h"
#include "pheromon.h"
#include "huddefs.h"
#include "hudgfx.h"
#include "font.h"
#include "bh_gener.h"
#include "pvisible.h"

#include "projload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
//#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?

#include "ffstdio.h" // fast file stdio
#include "avp_menus.h"

/*------------Patrick 1/6/97---------------
New sound system 
-------------------------------------------*/
#include "psndplat.h"
#include "progress_bar.h"
#include "bh_rubberduck.h"
#include "game_statistics.h"
#include "CDTrackSelection.h"


// EXTERNS


extern int WindowMode;
extern int VideoMode;

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

extern SHAPEHEADER** mainshapelist;

extern int NumActiveBlocks;
extern int NumOnScreenBlocks;
extern DISPLAYBLOCK *ActiveBlockList[];

extern MODULEMAPBLOCK AvpCompiledMaps[];
extern MAPHEADER TestMap[];
extern MAPHEADER Map[];
extern MAPHEADER * staticmaplist[];
extern MAPBLOCK8 Player_and_Camera_Type8[];

extern void create_strategies_from_list();
extern void AssignAllSBNames();

extern SCENEMODULE **Global_ModulePtr;
extern SCENEMODULE *MainSceneArray[];

extern int Resolution;
extern void SetupVision(void);
extern void ReInitHUD(void);
extern void CheckCDStatus(void);
void InitSquad(void);
void InitialiseParticleSystem(void);
void InitialiseSfxBlocks(void);
void InitialiseLightElementSystem(void);
extern void InitialiseTriggeredFMVs();
void MessageHistory_Initialise(void);
void TeleportNetPlayerToAStartingPosition(STRATEGYBLOCK *playerSbPtr, int startOfGame);
void CDDA_Stop();
void TimeStampedMessage(char *stringPtr);

extern void DeallocateSoundsAndPoolAllocatedMemory();


/*Globals */

int WindowRequestMode;
int VideoRequestMode;
int RasterisationRequestMode;
int SoftwareScanDrawRequestMode;
WINSCALEXY TopLeftSubWindow;
WINSCALEXY ExtentXYSubWindow;


// static 

static int ReadModuleMapList(MODULEMAPBLOCK *mmbptr);
RIFFHANDLE env_rif = INVALID_RIFFHANDLE;
RIFFHANDLE player_rif = INVALID_RIFFHANDLE;
RIFFHANDLE alien_weapon_rif = INVALID_RIFFHANDLE;
RIFFHANDLE marine_weapon_rif = INVALID_RIFFHANDLE;
RIFFHANDLE predator_weapon_rif = INVALID_RIFFHANDLE;

void ProcessSystemObjects();

/*******************************************************************************************/
/*******************************************************************************************/
/***************		   GAME AND ENIVROMENT CONTROL			  **************************/

void InitCharacter()
{
	/*** RWH cleans up the character initialisation 
			 it would be nice if this can be called when
			 we load up a game of a different character
	***/
	
	// load charcater specific rif and sounds

	if (player_rif != INVALID_RIFFHANDLE)
	{
		// we already have a player loaded - delete the bastard
		avp_undo_rif_load(player_rif);
	}
	if (alien_weapon_rif != INVALID_RIFFHANDLE)
	{
		// we already have a player loaded - delete the bastard
		avp_undo_rif_load(alien_weapon_rif);
	}
	if (marine_weapon_rif != INVALID_RIFFHANDLE)
	{
		// we already have a player loaded - delete the bastard
		avp_undo_rif_load(marine_weapon_rif);
	}
	if (predator_weapon_rif != INVALID_RIFFHANDLE)
	{
		// we already have a player loaded - delete the bastard
		avp_undo_rif_load(predator_weapon_rif);
	}

	#if MaxImageGroups==1
	InitialiseTextures();
	#else
	SetCurrentImageGroup(0);
	DeallocateCurrentImages();
	#endif

	Start_Progress_Bar();

	Set_Progress_Bar_Position(PBAR_HUD_START);

	switch(AvP.Network)
	{
		case I_No_Network:
		{
			// set up the standard single player game
			switch(AvP.PlayerType)
			{
				case I_Marine:
				{
					marine_weapon_rif = avp_load_rif("avp_huds/marwep.rif");
					Set_Progress_Bar_Position(PBAR_HUD_START+PBAR_HUD_INTERVAL*.25);
					player_rif = avp_load_rif("avp_huds/marine.rif");
					break;
				}
				case I_Predator:
				{
					predator_weapon_rif = avp_load_rif("avp_huds/pred_hud.rif");
					Set_Progress_Bar_Position(PBAR_HUD_START+PBAR_HUD_INTERVAL*.25);
					player_rif = avp_load_rif("avp_huds/predator.rif");
					break;
				}
				case I_Alien:
				{
					#if ALIEN_DEMO
					alien_weapon_rif = avp_load_rif("alienavp_huds/alien_hud.rif");
					Set_Progress_Bar_Position(PBAR_HUD_START+PBAR_HUD_INTERVAL*.25);
					player_rif = avp_load_rif("alienavp_huds/alien.rif");
					#else
					alien_weapon_rif = avp_load_rif("avp_huds/alien_hud.rif");
					Set_Progress_Bar_Position(PBAR_HUD_START+PBAR_HUD_INTERVAL*.25);
					player_rif = avp_load_rif("avp_huds/alien.rif");
					#endif
					break;
				}
				default:
				{
					GLOBALASSERT(2<1);
				}
			}
			break;
		}
		default:
		{
			// set up a multiplayer game - here becuse we might end
			// up with a cooperative game
			//load all weapon rifs
			marine_weapon_rif = avp_load_rif("avp_huds/marwep.rif");
			predator_weapon_rif = avp_load_rif("avp_huds/pred_hud.rif");
			alien_weapon_rif = avp_load_rif("avp_huds/alien_hud.rif");

			Set_Progress_Bar_Position(PBAR_HUD_START+PBAR_HUD_INTERVAL*.25);
			player_rif = avp_load_rif("avp_huds/multip.rif");
		}
	}
	Set_Progress_Bar_Position(PBAR_HUD_START+PBAR_HUD_INTERVAL*.5);

	#if MaxImageGroups>1
	SetCurrentImageGroup(0);
	#endif
	copy_rif_data(player_rif,CCF_IMAGEGROUPSET,PBAR_HUD_START+PBAR_HUD_INTERVAL*.5,PBAR_HUD_INTERVAL*.25);

	Set_Progress_Bar_Position(PBAR_HUD_START+PBAR_HUD_INTERVAL*.75);

	if (alien_weapon_rif != INVALID_RIFFHANDLE)
		copy_rif_data(alien_weapon_rif, CCF_LOAD_AS_HIERARCHY_IF_EXISTS|CCF_IMAGEGROUPSET|CCF_DONT_INITIALISE_TEXTURES, PBAR_HUD_START+PBAR_HUD_INTERVAL*.5, PBAR_HUD_INTERVAL*.25);

	if (marine_weapon_rif != INVALID_RIFFHANDLE)
		copy_rif_data(marine_weapon_rif,CCF_LOAD_AS_HIERARCHY_IF_EXISTS|CCF_IMAGEGROUPSET|CCF_DONT_INITIALISE_TEXTURES, PBAR_HUD_START+PBAR_HUD_INTERVAL*.5, PBAR_HUD_INTERVAL*.25);

	if (predator_weapon_rif != INVALID_RIFFHANDLE)
		copy_rif_data(predator_weapon_rif, CCF_LOAD_AS_HIERARCHY_IF_EXISTS|CCF_IMAGEGROUPSET|CCF_DONT_INITIALISE_TEXTURES, PBAR_HUD_START+PBAR_HUD_INTERVAL*.5, PBAR_HUD_INTERVAL*.25);

	Set_Progress_Bar_Position(PBAR_HUD_START+PBAR_HUD_INTERVAL);
	//copy_chunks_from_environment(0);

	/*KJL*************************************
	*   Setup generic data for weapons etc   *
	*************************************KJL*/

	InitialiseEquipment();

	InitHUD();
}

void RestartLevel()
{
	//get the cd to start again at the beginning of the play list.
	ResetCDPlayForLevel();

	CleanUpPheromoneSystem();
	// now deallocate the module vis array
	DeallocateModuleVisArrays();

	/* destroy the VDB list */
	InitialiseVDBs();
	InitialiseTxAnimBlocks();


	// deallocate strategy and display blocks
	{
		int i ;

		i = maxstblocks;
		DestroyAllStrategyBlocks();
		while(i--)
		{
			ActiveStBlockList[i] = NULL;
		}

		i = maxobjects;
		InitialiseObjectBlocks();
		while (i --)
		{
			ActiveBlockList[i] = NULL;
		}
	}

	//stop all sound
	SoundSys_StopAll();

	//reset the displayblock for modules to 0
	{
		int i=2;
		while (MainScene.sm_module[i].m_type!=mtype_term)
		{
			MainScene.sm_module[i].m_dptr=0;
			i++;
		}
	}

	// set the Onscreenbloock lsit to zero
	NumOnScreenBlocks = 0;

	//start reinitialising stuff

	/* surely we should init squad data here?
	* funtion calls in create_strategies_from_list(); test data in squad struct
	* but if this hasn't been initialised, it surely just contains garbage?
	* keep an eye on this! 
	*/
	InitSquad();
	
	ProcessSystemObjects();

	create_strategies_from_list();
	AssignAllSBNames();

	SetupVision();
	InitObjectVisibilities();
	InitPheromoneSystem();
	InitHive();
//	InitSquad();

	/* KJL 14:22:41 17/11/98 - reset HUD data, such as where the crosshair is,
	whether the Alien jaw is on-screen, and so on */
	ReInitHUD();

	InitialiseParticleSystem();
	InitialiseSfxBlocks();
	InitialiseLightElementSystem();
	CreateRubberDucks();
	InitialiseTriggeredFMVs();

	CheckCDStatus();

	/*Make sure we don't get a slow frame when we restart , since this can cause problems*/
	ResetFrameCounter();

	CurrentGameStats_Initialise();
	MessageHistory_Initialise();

	if (AvP.Network!=I_No_Network)
	{
		TeleportNetPlayerToAStartingPosition(Player->ObStrategyBlock,1);
	}
	else
	{
		//make sure the visibilities are up to date
		extern VIEWDESCRIPTORBLOCK* Global_VDB_Ptr;
		Global_VDB_Ptr->VDB_World = Player->ObWorld;
		AllNewModuleHandler();
		DoObjectVisibilities();
	}
}

static ELO JunkEnv; /* This is not needed */
ELO* Env_List[I_Num_Environments] = { &JunkEnv };

/**** Construct filename and go for it ***************/
void DestroyActiveBlockList(void);
void InitialiseObjectBlocks(void);

char EnvFileName[100];
char LevelDir[100];

void ProcessSystemObjects()
{
	int i;

	MODULEMAPBLOCK* mmbptr = &AvpCompiledMaps[0];
	STRATEGYBLOCK* sbptr;

	/* PC Loading.
		 1 LoadRif File 
		 			a sets up precompiled shapes
					b	sets up other loaded shapes
					c sets maps ans SBs for loaded maps
					
		 2 
	*/


	#if TestRiffLoaders
	ReadMap(Map);							 /* for chunck loader*/
	ReadModuleMapList(mmbptr);
	#else
	#if SupportModules
	ReadModuleMapList(mmbptr);
	#endif /*SupportModules*/
	ReadMap(Map);	
	#endif

	/*HACK HACK*/

	sbptr = AttachNewStratBlock((MODULE*)NULL,
				(MODULEMAPBLOCK*)&Player_and_Camera_Type8[0],
				Player);

	AssignRunTimeBehaviours(sbptr);

	#if SupportModules

	Global_ModulePtr = MainSceneArray;
	PreprocessAllModules();
	i = GetModuleVisArrays();
	if (i == FALSE) 
	{
		textprint("GetModuleVisArrays() failed\n");
	}

	#endif
}

static int ReadModuleMapList(MODULEMAPBLOCK *mmbptr)
{
	MODULE m_temp;

	DISPLAYBLOCK *dptr;
	STRATEGYBLOCK *sbptr;
	/* this automatically attaches sbs to dbs */

	while (mmbptr->MapType != MapType_Term)
	{
		m_temp.m_mapptr = mmbptr;
		m_temp.m_sbptr = (STRATEGYBLOCK*)NULL;
		m_temp.m_dptr = NULL;
		AllocateModuleObject(&m_temp); 
		dptr = m_temp.m_dptr;
		LOCALASSERT(dptr); /* if this fires, cannot allocate displayblock */
		dptr->ObMyModule = NULL;
		sbptr = AttachNewStratBlock((MODULE*)NULL, mmbptr, dptr);
		/* enable compile in behaviours here */
		AssignRunTimeBehaviours(sbptr);

		mmbptr++;
	}

	return 0;
}

void UnloadRifFile()
{
	unload_rif(env_rif);
}

void ChangeEnvironmentToEnv(I_AVP_ENVIRONMENTS env_to_load)
{
	GLOBALASSERT(env_to_load != AvP.CurrentEnv);

	GLOBALASSERT(Env_List[env_to_load]);

	Destroy_CurrentEnvironment(); 
	/* Patrick: 26/6/97
	Stop and remove all sounds here */
	SoundSys_StopAll();
	SoundSys_RemoveAll(); 
	CDDA_Stop();

	// Loading functions
	AvP.CurrentEnv = env_to_load;
	LoadRifFile();
}


void IntegrateNewEnvironment()
{
	MODULEMAPBLOCK* mmbptr = &AvpCompiledMaps[0];

	// elements we need form processsystemobjects

	ReadMap(Map);                /* for chunck loader*/
	ReadModuleMapList(mmbptr);

	Global_ModulePtr = MainSceneArray;
	PreprocessAllModules();
	if (GetModuleVisArrays() == FALSE)
	{
		textprint("GetModuleVisArrays() failed\n");
	}

	// elements from start game for AI

	InitObjectVisibilities();
	InitPheromoneSystem();
	BuildFarModuleLocs();
	InitHive();

	AssignAllSBNames();

	/* KJL 20:54:55 05/15/97 - setup player vision (alien wideangle, etc) */
	SetupVision();

	UnloadRifFile();//deletes environment File_Chunk since it is no longer needed

	/* Patrick: 26/6/97
	Load our sounds for the new env */	
	LoadSounds("PLAYER");

	/* remove resident loaded 'fast' files */
	ffcloseall();

	ResetFrameCounter();
}

const char *GameDataDirName = "avp_rifs";
const char *FileNameExtension = ".rif";

void LoadRifFile()
{
	char file_and_path[MAX_PATH];
	
	Set_Progress_Bar_Position(PBAR_LEVEL_START);
	
	// clear the dir names
	file_and_path[0] = '\0';
	EnvFileName[0] = '\0';
	LevelDir[0] = '\0';

	// Set up the dirname for the Rif load
	strcpy(file_and_path, GameDataDirName);
	strcat(file_and_path, "/");
	strcat(file_and_path, Env_List[AvP.CurrentEnv]->main);
	strcat(file_and_path, FileNameExtension);

	env_rif = avp_load_rif((const char*)&file_and_path[0]);
	Set_Progress_Bar_Position(PBAR_LEVEL_START+PBAR_LEVEL_INTERVAL*.4);
	
	if (INVALID_RIFFHANDLE == env_rif)
	{
//		finiObjects();
		exit(0x3421);
	};

	#if MaxImageGroups>1
	SetCurrentImageGroup(2); // FOR ENV
	#endif
	copy_rif_data(env_rif, CCF_ENVIRONMENT, PBAR_LEVEL_START+PBAR_LEVEL_INTERVAL*.4, PBAR_LEVEL_INTERVAL*.6);
}

int Destroy_CurrentEnvironment(void)
{
	// RWH destroys all en specific data

	// function to change environment when we 
	// are playing a game	- environmnet reset
	
	// this stores all info we need

	TimeStampedMessage("Beginning Destroy_CurrentEnvironment");
	//CreateLevelMetablocks(AvP.CurrentEnv);
	TimeStampedMessage("After CreateLevelMetablocks");

	/*----------------------Patrick 14/3/97-----------------------
	  Clean up AI systems at end of level
	--------------------------------------------------------------*/

	{
		int i ;

		i = maxstblocks;
		DestroyAllStrategyBlocks();
		while (i--)
		{
			ActiveStBlockList[i] = NULL;
		}

		i = maxobjects;
		InitialiseObjectBlocks();
		while (i --)
		{
			ActiveBlockList[i] = NULL;
		}
	}
	TimeStampedMessage("After object blocks");
	
	//Get rid of all sounds
	//Deallocate memory for all shapes and hierarchy animations
	DeallocateSoundsAndPoolAllocatedMemory();
	
	KillFarModuleLocs();
	TimeStampedMessage("After KillFarModuleLocs");
	CleanUpPheromoneSystem();
	TimeStampedMessage("After CleanUpPheromoneSystem");
	
	#if MaxImageGroups>1
	SetCurrentImageGroup(2); // FOR ENV
	TimeStampedMessage("After SetCurrentImageGroup");

	DeallocateCurrentImages();
	TimeStampedMessage("After DeallocateCurrentImages");
	#endif
	// now deasllocate the module vis array
	DeallocateModuleVisArrays();
	TimeStampedMessage("After DeallocateModuleVisArrays");

	/* destroy the VDB list */
	InitialiseVDBs();
	TimeStampedMessage("After InitialiseVDBs");

	InitialiseTxAnimBlocks(); // RUN THE npcS ON OUR OWN
	TimeStampedMessage("After InitialiseTxAnimBlocks");

	/* frees the memory from the env load*/
	DeallocateModules();
	TimeStampedMessage("After DeallocateModules");

	avp_undo_rif_load(env_rif);
	TimeStampedMessage("After avp_undo_rif_load");

	// set the Onscreenbloock lsit to zero
	NumOnScreenBlocks = 0;

	return 0;
}

// project spec game exit
void ExitGame(void)
{
	if (player_rif != INVALID_RIFFHANDLE)
	{
		avp_undo_rif_load(player_rif);
		player_rif=INVALID_RIFFHANDLE;
	}
	
	if (alien_weapon_rif != INVALID_RIFFHANDLE)
	{
		avp_undo_rif_load(alien_weapon_rif);
		alien_weapon_rif=INVALID_RIFFHANDLE;
	}
	if (marine_weapon_rif != INVALID_RIFFHANDLE)
	{
		avp_undo_rif_load(marine_weapon_rif);
		marine_weapon_rif=INVALID_RIFFHANDLE;
	}
	if (predator_weapon_rif != INVALID_RIFFHANDLE)
	{
		avp_undo_rif_load(predator_weapon_rif);
		predator_weapon_rif=INVALID_RIFFHANDLE;
	}
	#if MaxImageGroups>1
	SetCurrentImageGroup(0);
	DeallocateCurrentImages();
	#endif
}
