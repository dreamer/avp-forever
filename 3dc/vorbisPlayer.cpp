#include <stdio.h>
#include "vorbisPlayer.h"
#include "logString.h"
#include <process.h>

#include <vector>
#include <fstream>
#include <assert.h>
#include "utilities.h" // avp_open()
#include "console.h"

void Vorbis_Play(VorbisCodec *VorbisStream);
int Vorbis_ReadData(VorbisCodec *VorbisStream, int sizeToRead);
unsigned int __stdcall Vorbis_UpdateThread(void *args);

extern "C" 
{
	extern int SetStreamingMusicVolume(int volume);
	extern int CDPlayerVolume; // volume control from menus
}

std::vector<std::string> TrackList;

#ifdef WIN32
	const std::string tracklistFilename = "Music/ogg_tracks.txt";
	const std::string musicFolderName = "Music/";
#endif
#ifdef _XBOX
	const std::string tracklistFilename = "d:\\Music\\ogg_tracks.txt";
	const std::string musicFolderName = "d:\\Music\\";
#endif

static VorbisCodec *inGameMusic = NULL;

VorbisCodec * Vorbis_LoadFile(const std::string &fileName)
{
	VorbisCodec *newVorbisStream = new VorbisCodec;

	memset(newVorbisStream, 0, sizeof(VorbisCodec));

	newVorbisStream->file = fopen(fileName.c_str(),"rb");
	if (!newVorbisStream->file) 
	{
		Con_PrintError("Can't find OGG Vorbis file " + fileName);
		Vorbis_Release(newVorbisStream);
		newVorbisStream = NULL;
		return NULL;
	}

	if (ov_open_callbacks(newVorbisStream->file, &newVorbisStream->oggFile, NULL, 0, OV_CALLBACKS_DEFAULT) < 0) 
	{
		Con_PrintError("File " + fileName + "is not a valid OGG Vorbis file");
		Vorbis_Release(newVorbisStream);
		newVorbisStream = NULL;
		return NULL;
	}

	// get some audio info
	newVorbisStream->pInfo = ov_info(&newVorbisStream->oggFile, -1);

	Con_PrintMessage("Opening OGG Vorbis file " + fileName);

/*
	// Check the number of channels... always use 16-bit samples
	if (pInfo->channels == 1) 
		LogString("\t Mono Vorbis file");
	else 
		LogString("\t Stereo Vorbis file");

	LogString("\t Vorbis frequency: " + IntToString(pInfo->rate));

	ogg_int64_t numSamples = ov_pcm_total(&oggFile, -1);
*/

	// create the streaming audio buffer
	newVorbisStream->audioStream = AudioStream_CreateBuffer(newVorbisStream->pInfo->channels, newVorbisStream->pInfo->rate, 32768, 3);
	if (newVorbisStream->audioStream == NULL)
	{
		Con_PrintError("Can't create audio stream buffer for OGG Vorbis!");
		Vorbis_Release(newVorbisStream);
		newVorbisStream = NULL;
	}

	// init some temp audio data storage
	newVorbisStream->audioData = new uint8_t[newVorbisStream->audioStream->bufferSize];
	if (newVorbisStream->audioData == NULL)
	{
		// report major system error here?
		Vorbis_Release(newVorbisStream);
		newVorbisStream = NULL;
	}

	Vorbis_ReadData(newVorbisStream, newVorbisStream->audioStream->bufferSize);

	// fill the first buffer
	AudioStream_WriteData(newVorbisStream->audioStream, newVorbisStream->audioData, newVorbisStream->audioStream->bufferSize);

	// start playing
	Vorbis_Play(newVorbisStream);

	return newVorbisStream;
}

void Vorbis_Play(VorbisCodec *VorbisStream)
{
	if (AudioStream_PlayBuffer(VorbisStream->audioStream) == AUDIOSTREAM_OK)
	{
		VorbisStream->oggIsPlaying = true;
		AudioStream_PlayBuffer(VorbisStream->audioStream);
		AudioStream_SetBufferVolume(VorbisStream->audioStream, CDPlayerVolume);
		VorbisStream->hPlaybackThreadFinished = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, Vorbis_UpdateThread, static_cast<void*>(VorbisStream), 0, NULL));
	}
	else 
	{
		LogErrorString("couldn't play ogg vorbis buffer\n");
	}
}

