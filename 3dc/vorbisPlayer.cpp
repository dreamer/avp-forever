
#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

//#include <vorbis/vorbisfile.h>

#include <stdio.h>
#include "vorbisPlayer.h"
#include "logString.h"
#include <process.h>

#include <vector>
#include <fstream>
#include <assert.h>
#include "utilities.h" // avp_open()
#include "console.h"

int ReadVorbisData(byte *audioBuffer, int sizeToRead, int offset);
void Vorbis_Play(VorbisCodec *VorbisStream);
int Vorbis_ReadData(VorbisCodec *VorbisStream, int sizeToRead);
void Vorbis_UpdateThread(void *arg);

extern "C" 
{
	extern int SetStreamingMusicVolume(int volume);
	extern int CDPlayerVolume; // volume control from menus
}

FILE *file;
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

static byte *audioData = 0;

VorbisCodec * Vorbis_LoadFile(const std::string &fileName)
{
	VorbisCodec *newVorbisStream = new VorbisCodec;

	newVorbisStream->file = fopen(fileName.c_str(),"rb");
	if (!newVorbisStream->file) 
//	if (1) // testing fail early
	{
		Con_PrintError("Can't find OGG Vorbis file " + fileName);
		//delete newVorbisStream;
		Vorbis_Release(newVorbisStream);
		return NULL;
	}

	if (ov_open_callbacks(newVorbisStream->file, &newVorbisStream->oggFile, NULL, 0, OV_CALLBACKS_DEFAULT) < 0) 
	{
		Con_PrintError("File " + fileName + "is not a valid OGG Vorbis file");
		Vorbis_Release(newVorbisStream);
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
	if (AudioStream_CreateBuffer(&newVorbisStream->audioStream, newVorbisStream->pInfo->channels, newVorbisStream->pInfo->rate, 32768, 3) != AUDIOSTREAM_OK)
	{
		Con_PrintError("Can't create audio stream buffer for OGG Vorbis!");
		Vorbis_Release(newVorbisStream);
	}

	/* init some temp audio data storage */
	newVorbisStream->audioData = new byte[newVorbisStream->audioStream.bufferSize];
	if (newVorbisStream->audioData == NULL)
	{
		// report major system error here?
		Vorbis_Release(newVorbisStream);
	}

	Vorbis_ReadData(newVorbisStream, newVorbisStream->audioStream.bufferSize);

	/* fill the first buffer */
	AudioStream_WriteData(&newVorbisStream->audioStream, newVorbisStream->audioData, newVorbisStream->audioStream.bufferSize);

	// start playing
	Vorbis_Play(newVorbisStream);

	OutputDebugString("should be playing menu music now.. \n");

	return newVorbisStream;
}

void Vorbis_Play(VorbisCodec *VorbisStream)
{
	if (AudioStream_PlayBuffer(&VorbisStream->audioStream) == AUDIOSTREAM_OK)
	{
		VorbisStream->oggIsPlaying = true;
		AudioStream_SetBufferVolume(&VorbisStream->audioStream, CDPlayerVolume);
		VorbisStream->hPlaybackThreadFinished = CreateEvent( NULL, FALSE, FALSE, NULL );
		 _beginthread(Vorbis_UpdateThread, 0, static_cast<void*>(VorbisStream));
	}
	else 
	{
		LogErrorString("couldn't play ogg vorbis buffer\n");
	}
}

void Vorbis_UpdateThread(void *arg) 
{
	VorbisCodec *VorbisStream = static_cast<VorbisCodec*>(arg);

#ifdef USE_XAUDIO2
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
#endif

	DWORD dwQuantum = 1000 / 60;

	while (VorbisStream->oggIsPlaying)
	{
		OutputDebugString("thread running..\n");
		int numBuffersFree = AudioStream_GetNumFreeBuffers(&VorbisStream->audioStream);

		if (numBuffersFree)
		{
			Vorbis_ReadData(VorbisStream, VorbisStream->audioStream.bufferSize);
			AudioStream_WriteData(&VorbisStream->audioStream, VorbisStream->audioData, VorbisStream->audioStream.bufferSize);
		}

		Sleep( dwQuantum );
	}
	SetEvent(VorbisStream->hPlaybackThreadFinished);
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
	if (VorbisStream->oggIsPlaying)
	{
		AudioStream_StopBuffer(&VorbisStream->audioStream);
		VorbisStream->oggIsPlaying = false;
	}

	/* wait until audio processing thread has finished running before continuing */ 
	WaitForMultipleObjects(1, &VorbisStream->hPlaybackThreadFinished, TRUE, INFINITE);

	CloseHandle(VorbisStream->hPlaybackThreadFinished);
}

void Vorbis_Release(VorbisCodec *VorbisStream)
{
	assert (VorbisStream);

	Vorbis_Stop(VorbisStream);
	
//	if (VorbisStream->audioStream)
	{
		AudioStream_ReleaseBuffer(&VorbisStream->audioStream);
	}

	ov_clear(&VorbisStream->oggFile);

	if (VorbisStream->audioData)
	{
		delete[] VorbisStream->audioData;
		VorbisStream->audioData = 0;
	}

	delete[] VorbisStream;
	VorbisStream = NULL;
}


















int ReadVorbisData(byte *audioBuffer, int sizeToRead, int offset)
{
	int bytesReadTotal = 0;
	int bytesReadPerLoop = 0;

	while (bytesReadTotal < sizeToRead) 
	{
		bytesReadPerLoop = ov_read(
			&oggFile,									//what file to read from
			reinterpret_cast<char*>(audioBuffer + bytesReadTotal),
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
	if (oggIsPlaying) 
		StopVorbis();

	/* TODO? rather than return, pick a random track or just play last? */
	if (track > TrackList.size()) 
		return;

	/* if user enters 1, decrement to 0 to align to array (enters 2, decrement to 1 etc) */
	if (track != 0) 
		track--;

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

	ogg_int64_t numSamples = ov_pcm_total(&oggFile, -1);

	/* create the audio buffer (directsound or whatever) */
	if (AudioStream_CreateBuffer(&vorbisStream, pInfo->channels, pInfo->rate, 32768, 3) != AUDIOSTREAM_OK)
	{
		LogErrorString("Can't create audio stream buffer for OGG Vorbis!");
	}

	/* init some temp audio data storage */
	audioData = new byte[vorbisStream.bufferSize];

	int totalRead = ReadVorbisData(audioData, vorbisStream.bufferSize, 0);

	/* fill the first buffer */
	AudioStream_WriteData(&vorbisStream, audioData, vorbisStream.bufferSize);

	/* start playing */
	PlayVorbis();
}

void UpdateVorbisBuffer(void *arg) 
{

#ifdef USE_XAUDIO2
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
#endif

	DWORD dwQuantum = 1000 / 60;

	while (oggIsPlaying)
	{
		int numBuffersFree = AudioStream_GetNumFreeBuffers(&vorbisStream);

		if (numBuffersFree)
		{
			ReadVorbisData(audioData, vorbisStream.bufferSize, 0);
			AudioStream_WriteData(&vorbisStream, audioData, vorbisStream.bufferSize);
		}

		Sleep( dwQuantum );
	}
	SetEvent(hPlaybackThreadFinished);
}

void PlayVorbis() 
{
	if (AudioStream_PlayBuffer(&vorbisStream) == AUDIOSTREAM_OK)
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