
#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "strategy_def.h"
#include "gamedef.h"
#include "gameplat.h"
#include "bh_types.h"
#include "user_io.h"
#include <mmsystem.h>
#include "renderer.h"
#include "compiled_shapes.h"
#include "chunk_load.hpp"
#include "npcsetup.h" /* JH 30/4/97 */
#include "pldnet.h"
#include "view.h"
#include "vision.h"
#include "menus.h"
#include "ourasert.h" 
#include "davehook.h"
#include "showcmds.h"
#include "console_bind.hpp"
#include "mempool.h"
#include "gammacontrol.h"
#include "intro.h"
#include "MusicPlayer.h"
#include "psndplat.h"
#include "user_profile.h"
#include "menus.h"
#include "configFile.h"
#include "VorbisPlayer.h"
#include "networking.h"
#include "view.h"
#include "renderer.h"
#include "mp_config.h"
#include "logString.h"
#include "FastFile.h"
#include "console.h"

#if debug
#define MainTextPrint 1
extern int alloc_cnt, deall_cnt;
extern int ItemCount;
#else
#define MainTextPrint 0
#endif

extern int PrintDebuggingText(const char* t, ...);
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
extern void Game_Has_Loaded();
extern void FinishCentreMouseThread();
extern void DoCompletedLevelStatisticsScreen(void);
extern void DeInitialisePlayer();
extern void BuildMultiplayerLevelNameArray();
extern char CommandLineIPAddressString[];
extern int AvP_MainMenus(void);
extern int AvP_InGameMenus(void);
extern int InGameMenusAreRunning(void);
extern void InitFmvCutscenes();
extern void ChangeWindowsSize(uint32_t width, uint32_t height); 
extern void DetermineGameVersion();

extern struct DEBUGGINGTEXTOPTIONS ShowDebuggingText;

extern bool bRunning;

bool unlimitedSaves = false;

extern bool IsDemoVersion();
extern texID_t AAFontImageNumber;

void exit_break_point_function()
{
	#if debug
	if (WindowMode == WindowModeSubWindow)
	{
		__debugbreak();
	}
	#endif
}

//#define _ALLOC_CONSOLE

extern void LoadKeyConfiguration();
extern bool InitialiseWindowsSystem(HINSTANCE hInstance, int nCmdShow, int WinInitMode);

// so we can disable/enable stickey keys
STICKYKEYS startupStickyKeys = {sizeof(STICKYKEYS), 0};
STICKYKEYS skOff;

// entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	SetWindowTitle();
	ClearLog();
	LogString(GetWindowTitle());

	I_AVP_ENVIRONMENTS level_to_load = I_Num_Environments;
	char *command_line = lpCmdLine;
	char *instr = 0;

#ifdef _ALLOC_CONSOLE
	AllocConsole();
	freopen("CONOUT$", "wb", stdout);
#endif

	skOff = startupStickyKeys;

	if ((skOff.dwFlags & SKF_STICKYKEYSON) == 0)
	{
		// Disable the hotkey and the confirmation
		skOff.dwFlags &= ~SKF_HOTKEYACTIVE;
		skOff.dwFlags &= ~SKF_CONFIRMHOTKEY;

		SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &skOff, 0);
	}

	DetermineGameVersion();

	InitFmvCutscenes();

	// load AliensVsPredator.cfg
	Config_Load();

	Music_Init(); //load list of music tracks assigned to levels , from a text file
	LoadVorbisTrackList(); // do the same for any user ogg vorbis music files

	SetFastRandom();

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
							printf("ForceLoad_Alien set to true\n");
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
				printf("ForceLoad_Alien set to true\n");
			}
			strpos = strstr(strpos, "-l");
		}
	}

	if (strstr(command_line, "-intro"))	{
		WeWantAnIntro();
	}

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

	if (strstr(command_line, "-m"))
	{
		UseMouseCentreing = TRUE;
	}

	UseMouseCentreing = TRUE;

	// windowed mode?
	if (strstr(command_line, "-w"))
	{
		WindowMode = WindowModeSubWindow;

		// will stop mouse cursor moving outside game window
		//UseMouseCentreing = TRUE;
	}

	if (strstr(command_line, "-dontgrabmouse"))
	{
		// if this was previously set due to -m or -w, disable it
		UseMouseCentreing = FALSE;
	}

	if (UseMouseCentreing)
	{
		InitCentreMouseThread();
	}

