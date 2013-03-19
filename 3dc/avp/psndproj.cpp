/* Patrick 5/6/97 -------------------------------------------------------------
  AvP Project sound source
  ----------------------------------------------------------------------------*/
#include "3dc.h"
#include "module.h"
#include "inline.h"
#include "strategy_def.h"
#include "gamedef.h"
#include "gameplat.h"
#include "bh_types.h"
#include "inventory.h"
#include "weapons.h"
#include "psnd.h"
#include "psndplat.h"
#include "menus.h"
#include "scream.h"
#include "io.h"
#define UseLocalAssert TRUE
#include "ourasert.h"
#include "db.h"
#include "dxlog.h"
#include "FileStream.h"

int ExtractWavFile(int soundNum, FileStream &fStream);

#define PRED_PISTOL_PITCH_CHANGE 300

#define CDDA_TEST       FALSE
#define CD_VOLUME_TEST  FALSE
#define SOUND_TEST_3D   FALSE

#define USE_REBSND_LOADERS  (LOAD_USING_FASTFILES||PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO||DEATHMATCH_DEMO)
#define USE_COMMON_FLL_FILE  (LOAD_USING_FASTFILES||PREDATOR_DEMO||MARINE_DEMO||ALIEN_DEMO||DEATHMATCH_DEMO)

char * SecondSoundDir = 0;
static const char *FirstSoundDir = "sound/";

/* Andy 9/6/97 ----------------------------------------------------------------
  Internal globals  
-----------------------------------------------------------------------------*/
int weaponHandle = SOUND_NOACTIVEINDEX;

#if 0
static int weaponReloading = 0;
static int backgroundHandle = SOUND_NOACTIVEINDEX;
#endif
static int sadarReloadTimer = 0;
static int weaponPitchTimer = 0;
static int playOneShotWS = 1;
static int oldRandomValue = -1;

#if SOUND_TEST_3D
static int testLoop = SOUND_NOACTIVEINDEX;
#endif

#if CDDA_TEST
static int doneCDDA = 0;
#endif

/* Has the player made a noise? */
int playerNoise;

/* Patrick 5/6/97 -------------------------------------------------------------
  External refernces
  ----------------------------------------------------------------------------*/
extern ACTIVESOUNDSAMPLE ActiveSounds[];

/* Patrick 5/6/97 -------------------------------------------------------------
  Function definitions 
  ----------------------------------------------------------------------------*/

/* Patrick 16/6/97 ----------------------------------------------------------------
  A.N.Other background sound management function  
------------------------------------------------------------------------------*/

