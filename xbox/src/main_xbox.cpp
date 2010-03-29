/* Main designed spec for use with windows95*/

#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "stratdef.h"
#include "gamedef.h"
#include "gameplat.h"
#include "bh_types.h"
#include "usr_io.h"
#include "font.h"

/* JH 27/1/97 */
extern "C" {
#include "comp_shp.h"
}

#include "chnkload.hpp"
#include "npcsetup.h" /* JH 30/4/97 */
#include "pldnet.h"
#include "avpview.h"
#include "scrshot.hpp"
#include "language.h"
#include "huddefs.h"
#include "vision.h"
#include "avp_menus.h"
#include "kshape.h"
//#define UseLocalAssert TRUE
#include "ourasert.h"
#include "ffstdio.h" // fast file stdio
#include "davehook.h"
#include "showcmds.h"
#include "consbind.hpp"
#include "AvpReg.hpp"
#include "mempool.h"
#include "GammaControl.h"
#include "avp_intro.h"
#include "CDTrackSelection.h"
#include "CD_Player.h"

/*------------Patrick 1/6/97---------------
New sound system
-------------------------------------------*/
#include "psndplat.h"

#define FRAMEAV 100

#include "AvP_UserProfile.h"
#include "avp_menus.h"
#include "configFile.h"
#include "vorbisPlayer.h"
#include "networking.h"
#include "avpview.h"

/*

 externs for commonly used global variables and arrays

*/
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

#if debug
#define MainTextPrint 1
extern int alloc_cnt, deall_cnt;
extern int ItemCount;
int DebugFontLoaded = 0;
#else
#define MainTextPrint 0
#endif

extern "C" {

extern int PrintDebuggingText(const char* t, ...);
extern int WindowRequestMode;
extern int FrameRate;
extern BOOL ForceLoad_Alien;
extern BOOL ForceLoad_Marine;
extern BOOL ForceLoad_Predator;
extern BOOL ForceLoad_Hugger;
extern BOOL ForceLoad_Queen;
extern BOOL ForceLoad_Civvie;
extern BOOL ForceLoad_PredAlien;
extern BOOL ForceLoad_Xenoborg;
extern BOOL ForceLoad_Pretorian;
extern BOOL ForceLoad_SentryGun;

BOOL UseMouseCentreing = FALSE;
BOOL KeepMainRifFile = FALSE;

char LevelName[] = {"predbit6\0QuiteALongNameActually"};

int VideoModeNotAvailable = 0;
int QuickStartMultiplayer = 1;

extern HWND hWndMain;
extern int WindowMode;
extern int DebuggingCommandsActive;
extern void dx_log_close();
extern void TimeStampedMessage(char *stringPtr);
extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);
extern void ScanImagesForFMVs();
extern void RestartLevel();
extern void ResetEaxEnvironment(void);
extern void ReleaseAllFMVTextures();
extern void MinimalNetCollectMessages(void);
extern void InitCentreMouseThread();
extern void IngameKeyboardInput_ClearBuffer(void);
extern void Game_Has_Loaded(void);
extern void FinishCentreMouseThread();
extern void DoCompletedLevelStatisticsScreen(void);
extern void DeInitialisePlayer();
extern void BuildMultiplayerLevelNameArray();
extern void EmptyUserProfilesList(void);
extern char CommandLineIPAddressString[];
extern int AvP_MainMenus(void);
extern int AvP_InGameMenus(void);
extern int InGameMenusAreRunning(void);
extern void LoadDeviceAndVideoModePreferences();
extern void InitFmvCutscenes();

#include "VideoModes.h"
extern DEVICEANDVIDEOMODE PreferredDeviceAndVideoMode;
extern struct DEBUGGINGTEXTOPTIONS ShowDebuggingText;

int unlimitedSaves = 0;

void exit_break_point_fucntion ()
{
#if debug
	if (WindowMode == WindowModeSubWindow)
	{
		__debugbreak();
	}
#endif
}