//	#endif //AVP_DEBUG_VERSION

	if (strstr(command_line, "-server"))
	{
		//game has been launched by mplayer , we best humour it
		LobbiedGame = LobbiedGame_Server;
		if (!Net_InitLobbiedGame())
		{
			exit(0x6364);
		}
	}
	else if (strstr(command_line, "-client"))
	{
		//ditto
		LobbiedGame = LobbiedGame_Client;
		if (!Net_InitLobbiedGame())
		{
			exit(0x6364);
		}
	}
	else if (strstr(command_line, "-debug"))
	{
		DebuggingCommandsActive = 1;
	}

	if (instr = strstr(command_line, "-ip"))
	{
		char buffer[100];
		sscanf(instr, "-ip %s", buffer);
		strncpy(CommandLineIPAddressString,buffer,15);
		CommandLineIPAddressString[15] = 0;
	}

	#if PLAY_INTRO//(MARINE_DEMO||ALIEN_DEMO||PREDATOR_DEMO)
	if (!LobbiedGame) { // Edmond
		WeWantAnIntro();
	}
	#endif

	// Initialise 'fast' file system
	FF_Init();

	InitGame();

	/****** Put in by John to sort out easy sub window mode ******/
	/****** REMOVE FOR GAME!!!!! ******/

	#if debug && 1//!PREDATOR_DEMO

	if (instr = strstr(command_line, "-s")) {
		sscanf(instr, "-s%d", &level_to_load);
	}

	#endif

	Env_List[0]->main = LevelName;

	// as per linux port
	AvP.CurrentEnv = AvP.StartingEnv = I_Gen1;

	/******* System initialisation **********/

	timeBeginPeriod(1);

	// Initialise main window, windows procedure etc
	InitialiseWindowsSystem(hInstance, nCmdShow, WinInitFull);
	InitialiseSystem();
	InitialiseRenderer();

	if (!InitialiseDirect3D())
	{
		// the device might have actually created OK, but maybe we are missing an important shader, so release
		// this first so we can exit fullscreen mode if needed
		ReleaseDirect3D();

		ChangeWindowsSize(1, 1);

		// give the user back their mouse cursor
		if (UseMouseCentreing)
		{
			FinishCentreMouseThread();
		}

		std::string error = "Failed to initialise Direct3D backend.\n" + GetLastErrorMessage();
		MessageBox(hWndMain, error.c_str(), "Couldn't create render device!", MB_OK | MB_ICONSTOP);
		exit(-1);
	}

	// load common font
	if (IsDemoVersion()) {
		AAFontImageNumber = Tex_CreateFromRIM("graphics/Menus/smallfont.RIM");
	}
	else {
		AAFontImageNumber = Tex_CreateFromRIM("graphics/Common/aa_font.RIM");
	}

	extern void CalculateWidthsOfAAFont();
	CalculateWidthsOfAAFont();

	// call a function to remove the red grid from the small font texture- only if the texture is loaded
	if (MISSING_TEXTURE != AAFontImageNumber)
	{
		DeRedTexture((Texture)Tex_GetTextureDetails(AAFontImageNumber));
	}

	// load slider graphics
	AVPMENUGFX_SLIDERBAR       = Tex_CreateFromRIM("graphics/Menus/SliderBar.RIM");
	AVPMENUGFX_SLIDER          = Tex_CreateFromRIM("graphics/Menus/Slider.RIM");

	Con_Init();

	Net_Initialise();

	LoadKeyConfiguration();

	/*-------------------Patrick 2/6/97-----------------------
	Start the sound system
	----------------------------------------------------------*/
	SoundSys_Start();
	Music_Start();

	// get rid of the mouse cursor
	SetCursor(NULL);

	// load language file and setup text string access
	InitTextStrings();

	BuildMultiplayerLevelNameArray(); //sort out multiplayer level names

	AvP.LevelCompleted = 0;
	LoadSounds("PLAYER");

	#if PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO
	if(AvP_MainMenus())
	#else

	// support removing limit on number of game saves
	unlimitedSaves = Config_GetBool("[Misc]", "UnlimitedSaves", false);

	while (AvP_MainMenus() && bRunning)
	#endif
	{
		// start of level load
		BOOL menusActive = FALSE;
		int thisLevelHasBeenCompleted = 0;

		mainMenu = false;

		#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO)
		if (instr = strstr(command_line, "-n"))
		{
			sscanf(instr, "-n %s", LevelName);
		}
		#endif

		// turn off any special effects
		d3d_light_ctrl.ctrl = LCCM_NORMAL;

		/* Check Gamma Settings are correct after video mode change */
		InitialiseGammaSettings(RequestedGammaSetting);

		// Load precompiled shapes 
		start_of_loaded_shapes = load_precompiled_shapes();

		/***********  Load up the character stuff *******/
		InitCharacter();

		LoadRifFile(); /* sets up a map*/

		/*********** Process the data ************/
		AssignAllSBNames();
		StartGame();

		/* JH 28/5/97 */
		/* remove resident loaded 'fast' files */
//		ffcloseall();
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

		while (AvP.MainLoopRunning && bRunning) 
		{
			CheckForWindowsMessages();

			CursorHome();

			#if debug
			if (memoryInitialisationFailure)
			{
				OutputDebugString("Initialisation not completed - out of memory!\n");
				textprint("Initialisation not completed - out of memory!\n");
				GLOBALASSERT(1 == 0);
			}
			#endif

			switch (AvP.GameMode)
			{
				case I_GM_Playing:
				{
					if ((!menusActive || (AvP.Network != I_No_Network && !netGameData.skirmishMode)) && !AvP.LevelCompleted)
					{
						//#if MainTextPrint 		/* debugging stuff */
						{
							if (ShowDebuggingText.FPS) ReleasePrintDebuggingText("FrameRate = %d fps\n",FrameRate);
							if (ShowDebuggingText.Environment) ReleasePrintDebuggingText("Environment %s\n", Env_List[AvP.CurrentEnv]->main);
							if (ShowDebuggingText.Coords) ReleasePrintDebuggingText("Player World Coords: %d,%d,%d\n",Player->ObWorld.vx,Player->ObWorld.vy,Player->ObWorld.vz);
							{
								PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
								PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
								TEMPLATE_WEAPON_DATA *twPtr = &TemplateWeapon[weaponPtr->WeaponIDNumber];
								if (ShowDebuggingText.GunPos)
								{
									PrintDebuggingText("Gun Position x:%d,y:%d,z:%d\n",twPtr->RestPosition.vx,twPtr->RestPosition.vy,twPtr->RestPosition.vz);
								}
							}
						}
						//#endif  /* MainTextPrint */

						const int FRAMES_PER_SECOND = 60;
						const int SKIP_TICKS = 1000 / FRAMES_PER_SECOND;

						ThisFramesRenderingHasBegun();
						
						DoAllShapeAnimations();

						UpdateGame();

						AvpShowViews();

						// Do screen shot here so that text and hud graphics aren't shown
						MaintainHUD();

						FlushTextprintBuffer();

						// check music status
						Music_Check();

						// check to see if we're pausing the game;
						// if so kill off any sound effects
						if (InGameMenusAreRunning() && ((AvP.Network != I_No_Network && netGameData.skirmishMode) || (AvP.Network == I_No_Network))) {
							SoundSys_StopAll();
						}
					}
					else
					{
						ReadUserInput();
						SoundSys_Management();

						ThisFramesRenderingHasBegun();
					}

					// draw ingame menu
					menusActive = AvP_InGameMenus();
					if (AvP.RestartLevel) menusActive = FALSE;

					if (AvP.LevelCompleted)
					{
						SoundSys_FadeOutFast();
						DoCompletedLevelStatisticsScreen();
						thisLevelHasBeenCompleted = 1;
					}

					/* after this call, no more graphics can be drawn until the next frame */
					ThisFramesRenderingHasFinished();

					FlipBuffers();

					FrameCounterHandler();
					{
						PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);

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
					LOCALASSERT(AvP.Network == I_No_Network);
					break;
				}

				default:
				{
					GLOBALASSERT(2<1);
					break;
				}
			}

			if (AvP.RestartLevel)
			{
				AvP.RestartLevel = 0;
				AvP.LevelCompleted = 0;
				FixCheatModesInUserProfile(UserProfilePtr);
				RestartLevel();
			}
		} 
		// end of main game loop

		AvP.LevelCompleted = thisLevelHasBeenCompleted;
		mainMenu = true;

		FixCheatModesInUserProfile(UserProfilePtr);

		#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO)
		TimeStampedMessage("We're out of the main loop");

		ReleaseAllFMVTextures();

		CONSBIND_WriteKeyBindingsToConfigFile();

		DeInitialisePlayer();

		DeallocatePlayersMirrorImage();

		// bjd - here temporarily as a test
		SoundSys_StopAll();

		Destroy_CurrentEnvironment();

		EndNPCs(); /* JH 30/4/97 - unload npc rifs */
		ExitGame();

		#endif

		ResetEaxEnvironment();

		// make sure the volume gets reset for the menus
		SoundSys_ResetFadeLevel();

		Music_Stop();
		Vorbis_CloseSystem(); // stop ogg vorbis player

		// netgame support
		if (AvP.Network != I_No_Network)
		{
			/* we cleanup and reset our game mode here, at the end of the game loop, as other 
			clean-up functions need to know if we've just exited a netgame */
			EndAVPNetGame();
			//EndOfNetworkGameScreen();
		}

		ClearMemoryPool();

#if 0 //bjd - FIXME
		if (LobbiedGame)
		{
			/*
			We have been playing a lobbied game , and have now diconnected.
			Since we can't start a new multiplayer game , exit to avoid confusion
			*/
			break;
		}
#endif
	}
	#if !(PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO)
	TimeStampedMessage("After Menus");

	/* Added 28/1/98 by DHM: hook for my code on program shutdown */
	DAVEHOOK_UnInit();

	EmptyUserProfilesList();
	ClearMultiplayerLevelNameArray();

	/*-------------------Patrick 2/6/97-----------------------
	End the sound system
	----------------------------------------------------------*/

	SoundSys_StopAll();
	SoundSys_RemoveAll(); 

	#else
//	QuickSplashScreens();
	#endif

	Tex_Release(AAFontImageNumber);
	Tex_Release(AVPMENUGFX_SLIDERBAR);
	Tex_Release(AVPMENUGFX_SLIDER);

	ExitSystem();

	Config_Save();

	// close dx logfile if open (has to be called after all calls to TimeStampedMessage()
#if debug
	dx_log_close();
#endif

	Music_Deinit();
	ClearMemoryPool();

	// restore stickey keys setting
	SystemParametersInfo(SPI_SETSTICKYKEYS, sizeof(STICKYKEYS), &startupStickyKeys, 0);

	// 'shutdown' timer
	timeEndPeriod(1);

	if (UseMouseCentreing)
	{
		FinishCentreMouseThread();
	}

	return 0;
}