unsigned int __stdcall Vorbis_UpdateThread(void *args)
{
	VorbisCodec *VorbisStream = static_cast<VorbisCodec*>(args);

#ifdef USE_XAUDIO2
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
#endif

	DWORD dwQuantum = 1000 / 60;

	while (VorbisStream->oggIsPlaying)
	{
		int numBuffersFree = AudioStream_GetNumFreeBuffers(VorbisStream->audioStream);

		if (numBuffersFree)
		{
			Vorbis_ReadData(VorbisStream, VorbisStream->audioStream->bufferSize);
			AudioStream_WriteData(VorbisStream->audioStream, VorbisStream->audioData, VorbisStream->audioStream->bufferSize);
		}

		Sleep( dwQuantum );
	}

	_endthreadex(0);
	return 0;
}

int Vorbis_ReadData(VorbisCodec *VorbisStream, int sizeToRead)
{
	int bytesReadTotal = 0;
	int bytesReadPerLoop = 0;

	while (bytesReadTotal < sizeToRead) 
	{
		bytesReadPerLoop = ov_read(
			&VorbisStream->oggFile,									//what file to read from
			reinterpret_cast<char*>(VorbisStream->audioData + bytesReadTotal),
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
		// if we reach the end of the file, go back to start
		else if (bytesReadPerLoop == 0)
		{
			ov_raw_seek(&VorbisStream->oggFile, 0);
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

void Vorbis_Stop(VorbisCodec *VorbisStream)
{
	if (!VorbisStream)
		return;

	if (VorbisStream->oggIsPlaying)
	{
		AudioStream_StopBuffer(VorbisStream->audioStream);
		VorbisStream->oggIsPlaying = false;
	}

	// wait until audio processing thread has finished running before continuing
	if (VorbisStream->hPlaybackThreadFinished)
	{
		WaitForSingleObject(VorbisStream->hPlaybackThreadFinished, INFINITE);
		CloseHandle(VorbisStream->hPlaybackThreadFinished);
		VorbisStream->hPlaybackThreadFinished = NULL;
	}
}

void Vorbis_Release(VorbisCodec *VorbisStream)
{
	if (!VorbisStream)
		return;

	Vorbis_Stop(VorbisStream);
	
	if (VorbisStream->audioStream)
	{
		AudioStream_ReleaseBuffer(VorbisStream->audioStream);
		VorbisStream->audioStream = NULL;
	}

	ov_clear(&VorbisStream->oggFile);

	if (VorbisStream->audioData)
	{
		delete[] VorbisStream->audioData;
		VorbisStream->audioData = 0;
	}

	delete VorbisStream;
}

void LoadVorbisTrack(int track) 
{
	/* if we're already playing a track, stop it */ 
	Vorbis_Stop(inGameMusic);
	Vorbis_Release(inGameMusic);
	inGameMusic = NULL;

	/* TODO? rather than return, pick a random track or just play last? */
	if (track > TrackList.size()) 
		return;

	/* if user enters 1, decrement to 0 to align to array (enters 2, decrement to 1 etc) */
	if (track != 0) 
		track--;

	inGameMusic = Vorbis_LoadFile(TrackList[track].c_str());
}

bool LoadVorbisTrackList()
{
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
	if (inGameMusic)
		return inGameMusic->oggIsPlaying;
	else
		return false;
}

int SetStreamingMusicVolume(int volume)
{
	if (inGameMusic) // hack to stop this call before the audio stream is initialised
		return AudioStream_SetBufferVolume(inGameMusic->audioStream, volume);
	else
		return 0;
}

void Vorbis_CloseSystem()
{
	Vorbis_Release(inGameMusic);
	inGameMusic = NULL;
}