extern int GotMouse;
}

static char buf[100];

void _cdecl main()
{
	char command_line[200 + 1];
	char * instr;
	I_AVP_ENVIRONMENTS level_to_load = I_Num_Environments;

//	_controlfp(_PC_24,_MCW_PC); // bjd - CHECK

	InitFmvCutscenes();

	// load AliensVsPredator.cfg
	Config_Load();

	LoadCDTrackList(); //load list of cd tracks assigned to levels , from a text file
	LoadVorbisTrackList(); // do the same for any user ogg vorbis music files

	SetFastRandom();

	avp_GetCommandLineArgs(command_line, 200);

	/****
		init game now ONLY sets up varibles for the whole
		game. If you want to put something in it it must
		be something that only needs to be called once
	****/
	//see if any extra npc rif files should be loaded
	char* strpos = strstr(command_line, "-l");
	if (strpos)
	{
		while (strpos)
		{
			strpos += 2;
			if (*strpos >= 'a' && *strpos <= 'z')
			{
				while (*strpos >= 'a' && *strpos <= 'z')
				{
					switch (*strpos)
					{
						case 'a':
							ForceLoad_Alien = TRUE;
							break;
						case 'm':
							ForceLoad_Marine = TRUE;
							break;
						case 'p':
							ForceLoad_Predator = TRUE;
							break;
						case 'h':
							ForceLoad_Hugger = TRUE;
							break;
						case 'q':
							ForceLoad_Queen = TRUE;
							break;
						case 'c':
							ForceLoad_Civvie = TRUE;
							break;
						case 'x':
							ForceLoad_Xenoborg = TRUE;
							break;
						case 't':
							ForceLoad_Pretorian = TRUE;
							break;
						case 'r':
							ForceLoad_PredAlien = TRUE;
							break;
						case 's':
							ForceLoad_SentryGun = TRUE;
							break;
					}
					strpos++;
				}
			}
			else
			{
				ForceLoad_Alien = TRUE;
			}
			strpos = strstr(strpos, "-l");
		}
	}

	#ifdef AVP_DEBUG_VERSION
	if (strstr(command_line, "-intro"))	WeWantAnIntro();
	if (strstr(command_line, "-qm"))
	{
		QuickStartMultiplayer = 1;
	}
	else if (strstr(command_line, "-qa"))
	{
		QuickStartMultiplayer = 2;
	}
	else if (strstr(command_line, "-qp"))
	{
		QuickStartMultiplayer = 3;
	}
	else
	{
		QuickStartMultiplayer = 0;
	}

	if (strstr(command_line, "-keeprif"))
	{
		KeepMainRifFile = TRUE;
	}
	#endif //AVP_DEBUG_VERSION

	if (strstr(command_line,"-server"))
	{
/*
		extern int DirectPlay_InitLobbiedGame();
		//game has been launched by mplayer , we best humour it
		LobbiedGame=LobbiedGame_Server;
		if(!DirectPlay_InitLobbiedGame())
		{
			//exit(0x6364);
			OutputDebugString("\n line 312 main_xbox.c");
		}
*/
	}
	else if (strstr(command_line, "-client"))
	{
/*
		extern int DirectPlay_InitLobbiedGame();
		//ditto
		LobbiedGame=LobbiedGame_Client;
		if(!DirectPlay_InitLobbiedGame())
		{
			//exit(0x6364);
			OutputDebugString("\n line 323 main_xbox.c");
		}
*/
	}
	else if (strstr(command_line, "-debug"))
	{
		DebuggingCommandsActive = 1;
	}

	if (instr = strstr(command_line, "-ip"))
	{
		char buffer[100];
		extern char CommandLineIPAddressString[];

		sscanf(instr, "-ip %s", &buffer);
		strncpy(CommandLineIPAddressString,buffer,15);
		CommandLineIPAddressString[15] = 0;
	}

 	// Modified by Edmond for mplayer demo
 	#if MPLAYER_DEMO
 	if (!LobbiedGame)
 	{
 		MessageBox(NULL, "This demo can only be launched from Mplayer.", "Oh no!", MB_OK);
// 		exit(33445);
		OutputDebugString("\n line 347 main_xbox.c");
 	}
 	#endif

 	#if PLAY_INTRO//(MARINE_DEMO||ALIEN_DEMO||PREDATOR_DEMO)
  	if (!LobbiedGame)  // Edmond
	 	WeWantAnIntro();
	#endif
	GetPathFromRegistry();

	/* JH 28/5/97 */
	/* Initialise 'fast' file system */
	#if MARINE_DEMO
	ffInit("fastfile\\mffinfo.txt","fastfile\\");
	#elif ALIEN_DEMO
	ffInit("alienfastfile\\ffinfo.txt","alienfastfile\\");
	#else
	ffInit("fastfile\\ffinfo.txt","fastfile\\");
	#endif

	InitGame();

	/****** Put in by John to sort out easy sub window mode ******/
	/****** REMOVE FOR GAME!!!!! ******/

	#if debug && 1//!PREDATOR_DEMO

	if (instr = strstr(command_line, "-s"))
		sscanf(instr, "-s%d", &level_to_load);

	#endif

	Env_List[0]->main = LevelName;

	// as per linux port
	AvP.CurrentEnv = AvP.StartingEnv = I_Gen1;

	/*******	System initialisation **********/

	InitialiseSystem(0,0/*hInstance, nCmdShow*/);
	GotMouse = 1;

	InitialiseRenderer();

	if (!InitialiseDirect3D())
	{
		ReleaseDirect3D();
	}

	LoadDeviceAndVideoModePreferences();

	LoadKeyConfiguration();

	/********** Grab The Video mode **********/
	/* JH - nope, not yet; not until we start the menus
		(or if debugging, start the game), do we need to
		set the initial video mode */

	/*-------------------Patrick 2/6/97-----------------------
	Start the sound system
	----------------------------------------------------------*/
	SoundSys_Start();
	CDDA_Start();

	/* load language file and setup text string access */
	InitTextStrings();

	BuildMultiplayerLevelNameArray();//sort out multiplayer level names

	AvP.LevelCompleted = 0;
	LoadSounds("PLAYER");

	#if PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO
	if (AvP_MainMenus())
	#else

	while (AvP_MainMenus())
	#endif
	{
		int menusActive=0;
		int thisLevelHasBeenCompleted=0;

		mainMenu = 0;

		#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO)
/*
		if(instr = strstr(command_line, "-n"))
		{
			sscanf(instr, "-n %s", &LevelName);
		}
*/
		#endif

		// support removing limit on number of game saves
		if (!Config_GetBool("[Misc]", "UnlimitedSaves", false))
		{
			unlimitedSaves = 0;
		}
		else unlimitedSaves = 1;

		/* turn off any special effects */
		d3d_light_ctrl.ctrl = LCCM_NORMAL;

		/* JH 20/5/97
			The video mode is no longer set when exiting the menus
			(not necessary if user selects EXIT)
			So it is set here */

//		ChangeGameResolution(PreferredDeviceAndVideoMode.Width, PreferredDeviceAndVideoMode.Height, PreferredDeviceAndVideoMode.ColourDepth);

		// Load precompiled shapes
	    start_of_loaded_shapes = load_precompiled_shapes();

		/***********  Load up the character stuff *******/

		InitCharacter();

		LoadRifFile(); /* sets up a map*/
		#if debug
		DebugFontLoaded = 1;
		#endif

		/*********** Process the data ************/
		AssignAllSBNames();
		StartGame();

//		UnloadRifFile();//deletes environment File_Chunk since it is no longer needed


		/* Patrick 26/6/97
		Load the game sounds here: should be done after loading everthing
		else, incase sounds take up system memory */
//		LoadSounds("PLAYER");

		/* JH 28/5/97 */
		/* remove resident loaded 'fast' files */
		ffcloseall();
		/*********** Play the game ***************/

		/* KJL 15:43:25 03/11/97 - run until this boolean is set to 0 */
		AvP.MainLoopRunning = 1;

		ScanImagesForFMVs();

		ResetFrameCounter();
		Game_Has_Loaded();
		ResetFrameCounter();

		if (AvP.Network != I_No_Network)
		{
			/*Need to choose a starting position for the player , but first we must look
			through the network messages to find out which generator spots are currently clear*/
			netGameData.myGameState = NGS_Playing;
			MinimalNetCollectMessages();
			TeleportNetPlayerToAStartingPosition(Player->ObStrategyBlock,1);
		}

		IngameKeyboardInput_ClearBuffer();

		while(AvP.MainLoopRunning)
		{
		 	CheckForWindowsMessages();
			CursorHome();

			#if debug
			if (memoryInitialisationFailure)
			{
				textprint("Initialisation not completed - out of memory!\n");
				GLOBALASSERT(1 == 0);
			}
			#endif

			switch(AvP.GameMode)
			{
				case I_GM_Playing:
				{
					if ((!menusActive || (AvP.Network!=I_No_Network && !netGameData.skirmishMode)) && !AvP.LevelCompleted)
					{
						#if MainTextPrint 		/* debugging stuff */
						{

							if (ShowDebuggingText.FPS) ReleasePrintDebuggingText("FrameRate = %d fps\n",FrameRate);
							if (ShowDebuggingText.Environment) ReleasePrintDebuggingText("Environment %s\n", Env_List[AvP.CurrentEnv]->main);
							if (ShowDebuggingText.Coords) ReleasePrintDebuggingText("Player World Coords: %d,%d,%d\n",Player->ObWorld.vx,Player->ObWorld.vy,Player->ObWorld.vz);

							{
								PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
								PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
								TEMPLATE_WEAPON_DATA *twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];
								if (ShowDebuggingText.GunPos)
								{
									PrintDebuggingText("Gun Position x:%d,y:%d,z:%d\n",twPtr->RestPosition.vx,twPtr->RestPosition.vy,twPtr->RestPosition.vz);
								}
							}
						}
						#endif  /* MainTextPrint */

						// bjd
						ThisFramesRenderingHasBegun();

//						FlushD3DZBuffer();

						DoAllShapeAnimations();

						UpdateGame();

						#if 1
						#if PROFILING_ON
					  	ProfileStart();
						#endif
						AvpShowViews();
						#if PROFILING_ON
					  	ProfileStop("SHOW VIEW");
						#endif

						//Do screen shot here so that text and  hud graphics aren't shown
						#if PROFILING_ON
					  	ProfileStart();
						#endif
						MaintainHUD();
						#if PROFILING_ON
					  	ProfileStop("RENDER HUD");
						#endif

						#if debug
						FlushTextprintBuffer();
						#endif

						//check cd status
						CheckCDAndChooseTrackIfNeeded();

						// check to see if we're pausing the game;
						// if so kill off any sound effects
						if(InGameMenusAreRunning() && ( (AvP.Network!=I_No_Network && netGameData.skirmishMode) || (AvP.Network==I_No_Network))	)
							SoundSys_StopAll();
					}
					else
					{
						ReadUserInput();
//						UpdateAllFMVTextures();
						SoundSys_Management();

						// bjd
						ThisFramesRenderingHasBegun();
					}

					{
						menusActive = AvP_InGameMenus();
						if(AvP.RestartLevel) menusActive=0;
					}
					if (AvP.LevelCompleted)
					{
						SoundSys_FadeOutFast();
						DoCompletedLevelStatisticsScreen();
						thisLevelHasBeenCompleted = 1;
					}

					{
						/* after this call, no more graphics can be drawn until the next frame */
						//extern void ThisFramesRenderingHasFinished(void);
						ThisFramesRenderingHasFinished();
					}

					/* JH 5/6/97 - this function may draw translucent polygons to the whole screen */
					//HandleD3DScreenFading();
					#else
					{

					ColourFillBackBuffer(0);
					}
					#endif

					FlipBuffers();

					FrameCounterHandler();
					{
						PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);

						if (!menusActive && playerStatusPtr->IsAlive && !AvP.LevelCompleted)
						{
							DealWithElapsedTime();
						}
					}
					break;
				}
				case I_GM_Menus:
				{
					AvP.GameMode = I_GM_Playing;
					//StartGameMenus();
					LOCALASSERT(AvP.Network == I_No_Network);
					//AccessDatabase(0);
					break;
				}
				#if 0
				case I_GM_Paused:
				{
					{
						extern void DoPcPause(void);
						DoPcPause();
					}
					break;
				}
				#endif

				default:
				{
					GLOBALASSERT(2<1);
					break;
				}
			}

			if(AvP.RestartLevel)
			{
				AvP.RestartLevel=0;
				AvP.LevelCompleted = 0;
				FixCheatModesInUserProfile(UserProfilePtr);
				RestartLevel();
			}


		}// end of main game loop

		AvP.LevelCompleted = thisLevelHasBeenCompleted;
		mainMenu = 1;

		FixCheatModesInUserProfile(UserProfilePtr);

		#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO)
		TimeStampedMessage("We're out of the main loop");

		ReleaseAllFMVTextures();

		/* DHM 8/4/98 */
		CONSBIND_WriteKeyBindingsToConfigFile();

		/* CDF 2/10/97 */
		DeInitialisePlayer();

		DeallocatePlayersMirrorImage();

		// bjd - here temporarily as a test
		SoundSys_StopAll();

		Destroy_CurrentEnvironment();

		DeallocateAllImages();

		EndNPCs(); /* JH 30/4/97 - unload npc rifs */

		ExitGame();

		#endif
		/* Patrick 26/6/97
		Stop and remove all game sounds here, since we are returning to the menus */
