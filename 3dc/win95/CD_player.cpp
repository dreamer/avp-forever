#ifdef WIN32

#include "3dc.h"
#include "inline.h"
#include "psndplat.h"
#include "cd_player.h"
#define UseLocalAssert TRUE
#include "ourasert.h"

/* KJL 12:40:35 07/05/98 - This is code derived from Patrick's original stuff & moved into it's own file. */

/* Patrick 10/6/97 -------------------------------------------------------------
  CDDA Support
  ----------------------------------------------------------------------------*/
const int kNoDevice = -1;
static MCIDEVICEID cdDeviceID = kNoDevice;
int cdAuxDeviceID = kNoDevice;

/* Patrick 9/6/97 -------------------------------------------------------------
   ----------------------------------------------------------------------------
  CDDA Support
  -----------------------------------------------------------------------------
  -----------------------------------------------------------------------------*/
static bool CDDASwitchedOn = false;
static bool CDDAIsInitialised = false;
static int CDDAVolume = CDDA_VOLUME_DEFAULT;
CDOPERATIONSTATES CDDAState;

static DWORD PreGameCDVolume; //windows cd volume before the game started

static CDTRACKID TrackBeingPlayed;
static enum CDCOMMANDID LastCommandGiven;

extern HWND hWndMain;

int CDPlayerVolume; // volume control from menus

int CDTrackMax = -1; //highest track number on cd

extern int SetStreamingMusicVolume(int volume);
static void PlatGetCDDAVolumeControl(void);

void CDDA_Start(void)
{
	return; // bjd - revert

	/* function should complete successfully even if no disc in drive */
	CDDAVolume = CDDA_VOLUME_DEFAULT;
	CDPlayerVolume = CDDAVolume;
	CDDAState = CDOp_Idle;
	CDDAIsInitialised = false;

	if (PlatStartCDDA() != SOUND_PLATFORMERROR)
	{
		CDDAIsInitialised = true;
		CDDA_SwitchOn();
		CDDA_ChangeVolume(CDDAVolume); /* init the volume */
		CDDA_CheckNumberOfTracks();
	}
	LastCommandGiven = CDCOMMANDID_Start;
}

void CDDA_End(void)
{
	if (!CDDAIsInitialised) {
		return;
	}

	CDDA_Stop();
	PlatChangeCDDAVolume(CDDA_VOLUME_RESTOREPREGAMEVALUE);
	PlatEndCDDA();
	CDDA_SwitchOff();
	CDDAIsInitialised = false;

	LastCommandGiven = CDCOMMANDID_End;
}

void CDDA_Management(void)
{
	if (!CDDASwitchedOn) {
		return; // CDDA is off
	}
	if (CDDAState == CDOp_Playing) {
		return; // already playing
	}
}

void CDDA_Play(int CDDATrack)
{
	if (!CDDASwitchedOn)
		return; /* CDDA is off */

	if (CDDAState == CDOp_Playing)
		return; /* already playing */

	if ((CDDATrack <= 0)||(CDDATrack >= CDTrackMax))
		return; /* no such track */

	int ok = PlatPlayCDDA((int)CDDATrack);
	if (ok != SOUND_PLATFORMERROR)
	{
		CDDAState = CDOp_Playing;
		LastCommandGiven = CDCOMMANDID_Play;
		TrackBeingPlayed = static_cast<enum cdtrackid>(CDDATrack);
	}
}

void CDDA_PlayLoop(int CDDATrack)
{
	if (!CDDASwitchedOn) {
		return; // CDDA is off
	}
	if (CDDAState == CDOp_Playing) {
		return; // already playing
	}
	if ((CDDATrack <= 0) || (CDDATrack >= CDTrackMax)) {
		return; // no such track
	}

	int ok = PlatPlayCDDA((int)CDDATrack);
	if (ok != SOUND_PLATFORMERROR)
	{
		CDDAState=CDOp_Playing;
		LastCommandGiven = CDCOMMANDID_PlayLoop;
		TrackBeingPlayed = static_cast<enum cdtrackid>(CDDATrack);
	}
}

extern void CheckCDVolume(void)
{
	if (CDDAVolume != CDPlayerVolume) {
		CDDA_ChangeVolume(CDPlayerVolume);
	}
}