void DoPlayerSounds(void)
{
	PLAYER_STATUS *playerStatusPtr;
	PLAYER_WEAPON_DATA *weaponPtr;

	#if CDDA_TEST
	if (doneCDDA == 0)
	{
		CDDA_SwitchOn();

		doneCDDA = 1;

		if (AvP.PlayerType == I_Marine) 				CDDA_Play(CDTrack1);
		else if (AvP.PlayerType == I_Predator)	CDDA_Play(CDTrack3);
		else if (AvP.PlayerType == I_Alien) 		CDDA_Play(CDTrack2);

	}
	#endif

	#if CD_VOLUME_TEST
	{
		static int CDVolume = CDDA_VOLUME_DEFAULT;

		if (!CDDA_IsPlaying()) CDDA_Play(CDTrack1);
		if (KeyboardInput[KEY_L])
		{
			CDVolume++;
			CDDA_ChangeVolume(CDVolume);
		}
		else if(KeyboardInput[KEY_K])
		{
			CDVolume--;
			CDDA_ChangeVolume(CDVolume);
		}

		int currentSetting = CDDA_GetCurrentVolumeSetting();
		textprint("CD VOL: %d \n", currentSetting);
	}
	#endif

	#if SOUND_TEST_3D
	if(testLoop == SOUND_NOACTIVEINDEX)
	{
		VECTORCH zeroLoc = {0,0,0};
		Sound_Play(SID_VISION_LOOP,"del",&zeroLoc,&testLoop);
	}
	#endif

	/* do weapon sound */
	/* access the extra data hanging off the strategy block */
	playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);
	GLOBALASSERT(playerStatusPtr);

	/* init a pointer to the weapon's data */
	weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);

	if (sadarReloadTimer)
	{
		sadarReloadTimer -= NormalFrameTime;
		if (sadarReloadTimer <= 0)
		{
			sadarReloadTimer = 0;
			playerNoise = 1;
		}
	}

	switch (weaponPtr->WeaponIDNumber)
	{
		case WEAPON_PRED_PISTOL:
		{
			if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if (weaponHandle == SOUND_NOACTIVEINDEX) 
				{
					//Sound_Play(SID_PULSE_START,"hp",-PRED_PISTOL_PITCH_CHANGE);
					//Sound_Play(SID_PULSE_LOOP,"elhp",&weaponHandle,-PRED_PISTOL_PITCH_CHANGE);	
					//weaponPitchTimer=ONE_FIXED>>3;
					Sound_Play(SID_PRED_PISTOL,"h");
					playerNoise=1;
				}
				else
				{
					//weaponPitchTimer-=NormalFrameTime;
					//if (weaponPitchTimer<=0)
					//{
					//	weaponPitchTimer=ONE_FIXED>>3;
					//	Sound_ChangePitch(weaponHandle,(FastRandom()&63)-(32+PRED_PISTOL_PITCH_CHANGE));
					//}
				}
			}
			else
			{
				//if(weaponHandle != SOUND_NOACTIVEINDEX)
				//{
		          //if (ActiveSounds[weaponHandle].soundIndex == SID_PULSE_LOOP)
		          //{
			      //    Sound_Play(SID_PULSE_END,"hp",-PRED_PISTOL_PITCH_CHANGE);
				  //	  Sound_Stop(weaponHandle);
				  //}
				//}
			}
		break;
	}

		case WEAPON_PULSERIFLE:
		{
			if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if (weaponHandle == SOUND_NOACTIVEINDEX) 
				{
					Sound_Play(SID_PULSE_START, "h");
					Sound_Play(SID_PULSE_LOOP, "elh", &weaponHandle);
					playerNoise = 1;
					weaponPitchTimer = ONE_FIXED>>3;
				}
				else
				{
					weaponPitchTimer -= NormalFrameTime;
					if (weaponPitchTimer<=0)
					{
						weaponPitchTimer = ONE_FIXED>>3;
						Sound_ChangePitch(weaponHandle, (FastRandom()&63)-32);
						playerNoise = 1;
					}
				}
			}
		else if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_SECONDARY)
		{
			if (weaponHandle == SOUND_NOACTIVEINDEX)
			{
				Sound_Play(SID_NADEFIRE, "h");
				playerNoise = 1;
			}
		}
		else
			{
				if (weaponHandle != SOUND_NOACTIVEINDEX)
				{
					Sound_Play(SID_PULSE_END,"h");
					Sound_Stop(weaponHandle);
				}
			}
		break;
	}

	case WEAPON_FLAMETHROWER:
		{
			if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if (weaponHandle == SOUND_NOACTIVEINDEX)
				{
					Sound_Play(SID_INCIN_START, "h");
					Sound_Play(SID_INCIN_LOOP, "elh", &weaponHandle);
					playerNoise = 1;
			}
		}
			else
			{
				if (weaponHandle != SOUND_NOACTIVEINDEX)
				{
					Sound_Play(SID_INCIN_END, "h");
					Sound_Stop(weaponHandle);
				}
			}
		break;
		}

	case (WEAPON_MINIGUN):
	{
		if (PlayerStatusPtr->IsAlive==0)
		{
			if (weaponHandle != SOUND_NOACTIVEINDEX)
			{
				Sound_Play(SID_MINIGUN_END, "h");
				Sound_Stop(weaponHandle);
			}
		}
		break;
	}

	case WEAPON_AUTOSHOTGUN:
	{
		if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
		{
			Sound_Play(SID_SHOTGUN, "h");
			playerNoise = 1;
		}
		break;
	}

	case WEAPON_MARINE_PISTOL:
	case WEAPON_TWO_PISTOLS:
	{
		if ((weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			||(weaponPtr->CurrentState == WEAPONSTATE_FIRING_SECONDARY))
		{
			Sound_Play(SID_SHOTGUN, "h");
			playerNoise = 1;
		}
		break;
	}

	case WEAPON_SADAR:
	{
		if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
		{
			Sound_Play(SID_SADAR_FIRE, "h");
			playerNoise = 1;
		}
		break;
	}

	case (WEAPON_FRISBEE_LAUNCHER):
	{
		if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
		{
			if (weaponPtr->StateTimeOutCounter == WEAPONSTATE_INITIALTIMEOUTCOUNT)
			{
				playerNoise = 1;
				if (weaponHandle == SOUND_NOACTIVEINDEX)
				{
					Sound_Play(SID_ED_SKEETERCHARGE,"eh",&weaponHandle);
				}
			}
			else
			{
				if (weaponHandle == SOUND_NOACTIVEINDEX)
				{
					playerNoise = 0;
				}
			}
		}
		else
		{
			if (weaponHandle != SOUND_NOACTIVEINDEX)
			{
				Sound_Stop(weaponHandle);
			}
		}
		break;
	}
		case WEAPON_SMARTGUN:
		{
			if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if (weaponHandle == SOUND_NOACTIVEINDEX)
				{
					unsigned int rand=FastRandom() % 3;
			if (rand == oldRandomValue) rand=(rand + 1) % 3;
			oldRandomValue = rand;
			playerNoise=1;
			switch (rand)
			{
				case 0:
				{
				Sound_Play(SID_SMART1,"ehp",&weaponHandle,(FastRandom()&255)-128);
				break;
				}
				case 1:
				{
				Sound_Play(SID_SMART2,"ehp",&weaponHandle,(FastRandom()&255)-128);
				break;
				}
				case 2:
				{
					Sound_Play(SID_SMART3,"ehp",&weaponHandle,(FastRandom()&255)-128);
					break;
				}
					default:
						{
							break;
						}
					}
				}
		}
			break;
		}

		case WEAPON_GRENADELAUNCHER:
		{
			if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if (playOneShotWS)
				{
					Sound_Play(SID_ROCKFIRE,"h");
					playerNoise=1;
					playOneShotWS = 0;
				}
			}
			else playOneShotWS = 1;
			break;
		}

		case WEAPON_PRED_WRISTBLADE:
		{
			break;
		}

		case WEAPON_ALIEN_CLAW:
		{
			break;
		}

		case WEAPON_ALIEN_GRAB:
		{
			break;
		}

		case WEAPON_PRED_RIFLE:
		{
			if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if (playOneShotWS)
				{
					Sound_Play(SID_PRED_LASER,"hp",(FastRandom()&255)-128);
					playerNoise=1;
					playOneShotWS = 0;
				}
			}
			else playOneShotWS = 1;
			break;
		}

		case WEAPON_PRED_SHOULDERCANNON:
		{
			if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if (playOneShotWS)
				{
					//Sound_Play(SID_PRED_LAUNCHER,"hp",(FastRandom()&255)-128);
					playOneShotWS = 0;
				}
			}
			else playOneShotWS = 1;
			break;
		}

		case WEAPON_PRED_DISC:
		{
			if(weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if(playOneShotWS)
				{
					Sound_Play(SID_PRED_FRISBEE,"hp",(FastRandom()&255)-128);
					playerNoise=1;
					playOneShotWS = 0;
				}
			}
			else playOneShotWS = 1;
			break;
		}

		case WEAPON_ALIEN_SPIT:
		{
			if (weaponPtr->CurrentState == WEAPONSTATE_FIRING_PRIMARY)
			{
				if (playOneShotWS)
				{
					Sound_Play(SID_ACID_SPRAY,"hp",(FastRandom()&255)-128);
					playerNoise=1;
					playOneShotWS = 0;
				}
			}
			else playOneShotWS = 1;
			break;
		}
		default:
		{
			break;
		}
	}
}

