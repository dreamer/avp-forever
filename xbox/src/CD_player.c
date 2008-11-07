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
#define NO_DEVICE -1
int cdDeviceID = NO_DEVICE;
int cdAuxDeviceID = NO_DEVICE;

/* Patrick 9/6/97 -------------------------------------------------------------
   ----------------------------------------------------------------------------
  CDDA Support
  -----------------------------------------------------------------------------
  -----------------------------------------------------------------------------*/
static int CDDASwitchedOn = 0;
static int CDDAIsInitialised = 0;
static int CDDAVolume = CDDA_VOLUME_DEFAULT;
//CDOPERATIONSTATES CDDAState;

static DWORD PreGameCDVolume;//windows cd volume before the game started

static CDTRACKID TrackBeingPlayed;
static enum CDCOMMANDID LastCommandGiven;

//extern HWND hWndMain;

int CDPlayerVolume; // volume control from menus

int CDTrackMax=-1; //highest track number on cd

void CDDA_Start(void)
{

}

void CDDA_End(void)
{

}

void CDDA_Management(void)
{

}

void CDDA_Play(int CDDATrack)
{

}
void CDDA_PlayLoop(int CDDATrack)
{

}

extern void CheckCDVolume(void)
{

}
void CDDA_ChangeVolume(int volume)
{

}

int CDDA_GetCurrentVolumeSetting(void)
{
	return CDDAVolume;
}

void CDDA_Stop()
{

}

void CDDA_SwitchOn()
{

}

void CDDA_SwitchOff()
{

}

int CDDA_IsOn()
{
	return CDDASwitchedOn;
}

int CDDA_IsPlaying()
{

	return 0;
}

int CDDA_CheckNumberOfTracks()
{
	int numTracks=0;

	return numTracks;
}



/* win95 specific */

int PlatStartCDDA(void)
{
	return 0;
}

static void PlatGetCDDAVolumeControl(void)
{
	return;
}


void PlatEndCDDA(void)
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

int PlatStopCDDA(void)
{
    return 0;
}

int PlatChangeCDDAVolume(int volume)
{
	return 1;
}


void PlatCDDAManagement(void)
{
	/* does nothing for Win95: use call back instead */
}

void PlatCDDAManagementCallBack(WPARAM flags, LONG deviceId)
{

}