void CDDA_ChangeVolume(int volume)
{
	// set vorbis volume here for now
	if (SetStreamingMusicVolume(volume) == 0) // ok
	{
		CDDAVolume=volume;
		CDPlayerVolume = volume;
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

int CDDA_GetCurrentVolumeSetting(void)
{
	return CDDAVolume;
}

void CDDA_Stop()
{
	if (!CDDASwitchedOn) {
		return; // CDDA is off
	}
	if (CDDAState != CDOp_Playing) {
		return; // nothing playing
	}
	int ok = PlatStopCDDA();
	CDDAState = CDOp_Idle;
	LastCommandGiven = CDCOMMANDID_Stop;
}

void CDDA_SwitchOn()
{
	LOCALASSERT(!CDDA_IsPlaying());
	if (CDDAIsInitialised) {
		CDDASwitchedOn = true;
	}
}

void CDDA_SwitchOff()
{
	if (!CDDASwitchedOn) {
		return; // CDDA is off already
	}
	if (CDDA_IsPlaying()) {
		CDDA_Stop();
	}
	CDDASwitchedOn = false;
}

bool CDDA_IsOn()
{
	return CDDASwitchedOn;
}

bool CDDA_IsPlaying()
{
	if (CDDAState == CDOp_Playing)
	{
		LOCALASSERT(CDDASwitchedOn);
		return true;
	}
	return false;
}

int CDDA_CheckNumberOfTracks()
{
	int numTracks = 0;

	if (CDDA_IsOn())
	{
		PlatGetNumberOfCDTracks(&numTracks);

		// if there is only one track , then it probably can't be used anyway
//		if(numTracks==1) numTracks=0;
		if (numTracks <= 1) {
			numTracks = 0; // bjd
		}

		// store the maximum allowed track number
		CDTrackMax=numTracks;
	}
	return numTracks;
}



// win32 specific

int PlatStartCDDA(void)
{
	DWORD dwReturn;
	MCI_OPEN_PARMS mciOpenParms;
	MCIDEVICEID devId = 0;

	// Initialise device handles
	cdDeviceID = kNoDevice;
	cdAuxDeviceID = kNoDevice;

	// try to open mci cd-audio device
	mciOpenParms.lpstrDeviceType = (LPCSTR) MCI_DEVTYPE_CD_AUDIO;
	dwReturn = mciSendCommand(devId, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID, (DWORD_PTR)&mciOpenParms);
	if (dwReturn)
	{
		cdDeviceID = kNoDevice;
		return SOUND_PLATFORMERROR;
	}
	cdDeviceID = mciOpenParms.wDeviceID;

	// now try to get the cd volume control, by obtaining the auxiliary device id for the cd-audio player
	PlatGetCDDAVolumeControl();
	return 0;
}

static void PlatGetCDDAVolumeControl(void)
{
	uint32_t numDev = mixerGetNumDevs();

	// go through the mixer devices searching for one that can deal with the cd volume
	for (uint32_t i = 0; i < numDev; i++)
	{
		HMIXER handle;
		if (mixerOpen(&handle, i, 0, 0, MIXER_OBJECTF_MIXER ) == MMSYSERR_NOERROR )
		{

			// try to get the compact disc mixer line
			MIXERLINE line;
			line.cbStruct = sizeof(MIXERLINE);
			line.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;

			// bjd - everyone seems to just casts to HMIXEROBJ
			if (mixerGetLineInfo((HMIXEROBJ)handle, &line, MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR)
			{
				MIXERLINECONTROLS lineControls;
				MIXERCONTROL control;

				lineControls.cbStruct = sizeof(MIXERLINECONTROLS);
				lineControls.dwLineID = line.dwLineID;
				lineControls.pamxctrl = &control;
				lineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
				lineControls.cControls = 1;
				lineControls.cbmxctrl = sizeof(MIXERCONTROL);

				control.cbStruct = sizeof(MIXERCONTROL);

				// try to get the volume control
				if (mixerGetLineControls((HMIXEROBJ)handle, &lineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
				{
					MIXERCONTROLDETAILS details;
					MIXERCONTROLDETAILS_UNSIGNED detailValue;

					details.cbStruct = sizeof(MIXERCONTROLDETAILS);
					details.dwControlID = control.dwControlID;
					details.cChannels = 1;
					details.cMultipleItems = 0;
					details.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
					details.paDetails = &detailValue;

					// get the current volume so that we can restore it later
					if (mixerGetControlDetails((HMIXEROBJ)handle, &details, MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
					{
						PreGameCDVolume = detailValue.dwValue;
						mixerClose(handle);

						return; //success
					}
				}
			}
			mixerClose(handle);
		}
	}
}

void PlatEndCDDA(void)
{
	return;

	// check the cdDeviceId
	if (cdDeviceID == kNoDevice)
		return;

	DWORD dwReturn = mciSendCommand(cdDeviceID, MCI_CLOSE, MCI_WAIT, 0);
	cdDeviceID = kNoDevice;

	// reset the auxilary device handle
	cdAuxDeviceID = kNoDevice;
}

int PlatPlayCDDA(int track)
{
	DWORD dwReturn;
	MCI_SET_PARMS mciSetParms = {0,0,0};
	MCI_PLAY_PARMS mciPlayParms = {0,0,0};
	MCI_STATUS_PARMS mciStatusParms = {0,0,0,0};

	// check the cdDeviceId
	if (cdDeviceID==kNoDevice)
		return SOUND_PLATFORMERROR;

	// set the time format
	mciSetParms.dwTimeFormat = MCI_FORMAT_MSF;
	dwReturn = mciSendCommand(cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR) &mciSetParms);
	if (dwReturn)
	{
//    	NewOnScreenMessage("CD ERROR - TIME FORMAT");
		return SOUND_PLATFORMERROR;
	}

	// find the length of the track
	mciStatusParms.dwItem = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
	dwReturn = mciSendCommand(cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM|MCI_TRACK, (DWORD_PTR) &mciStatusParms);
	if (dwReturn)
	{
//    	NewOnScreenMessage("CD ERROR - GET LENGTH");
		return SOUND_PLATFORMERROR;
	}

	// set the time format
	mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
	dwReturn = mciSendCommand(cdDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD_PTR) &mciSetParms);
	if (dwReturn)
	{
//    	NewOnScreenMessage("CD ERROR - TIME FORMAT");
		return SOUND_PLATFORMERROR;
	}

	// play the track: set up for notification when track finishes, or an error occurs
	mciPlayParms.dwFrom = MCI_MAKE_TMSF(track,0,0,0);
	mciPlayParms.dwTo = MCI_MAKE_TMSF(track, MCI_MSF_MINUTE(mciStatusParms.dwReturn), MCI_MSF_SECOND(mciStatusParms.dwReturn), MCI_MSF_FRAME(mciStatusParms.dwReturn));
	mciPlayParms.dwCallback = (DWORD_PTR)hWndMain;
	dwReturn = mciSendCommand(cdDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD_PTR) &mciPlayParms);
	if (dwReturn)
	{
//    	NewOnScreenMessage("CD ERROR - PLAY");
		return SOUND_PLATFORMERROR;
	}
	return 0;
}

int PlatGetNumberOfCDTracks(int* numTracks)
{
	DWORD dwReturn;
	MCI_STATUS_PARMS mciStatusParms = {0,0,0,0};

	// check the cdDeviceId
	if (cdDeviceID == kNoDevice)
		return SOUND_PLATFORMERROR;

	if (!numTracks)
		return SOUND_PLATFORMERROR;

	// find the number tracks
	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS ;
	dwReturn = mciSendCommand(cdDeviceID, MCI_STATUS, MCI_STATUS_ITEM , (DWORD_PTR)&mciStatusParms);
	if (dwReturn)
	{
		return SOUND_PLATFORMERROR;
	}

	// number of tracks is in the dwReturn member
	*numTracks = (int)mciStatusParms.dwReturn;

	return 0;
}

int PlatStopCDDA(void)
{
	// check the cdDeviceId
	if (cdDeviceID == kNoDevice)
	{
		return SOUND_PLATFORMERROR;
	}

	// stop the cd player
	DWORD dwReturn = mciSendCommand(cdDeviceID, MCI_STOP, MCI_WAIT, 0);
	if (dwReturn)
	{
		return SOUND_PLATFORMERROR;
	}

	return 0;
}

int PlatChangeCDDAVolume(int volume)
{
	MMRESULT mmres;
	unsigned int newVolume;

	uint32_t numDev = mixerGetNumDevs();

	// check the cdDeviceId
	if (cdDeviceID == kNoDevice)
		return SOUND_PLATFORMERROR;

	// go through the mixer devices searching for one that can deal with the cd volume
	for (uint32_t i = 0; i < numDev; i++)
	{
		HMIXER handle;
		if (mixerOpen(&handle, i, 0, 0, MIXER_OBJECTF_MIXER) == MMSYSERR_NOERROR)
		{
			// try to get the compact disc mixer line
			MIXERLINE line;
			line.cbStruct = sizeof(MIXERLINE);
			line.dwComponentType = MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC;

			if (mixerGetLineInfo((HMIXEROBJ)handle,&line,MIXER_GETLINEINFOF_COMPONENTTYPE) == MMSYSERR_NOERROR)
			{
				MIXERLINECONTROLS lineControls;
				MIXERCONTROL control;

				lineControls.cbStruct = sizeof(MIXERLINECONTROLS);
				lineControls.dwLineID = line.dwLineID;
				lineControls.pamxctrl = &control;
				lineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
				lineControls.cControls = 1;
				lineControls.cbmxctrl = sizeof(MIXERCONTROL);

				control.cbStruct = sizeof(MIXERCONTROL);

				//try to get the volume control
				if (mixerGetLineControls((HMIXEROBJ)handle, &lineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE) == MMSYSERR_NOERROR)
				{

					MIXERCONTROLDETAILS details;
					MIXERCONTROLDETAILS_UNSIGNED detailValue;

					details.cbStruct = sizeof(MIXERCONTROLDETAILS);
					details.dwControlID = control.dwControlID;
					details.cChannels = 1;
					details.cMultipleItems = 0;
					details.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
					details.paDetails = &detailValue;

					if (volume == CDDA_VOLUME_RESTOREPREGAMEVALUE)
					{
						// set the volume to what it was before the game started
						newVolume = PreGameCDVolume;
					}
					else
					{
						// scale the volume
						newVolume = control.Bounds.dwMinimum +  WideMulNarrowDiv(volume, (control.Bounds.dwMaximum-control.Bounds.dwMinimum), (CDDA_VOLUME_MAX-CDDA_VOLUME_MIN));

						if (newVolume<control.Bounds.dwMinimum) newVolume = control.Bounds.dwMinimum;
						if (newVolume>control.Bounds.dwMaximum) newVolume = control.Bounds.dwMaximum;
					}
					// fill in the volume in the control details structure
					detailValue.dwValue=newVolume;

					mmres = mixerSetControlDetails((HMIXEROBJ)handle, &details, MIXER_SETCONTROLDETAILSF_VALUE);
					mixerClose(handle);

					if (mmres == MMSYSERR_NOERROR) {
						return 1;
					}
					else {
						return SOUND_PLATFORMERROR;
					}
				}
			}
			mixerClose(handle);
		}
	}

	return SOUND_PLATFORMERROR;
}

void PlatCDDAManagementCallBack(WPARAM flags, LONG deviceId)
{
	// check the cdDeviceId
	if (cdDeviceID == kNoDevice) {
		return;
	}
	// compare with the passed device id
	if ((UINT)deviceId != cdDeviceID) {
		return;
	}

	if (flags & MCI_NOTIFY_SUCCESSFUL)
	{
		CDDAState = CDOp_Idle;
		//NewOnScreenMessage("CD COMMAND RETURNED WITH SUCCESSFUL");
		/* Play it again, sam */
		if (LastCommandGiven == CDCOMMANDID_PlayLoop)
		{
			CDDA_PlayLoop(TrackBeingPlayed);
		}
	}
	else if (flags & MCI_NOTIFY_FAILURE)
	{
		/* error while playing: abnormal termination */
		//NewOnScreenMessage("CD COMMAND FAILED");
		CDDAState = CDOp_Idle;
	}
	else if (flags & MCI_NOTIFY_SUPERSEDED)
	{
		//NewOnScreenMessage("CD COMMAND SUPERSEDED");
	}
	else if (flags & MCI_NOTIFY_ABORTED)
	{
		/* aborted or superceeded: try and stop the device */
		//NewOnScreenMessage("CD COMMAND ABORTED(?)");
	  //	CDDA_Stop();
	}
	else
	{
		//NewOnScreenMessage("CD COMMAND RETURNED WITH UNKNOWN MESSAGE");
	}
}
#endif // ifdef WIN32
