
#include "MusicPlayer.h"
#include "FileStream.h"
#include "logstring.h"
#include "VorbisPlayer.h"
#include "3dc.h"
#include "gamedef.h"
#include "AvP_EnvInfo.h"
#include "list_tem.hpp"
#include "ConfigFile.h"

static void Music_EmptyCDTrackList();
static void ExtractTracksForLevel(char* &buffer, List<int> &track_list);
static bool PickTrack(List<int>& track_list);
void Music_EmptyTrackList();

const int kMultiplayerTracksCount = 3;

// lists of tracks for each level
List<int> LevelCDTracks[AVP_ENVIRONMENT_END_OF_LIST];
List<int> MultiplayerCDTracks[kMultiplayerTracksCount];

static bool musicAvailable = false;
static int LastTrackChosen=-1;
static uint32_t TrackSelectCounter=0;
int musicVolume = 0;

extern int NumberForCurrentLevel();

bool Music_Init()
{
	musicAvailable = Config_GetBool("[Music]", "UseMusic", true);
	if (!musicAvailable) {
		return true; // silently fail as user doesn't want music
	}

	Music_EmptyTrackList();

	FileStream trackFile;
	if (!trackFile.Open("CD Tracks.txt", FileStream::FileRead, FileStream::SkipFastFileCheck)) {
		LogErrorString("Failed to open CD Tracks.txt");
		return false;
	}

	// create a string buffer as big as the file (+1 for null terminator)
	uint32_t fileSize = trackFile.GetFileSize();
	char *buffer = new char[fileSize+1];
	trackFile.ReadBytes((uint8_t*)buffer, fileSize);

	// set null terminator to last character
	buffer[fileSize] = '\0';

	char *startBufferptr = buffer;

	// first extract the multiplayer tracks
	for (int i = 0; i < kMultiplayerTracksCount; i++)
	{
		ExtractTracksForLevel(startBufferptr, MultiplayerCDTracks[i]);
	}

	// now the level tracks
	for (int i = 0; i < AVP_ENVIRONMENT_END_OF_LIST; i++)
	{
		ExtractTracksForLevel(startBufferptr, LevelCDTracks[i]);
	}

	delete[] buffer;

	musicAvailable = true;
	return true;
}

void Music_Deinit()
{

}

bool Music_Start()
{
	return true;
}

void Music_Stop()
{
	if (!musicAvailable) {
		return; // silently fail
	}

	Vorbis_CloseSystem();
}

bool Music_IsPlaying()
{
	return IsVorbisPlaying();
}

void Music_ResetForLevel()
{
	TrackSelectCounter = 0;
	Music_Stop();
}

void Music_CheckVolume()
{
	if (!musicAvailable) {
		return; // silently fail
	}

	if (musicVolume != GetStreamingMusicVolume())
	{
		SetStreamingMusicVolume(musicVolume);
	}
}

// CheckCDAndChooseTrackIfNeeded()
void Music_Check()
{
	static enum playertypes lastPlayerType;

	if (!musicAvailable) {
		return;
	}

	// are we still playing music?
	if (IsVorbisPlaying())
	{
		// if in a multiplayer game see if we have changed character type
		if (AvP.Network == I_No_Network || AvP.PlayerType == lastPlayerType) {
			return;
		}

		// have changed character type, is the current track in the list for this character type
		if (MultiplayerCDTracks[AvP.PlayerType].contains(LastTrackChosen)) {
			return;
		}

		// Lets choose a new track then
	}

	if (AvP.Network == I_No_Network)
	{
		int level = NumberForCurrentLevel();
		if (level >= 0 && level < AVP_ENVIRONMENT_END_OF_LIST)
		{
			if (PickTrack(LevelCDTracks[level])) {
				return;
			}
		}
	}

	// multiplayer (or their weren't ant level specific tracks)
	lastPlayerType = AvP.PlayerType;
	PickTrack(MultiplayerCDTracks[AvP.PlayerType]);
}

static bool PickTrack(List<int>& track_list)
{
	if (!musicAvailable) {
		return true; // silently fail
	}

	// make sure we have some tracks in the list
	if (!CheckNumberOfVorbisTracks()) {
		return false;
	}

	if (!track_list.size()) {
		return false;
	}

	// pick the next track in the list
	uint32_t index = TrackSelectCounter % track_list.size();

	if (index > CheckNumberOfVorbisTracks()) {
		return false;
	}

	TrackSelectCounter++;

	// play it
	LoadVorbisTrack(track_list[index]);

	LastTrackChosen = track_list[index];

	return true;
}

static void ExtractTracksForLevel(char* &buffer, List<int> &track_list)
{
	// search for a line starting with a #
	while (*buffer)
	{
		if (*buffer=='#') {	
			break;
		}

		// search for next line
		while (*buffer)
		{
			if (*buffer=='\n')
			{
				buffer++;
				if (*buffer=='\r') {
					buffer++;
				}
				break;
			}
			buffer++;
		}
	}

	while (*buffer)
	{
		// search for a track number or comment
		if (*buffer==';')
		{
			// comment, so no further info on this line
			break;
		}
		else if (*buffer=='\n' || *buffer=='\r')
		{
			// reached end of line
			break;
		}
		else if (*buffer>='0' && *buffer<='9')
		{
			signed int track = -1;
			// find a number, add it to the list
			sscanf(buffer, "%d", &track);

			if (track >= 0) {
				track_list.add_entry(track);
			}

			// skip to the next non numerical character
			while (*buffer >= '0' && *buffer <= '9') {
				buffer++;
			}
		}
		else {
			buffer++;
		}
	}

	// go to the next line
	while (*buffer)
	{
		if (*buffer=='\n')
		{
			buffer++;
			if (*buffer=='\r') {
				buffer++;
			}
			break;
		}
		buffer++;
	}
}

void Music_EmptyTrackList()
{
	for (int i = 0; i < AVP_ENVIRONMENT_END_OF_LIST; i++)
	{
		while (LevelCDTracks[i].size()) LevelCDTracks[i].delete_first_entry();
	}

	for (int i = 0; i < kMultiplayerTracksCount; i++)
	{
		while (MultiplayerCDTracks[i].size()) MultiplayerCDTracks[i].delete_first_entry();
	}
}

