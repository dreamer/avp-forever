#include "3dc.h"
#include "inline.h"
#include "psndplat.h"
#include "cd_player.h"

#define UseLocalAssert Yes
#include "ourasert.h"

/* KJL 12:40:35 07/05/98 - This is code derived from Patrick's original stuff &
moved into it's own file. */

/* Patrick 10/6/97 -------------------------------------------------------------
  CDDA Support
  ----------------------------------------------------------------------------*/
const int kNoDevice = -1;
int cdDeviceID    = kNoDevice;
int cdAuxDeviceID = kNoDevice;

/* Patrick 9/6/97 -------------------------------------------------------------
   ----------------------------------------------------------------------------
  CDDA Support
  -----------------------------------------------------------------------------
  -----------------------------------------------------------------------------*/
static bool CDDASwitchedOn = false;
static bool CDDAIsInitialised = false;
static int CDDAVolume = CDDA_VOLUME_DEFAULT;

static DWORD PreGameCDVolume;//windows cd volume before the game started

static CDTRACKID TrackBeingPlayed;
static enum CDCOMMANDID LastCommandGiven;

int CDPlayerVolume; // volume control from menus
int CDTrackMax = -1; //highest track number on cd

extern int SetStreamingMusicVolume(int volume);

void CDDA_Start()
{
	CDDAIsInitialised = 1; 
	CDDA_SwitchOn();
	CDDA_ChangeVolume(CDDAVolume); // init the volume
	CDDA_CheckNumberOfTracks();
}

void CDDA_End()
{
	if (!CDDAIsInitialised) {
		return;
	}

	CDDAIsInitialised = false;
}

void CDDA_Management()
{
}

void CDDA_Play(int CDDATrack)
{
}

void CDDA_PlayLoop(int CDDATrack)
{
}

extern void CheckCDVolume()
{
	if (CDDAVolume != CDPlayerVolume) {
		CDDA_ChangeVolume(CDPlayerVolume);
	}
}

void CDDA_ChangeVolume(int volume)
{
	// set vorbis volume here for now
	if (SetStreamingMusicVolume(volume))
	{
		CDDAVolume = volume;
		CDPlayerVolume = volume;
		return;
	}

	if (!CDDASwitchedOn) {
		return; // CDDA is off
	}

	if ((volume < CDDA_VOLUME_MIN) || (volume > CDDA_VOLUME_MAX)) {
		return;
	}

	if (CDDA_IsOn()) 
	{
		if (PlatChangeCDDAVolume(volume))
		{
			CDDAVolume = volume;
			CDPlayerVolume = volume;
			LastCommandGiven = CDCOMMANDID_ChangeVolume;
		}
	}
}

int CDDA_GetCurrentVolumeSetting()
{
	return CDDAVolume;
}

void CDDA_Stop()
{
}

void CDDA_SwitchOn()
{
	if (CDDAIsInitialised) {
		CDDASwitchedOn = true;
	}
}

void CDDA_SwitchOff()
{
}

bool CDDA_IsOn()
{
	return CDDASwitchedOn;
}

bool CDDA_IsPlaying()
{
	return false;
}

int CDDA_CheckNumberOfTracks()
{
	int numTracks = 0;

	return numTracks;
}



// win32 specific

bool PlatStartCDDA()
{
	return false;
}

static void PlatGetCDDAVolumeControl()
{
	return;
}

void PlatEndCDDA()
{
}

int PlatPlayCDDA(int track)
{
    return 0;
}

int PlatGetNumberOfCDTracks(int* numTracks)
{
	return 0;
}

int PlatStopCDDA()
{
    return 0;
}

int PlatChangeCDDAVolume(int volume)
{
	return 1;
}

void PlatCDDAManagement()
{
	/* does nothing for Win95: use call back instead */
}

void PlatCDDAManagementCallBack(WPARAM flags, LONG deviceId)
{

}