static int SpotEffectWeaponHandle = SOUND_NOACTIVEINDEX;
void PlayWeaponClickingNoise(enum WEAPON_ID weaponIDNumber)
{
	if (SpotEffectWeaponHandle != SOUND_NOACTIVEINDEX)
		return;
	
	switch (weaponIDNumber)
	{
		// Marine weapons
		case WEAPON_PULSERIFLE:
		{
			Sound_Play(SID_PULSE_RIFLE_FIRING_EMPTY,"eh",&SpotEffectWeaponHandle);
			break;
		}
		case WEAPON_SMARTGUN:
		{
			Sound_Play(SID_NOAMMO,"eh",&SpotEffectWeaponHandle);
			break;
		}
		case WEAPON_MINIGUN:
		{
			#if 0
			Sound_Play(SID_MINIGUN_EMPTY,"eh",&SpotEffectWeaponHandle);
			#endif
			break;
		}
		// Predator weapons
		case WEAPON_PRED_RIFLE:
		{
			Sound_Play(SID_PREDATOR_SPEARGUN_EMPTY,"eh",&SpotEffectWeaponHandle);
			break;
		}

		default:
			break;
	}
}

void MakeRicochetSound(VECTORCH *position)
{
	switch (NormalFrameTime & 0x3)
	{
		case 0:
			Sound_Play(SID_RICOCH1,"pd",((FastRandom()&255)-128),position);
			break;
		case 1:
			Sound_Play(SID_RICOCH2,"pd",((FastRandom()&255)-128),position);
			break;
		case 2:
			Sound_Play(SID_RICOCH3,"pd",((FastRandom()&255)-128),position);
			break;
		case 3:
			Sound_Play(SID_RICOCH4,"pd",((FastRandom()&255)-128),position);
			break;
		default:
			break;
	}
}

