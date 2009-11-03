
#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include <vorbis/vorbisfile.h>

#include <stdio.h>
#include "vorbisPlayer.h"
#include "logString.h"
#include <process.h>
#include "audioStreaming.h"

#include <vector>
#include <string>
#include <fstream>
#include <assert.h>
#include "utilities.h" // avp_open()

extern "C" 
{
	extern int CreateVorbisAudioBuffer(int channels, int rate, unsigned int *bufferSize);
	extern int UpdateVorbisAudioBuffer(char *audioData, int dataSize, int offset);
	extern void ProcessStreamingAudio();
	extern int SetStreamingMusicVolume(int volume);
	extern int StopVorbisBuffer();
	extern int CDPlayerVolume; // volume control from menus
	extern bool PlayVorbisBuffer();
}

FILE* file;
vorbis_info *pInfo;
OggVorbis_File oggFile;
StreamingAudioBuffer vorbisStream;

bool oggIsPlaying = false;
unsigned int bufferSize = 0;
unsigned int halfBufferSize = 0;
HANDLE hPlaybackThreadFinished;

std::vector<std::string> TrackList;

#ifdef WIN32
	const std::string tracklistFilename = "Music/ogg_tracks.txt";
	const std::string musicFolderName = "Music/";
#endif
#ifdef _XBOX
	const std::string tracklistFilename = "d:\\Music\\ogg_tracks.txt";
	const std::string musicFolderName = "d:\\Music\\";
#endif

static char *audioData = 0;

int ReadVorbisData(char *audioBuffer, int sizeToRead, int offset)
{
	int bytesReadTotal = 0;
	int bytesReadPerLoop = 0;

	while (bytesReadTotal < sizeToRead) 
	{
		bytesReadPerLoop = ov_read(
			&oggFile,									//what file to read from
			(audioBuffer + offset) + bytesReadTotal,	//where to put the decoded data
			sizeToRead - bytesReadTotal,				//how much data to read
			0,											//0 specifies little endian decoding mode
			2,											//2 specifies 16-bit samples
			1,											//1 specifies signed data
			0
		);

		if (bytesReadPerLoop < 0) 
		{
			LogErrorString("ov_read encountered an error", __LINE__, __FILE__);
		}
		/* if we reach the end of the file, go back to start */
		else if (bytesReadPerLoop == 0)
		{
			ov_raw_seek(&oggFile, 0);
		}
		else if (bytesReadPerLoop == OV_HOLE) 
		{
			LogErrorString("OV_HOLE", __LINE__, __FILE__);
		}
		else if (bytesReadPerLoop == OV_EBADLINK) 
		{
			LogErrorString("OV_EBADLINK", __LINE__, __FILE__);
		}

		bytesReadTotal += bytesReadPerLoop;
	}

	return bytesReadTotal;
}

void LoadVorbisTrack(int track) 
{
	/* if we're already playing a track, stop it */
	if (oggIsPlaying) StopVorbis();

	/* TODO? rather than return, pick a random track or just play last? */
	if (track > TrackList.size()) return;

	/* if user enters 1, decrement to 0 to align to array (enters 2, decrement to 1 etc) */
	if (track != 0) track--;

	file = fopen(TrackList[track].c_str(),"rb");
	if (!file) 
	{
		LogErrorString("Can't find OGG Vorbis file " + TrackList[track]);
		return;
	}

	if (ov_open_callbacks(file, &oggFile, NULL, 0, OV_CALLBACKS_DEFAULT) < 0) 
	{
		LogErrorString("File " + TrackList[track] + "is not a valid OGG Vorbis file");
		fclose(file);
		return;
	}

	// get some audio info
	pInfo = ov_info(&oggFile, -1);

	LogString("Opening OGG Vorbis file " + TrackList[track]);

	/* Check the number of channels... always use 16-bit samples */
	if (pInfo->channels == 1) 
		LogString("\t Mono Vorbis file");
	else 
		LogString("\t Stereo Vorbis file");

	LogString("\t Vorbis frequency: " + IntToString(pInfo->rate));

	/* create the audio buffer (directsound or whatever) */
	if (AudioStream_CreateBuffer(&vorbisStream, pInfo->channels, pInfo->rate, 32768, 3) < 0)
	{
		LogErrorString("Can't create audio stream buffer for OGG Vorbis!");
	}

	/* init some temp audio data storage */
	audioData = new char[vorbisStream.bufferSize];

	int totalRead = ReadVorbisData(audioData, vorbisStream.bufferSize, 0);

	/* fill entire buffer initially */
	AudioStream_WriteData(&vorbisStream, audioData, vorbisStream.bufferSize);

	/* start playing */
	PlayVorbis();
}

void UpdateVorbisBuffer(void *arg) 
{
	int lockOffset = 0;

	DWORD dwQuantum = 1000 / 60;

	while (oggIsPlaying)
	{
		Sleep( dwQuantum );

		int numBuffersFree = AudioStream_GetNumFreeBuffers(&vorbisStream);

		if (numBuffersFree)
		{
			ReadVorbisData(audioData, vorbisStream.bufferSize, 0);
			AudioStream_WriteData(&vorbisStream, audioData, vorbisStream.bufferSize);
		}
	}
	SetEvent(hPlaybackThreadFinished);
}

void PlayVorbis() 
{
	if (AudioStream_PlayBuffer(&vorbisStream))
	{
		oggIsPlaying = true;
		AudioStream_SetBufferVolume(&vorbisStream, CDPlayerVolume);
		hPlaybackThreadFinished = CreateEvent( NULL, FALSE, FALSE, NULL );
		 _beginthread(UpdateVorbisBuffer, 0, 0);
	}
	else 
	{
		LogErrorString("couldn't play ogg vorbis buffer\n");
	}
}

void StopVorbis() 
{
	if (oggIsPlaying)
	{
		AudioStream_StopBuffer(&vorbisStream);
		oggIsPlaying = false;
	}

	/* wait until audio processing thread has finished running before continuing */ 
	WaitForMultipleObjects(1, &hPlaybackThreadFinished, TRUE, INFINITE);

	CloseHandle(hPlaybackThreadFinished);

	AudioStream_ReleaseBuffer(&vorbisStream);

	ov_clear(&oggFile);

	delete[] audioData;
	audioData = 0;

//	bytesReadTotal = 0;
//	bytesReadPerLoop = 0;
}

bool LoadVorbisTrackList()
{
	//clear out the old list first
//	EmptyCDTrackList();

	std::ifstream file(tracklistFilename.c_str());

	if (!file.is_open()) 
	{
		LogErrorString("no music tracklist found - not using ogg vorbis music");
		return false;
	}

	std::string trackName;
	unsigned int pos = 0;

	while (std::getline(file, trackName)) 
	{
/*
		pos = trackName.find(": ");
		if (pos != 0)
		{
			trackName = trackName.substr(pos + 2);
			TrackList.push_back(musicFolderName + trackName);
		}
*/
		TrackList.push_back(musicFolderName + trackName);
	}

	file.close();
	return true;
}

int CheckNumberOfVorbisTracks()
{
	return (int)TrackList.size();
}

bool IsVorbisPlaying()
{
	return oggIsPlaying;
}

int SetStreamingMusicVolume(int volume)
{
	return AudioStream_SetBufferVolume(&vorbisStream, volume);
}