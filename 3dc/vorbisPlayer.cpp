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

//unsigned int bytesReadTotal		= 0; //keep track of how many bytes we have read so far
//unsigned int bytesReadPerLoop	= 0; //keep track of how many bytes we read per ov_read invokation (1 to ensure that while loop is entered below)

bool oggIsPlaying = false;
unsigned int bufferSize = 0;
unsigned int halfBufferSize = 0;

std::vector<std::string> TrackList;

#ifdef _DEBUG
	#pragma comment(lib, "libvorbisfile_static_d.lib")
	#pragma comment(lib, "libvorbis_static_d.lib")
	#pragma comment(lib, "libogg_static_d.lib")
#else
	#pragma comment(lib, "libvorbisfile_static.lib")
	#pragma comment(lib, "libvorbis_static.lib")
	#pragma comment(lib, "libogg_static.lib")
#endif
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
			&oggFile,							//what file to read from
			(audioData + offset) + bytesReadTotal,			//where to put the decoded data
			sizeToRead - bytesReadTotal,		//how much data to read
			0,									//0 specifies little endian decoding mode
			2,									//2 specifies 16-bit samples
			1,									//1 specifies signed data
			0
		);

		if(bytesReadPerLoop < 0) 
		{
			LogDxErrorString("ov_read encountered an error\n");
		}
		/* if we reach the end of the file, go back to start */
		else if(bytesReadPerLoop == 0)
		{
			ov_raw_seek(&oggFile, 0);
		}
		else if (bytesReadPerLoop == OV_HOLE) 
		{
			LogDxErrorString("OV_HOLE\n");
		}
		else if (bytesReadPerLoop == OV_EBADLINK) 
		{
			LogDxErrorString("OV_EBADLINK\n");
		}

		bytesReadTotal += bytesReadPerLoop;
	}

	return bytesReadTotal;
}

void LoadVorbisTrack(int track) 
{
	/* check if we have some files to play */
//	if(!loadOggTrackList()) return;

	/* if we're already playing a track, stop it */
	if (oggIsPlaying) StopVorbis();

	/* TODO? rather than return, pick a random track or just play last? */
	if (track > TrackList.size()) return;

	/* if user enters 1, decrement to 0 to align to array (enters 2, decrement to 1 etc) */
	if (track != 0) track--;

	file = fopen(TrackList[track].c_str(),"rb");
	if (!file) 
	{
		LogDxErrorString("ogg file not found\n");
		return;
	}

	if (ov_open_callbacks(file, &oggFile, NULL, 0, OV_CALLBACKS_DEFAULT) < 0) 
	{
		LogDxErrorString("file requested isn't a valid ogg file?\n");
		fclose(file);
		return;
	}

	// get some audio info
	pInfo = ov_info(&oggFile, -1);

	/* Check the number of channels... always use 16-bit samples */
	if (pInfo->channels == 1) OutputDebugString("Mono Vorbis\n");
	else OutputDebugString("Stereo Vorbis\n");

//	freq = pInfo->rate;
	char buf[100];
	sprintf(buf, "Vorbis frequency: %d\n", pInfo->rate);
	OutputDebugString(buf);

	/* create the audio buffer (directsound or whatever) */
	CreateVorbisAudioBuffer(pInfo->channels, pInfo->rate, &bufferSize);

	/* init some temp audio data storage */
	audioData = new char[bufferSize];

	halfBufferSize = bufferSize / 2;

#ifdef WIN32
	int totalRead = ReadVorbisData(bufferSize, 0);
#endif
#ifdef _XBOX
	ProcessStreamingAudio();
#endif

#if 0
	while(bytesReadTotal < bufferSize) 
	{
		bytesReadPerLoop = ov_read(
			&oggFile,							//what file to read from
			audioData + bytesReadTotal,			//where to put the decoded data
			bufferSize - bytesReadTotal,		//how much data to read
			0,									//0 specifies little endian decoding mode
			2,									//2 specifies 16-bit samples
			1,									//1 specifies signed data
			0
		);

		if(bytesReadPerLoop < 0) 
		{
			LogDxErrorString("ov_read encountered an error\n");
		}
		/* if we reach the end of the file, go back to start */
		if(bytesReadPerLoop == 0)
		{
			ov_raw_seek(&oggFile, 0);
		}

		bytesReadTotal += bytesReadPerLoop;
	}
#endif

	/* fill entire buffer initially */
#ifdef WIN32
	UpdateVorbisAudioBuffer(audioData, /*bytesReadTotal*/totalRead, 0);
#endif
	/* start playing */
	PlayVorbis();
}

void UpdateVorbisBuffer(void *arg) 
{
	int lockOffset = 0;

	char buf[100];

	DWORD dwQuantum = 1000 / 60;

	while(oggIsPlaying)
	{
#ifdef _XBOX
		ProcessStreamingAudio();
		Sleep( dwQuantum );
#else
		int wait_value = WaitForMultipleObjects(2, hHandles, FALSE, 1);

		if((wait_value != WAIT_TIMEOUT) && (wait_value != WAIT_FAILED))
		{
			if (wait_value == 0) 
			{
				lockOffset = 0;
			}
			else 
			{
				lockOffset = halfBufferSize;
			}

			int totalRead = ReadVorbisData(halfBufferSize, 0);
#if 0
			bytesReadPerLoop = 0;
			bytesReadTotal = 0;

			while(bytesReadTotal < halfBufferSize) 
			{
				bytesReadPerLoop = ov_read(
					&oggFile,								//what file to read from
					audioData + bytesReadTotal,				//destination + offset into destination data
					halfBufferSize - bytesReadTotal,		//how much data to read
					0,										//0 specifies little endian decoding mode
					2,										//2 specifies 16-bit samples
					1,										//1 specifies signed data
					0
				);

				bytesReadTotal += bytesReadPerLoop;

				if (bytesReadPerLoop < 0) 
				{
					LogDxErrorString("ov_read encountered an error\n");
				}
				/* if we reach the end of the file, go back to start */
				if (bytesReadPerLoop == 0)
				{
					OutputDebugString("end of ogg file\n");
					ov_raw_seek(&oggFile, 0);
				}
				if (bytesReadPerLoop == OV_HOLE) 
				{
					LogDxErrorString("OV_HOLE\n");
				}
				if (bytesReadPerLoop == OV_EBADLINK) 
				{
					LogDxErrorString("OV_EBADLINK\n");
				}
			}
#endif
			UpdateVorbisAudioBuffer(audioData, /*bytesReadTotal*/totalRead, lockOffset);
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
		LogDxErrorString("couldn't play ogg vorbis buffer\n");
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
		LogDxErrorString("no music tracklist found - not using ogg vorbis music\n");
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
//			OutputDebugString("\n Added track:");
//			OutputDebugString(trackName.c_str());
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