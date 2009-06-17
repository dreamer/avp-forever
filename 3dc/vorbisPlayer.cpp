
#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include <vorbis/vorbisfile.h>

#include <stdio.h>
#include "vorbisPlayer.h"
#include "logString.h"
#include <process.h>

#include <vector>
#include <string>
#include <fstream>
#include <assert.h>

extern "C" 
{
	extern int CreateVorbisAudioBuffer(int channels, int rate, unsigned int *bufferSize);
	extern int UpdateVorbisAudioBuffer(char *audioData, int dataSize, int offset);
	extern void ProcessStreamingAudio();
	extern int SetVorbisBufferVolume(int volume);
	extern int StopVorbisBuffer();
	extern int CDPlayerVolume; // volume control from menus
	extern bool PlayVorbisBuffer();
	extern HANDLE hHandles[2];
}

FILE* file;
vorbis_info *pInfo;
OggVorbis_File oggFile;

bool oggIsPlaying = false;
unsigned int bufferSize = 0;
unsigned int halfBufferSize = 0;

std::vector<std::string> TrackList;

#ifdef WIN32
	const std::string tracklistFilename = "Music/ogg_tracks.txt";
	const std::string musicFolderName = "Music/";
#endif
#ifdef _XBOX
	const std::string tracklistFilename = "d:\\Music\\ogg_tracks.txt";
	const std::string musicFolderName = "d:\\Music\\";
#endif

char *audioData = 0;

int ReadVorbisData(int sizeToRead, int offset)
{
	assert(offset < bufferSize);

	int bytesReadTotal = 0;
	int bytesReadPerLoop = 0;

	while(bytesReadTotal < sizeToRead) 
	{
		bytesReadPerLoop = ov_read(
			&oggFile,									//what file to read from
			(audioData + offset) + bytesReadTotal,		//where to put the decoded data
			sizeToRead - bytesReadTotal,				//how much data to read
			0,											//0 specifies little endian decoding mode
			2,											//2 specifies 16-bit samples
			1,											//1 specifies signed data
			0
		);

		if(bytesReadPerLoop < 0) 
		{
			LogErrorString("ov_read encountered an error", __LINE__, __FILE__);
		}
		/* if we reach the end of the file, go back to start */
		else if(bytesReadPerLoop == 0)
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
	if (CreateVorbisAudioBuffer(pInfo->channels, pInfo->rate, &bufferSize) < 0)
	{
		LogErrorString("Can't create audio buffer for OGG Vorbis!");
	}

	/* init some temp audio data storage */
	audioData = new char[bufferSize];

	halfBufferSize = bufferSize / 2;

#ifdef WIN32
	int totalRead = ReadVorbisData(bufferSize, 0);
#endif
#ifdef _XBOX
	ProcessStreamingAudio();
#endif

	/* fill entire buffer initially */
#ifdef WIN32
	UpdateVorbisAudioBuffer(audioData, totalRead, 0);
#endif
	/* start playing */
	PlayVorbis();
}

void UpdateVorbisBuffer(void *arg) 
{
	int lockOffset = 0;

	DWORD dwQuantum = 1000 / 60;

	while(oggIsPlaying)
	{
#ifdef _XBOX
		ProcessStreamingAudio();
		Sleep( dwQuantum );
#else
		int waitValue = WaitForMultipleObjects(2, hHandles, FALSE, 1);

		if((waitValue != WAIT_TIMEOUT) && (waitValue != WAIT_FAILED))
		{
			if (waitValue == 0) 
			{
				lockOffset = 0;
			}
			else 
			{
				lockOffset = halfBufferSize;
			}

			int totalRead = ReadVorbisData(halfBufferSize, 0);

			UpdateVorbisAudioBuffer(audioData, totalRead, lockOffset);
		}
#endif
	}
}

void PlayVorbis() 
{
	if (PlayVorbisBuffer())
	{
		oggIsPlaying = true;
		SetVorbisBufferVolume(CDPlayerVolume);
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
		StopVorbisBuffer();
		oggIsPlaying = false;
	}

	ReleaseVorbisBuffer();

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

	if(!file) 
	{
		LogErrorString("no music tracklist found - not using ogg vorbis music\n");
		return false;
	}

	std::string trackName;
	unsigned int pos = 0;

	while (std::getline(file, trackName)) 
	{
		pos = trackName.find(": ");
		if(pos != 0)
		{
			trackName = trackName.substr(pos + 2);
			TrackList.push_back(musicFolderName + trackName);
		}
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