void PlayAlienSwipeSound(void)
{
	PlayAlienSound(0, ASC_Swipe,((FastRandom()&255)-128), &weaponHandle, NULL);
}

void PlayAlienTailSound(void)
{
	PlayAlienSound(0, ASC_TailSound, ((FastRandom()&255)-128), &weaponHandle, NULL);
}

void PlayPredSlashSound(void)
{
	PlayPredatorSound(0, PSC_Swipe, ((FastRandom()&255)-128), &weaponHandle, NULL);
}

void PlayCudgelSound(void)
{
	unsigned int rand=FastRandom() % 4;
	if (rand == oldRandomValue) rand = (rand + 1) % 4;
	oldRandomValue = rand;
	switch (rand)
	{
		case 0:
		{
			Sound_Play(SID_PULSE_SWIPE01,"ehp",&weaponHandle,(FastRandom()&255)-128);
			break;
		}
		case 1:
		{
			Sound_Play(SID_PULSE_SWIPE02,"ehp",&weaponHandle,(FastRandom()&255)-128);
			break;
		}
		case 2:
		{
			Sound_Play(SID_PULSE_SWIPE03,"ehp",&weaponHandle,(FastRandom()&255)-128);
			break;
		}
		case 3:
		{
			Sound_Play(SID_PULSE_SWIPE04,"ehp",&weaponHandle,(FastRandom()&255)-128);
			break;
		}
		default:
		{
			break;
		}
	}
}

int FindAndLoadWavFile(int soundNum, const char* wavFileName)
{
	std::string fileName = FirstSoundDir;
	fileName += wavFileName;

	Util::ChangeSlashes(fileName);

	return LoadWavFile(soundNum, fileName);
}

/* Patrick 5/6/97 -------------------------------------------------------------
  Sound data loaders 
  ----------------------------------------------------------------------------*/

void LoadSounds(char *soundDirectory)
{
	int soundIndex;
	int8_t pitch;

	LOCALASSERT(soundDirectory);

	/* first check that sound has initialised and is turned on */
	if (!SoundSys_IsOn())
		return;

	char filename[MAX_PATH];

#if ALIEN_DEMO
	strcpy(filename, "./alienfastfile");
#else
	strcpy(filename, "./fastfile");
#endif
	strcat(filename, "/common.ffl");

	FileStream fStream;
	fStream.Open(filename, FileStream::FileRead, FileStream::SkipFastFileCheck);

	soundIndex = fStream.GetByte();
	pitch      = fStream.GetByte();

	while ((soundIndex != 0xff) || (pitch != -1))
	{
		if ((soundIndex < 0) || (soundIndex >= SID_MAXIMUM))
		{
			/* invalid sound number */
			LOCALASSERT("Invalid Sound Index"==0);
		}
		if (GameSounds[soundIndex].loaded)
		{
			/* Duplicate game sound loaded */
			LOCALASSERT("Duplicate game sound loaded"==0);
		}

		ExtractWavFile(soundIndex, fStream);

		GameSounds[soundIndex].loaded = 1;
		GameSounds[soundIndex].activeInstances = 0;
		GameSounds[soundIndex].volume = VOLUME_DEFAULT;

		/* pitch offset is in semitones: need to convert to 1/128ths */
		GameSounds[soundIndex].pitch = pitch;

		InitialiseBaseFrequency(static_cast<enum soundindex>(soundIndex));
		soundIndex = fStream.GetByte();
		pitch      = fStream.GetByte();
	}
}