//		SoundSys_StopAll();
		ResetEaxEnvironment();
		//make sure the volume gets reset for the menus
		SoundSys_ResetFadeLevel();

		CDDA_Stop();
		Vorbis_CloseSystem(); // stop ogg vorbis player

		// netgame support
		if(AvP.Network != I_No_Network)
		{
			/* we cleanup and reset our game mode here, at the end of the game loop, as other
			clean-up functions need to know if we've just exited a netgame */
			EndAVPNetGame();
			//EndOfNetworkGameScreen();
		}

		//need to get rid of the player rifs before we can clear the memory pool

		ClearMemoryPool();

		if(LobbiedGame)
		{
			/*
			We have been playing a lobbied game , and have now diconnected.
			Since we can't start a new multiplayer game , exit to avoid confusion
			*/
			break;
		}
	}
	#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO)
	TimeStampedMessage("After Menus");

	/* Added 28/1/98 by DHM: hook for my code on program shutdown */
	{
		DAVEHOOK_UnInit();
	}
	TimeStampedMessage("After DAVEHOOK_UnInit");

	/*-------------------Patrick 2/6/97-----------------------
	End the sound system
	----------------------------------------------------------*/
	SoundSys_StopAll();
  	SoundSys_RemoveAll();

	/* bjd - delete some profile data that was showing up as memory leaks */
	EmptyUserProfilesList();

	#else
	QuickSplashScreens();
	#endif
	#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO)

	ExitSystem();

	Config_Save();

	#else
	SoundSys_End();
	ReleaseDirect3D();

	/* Kill windows procedures */
	ExitWindowsSystem();

	#endif
 	CDDA_End();
	ClearMemoryPool();

	/* bye bye! */
}

void CheckForWindowsMessages()
{
	DirectSoundDoWork();
}

BOOL ExitWindowsSystem(void)
{
 // do nothing here
	return 0;
}

BOOL InitialiseWindowsSystem(HINSTANCE hInstance, int nCmdShow, int WinInitMode)
{
	return 1;
}