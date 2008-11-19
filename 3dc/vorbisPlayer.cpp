#include <vorbis/vorbisfile.h>
#include <stdio.h>
#include "vorbisPlayer.h"
#include "logString.h"
#include "dsound.h"
#include <process.h>

#include <vector>
#include <string>
#include <fstream>

extern "C" {
	extern LPDIRECTSOUND DSObject;
	extern LPDIRECTSOUNDBUFFER vorbisBuffer;
	extern int CreateVorbisAudioBuffer(int channels, int rate, unsigned int *bufferSize);
	extern int UpdateVorbisAudioBuffer(char *audioData, int dataSize, int offset);
	extern int SetVorbisBufferVolume(int volume);
	extern int CDPlayerVolume; // volume control from menus
	extern HANDLE hHandles[2];
}

FILE* file;
vorbis_info *pInfo;
OggVorbis_File oggFile;

unsigned int bytesReadTotal		= 0; //keep track of how many bytes we have read so far
unsigned int bytesReadPerLoop	= 0; //keep track of how many bytes we read per ov_read invokation (1 to ensure that while loop is entered below)

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

#pragma comment(lib, "vorbisfile_static.lib")
#pragma comment(lib, "ogg_static.lib")
#pragma comment(lib, "vorbis_static.lib")

char *audioData;

void LoadVorbisTrack(int track) 
{
	/* check if we have some files to play */
//	if(!loadOggTrackList()) return;

	/* if we're already playing a track, stop it */
	if (oggIsPlaying) StopVorbis();

	/* if user enters 1, decrement to 0 to align to array (enters 2, decrement to 1 etc) */
	if(track != 0) track--;

	if (track > TrackList.size()) return;

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
	if (pInfo->channels == 1) OutputDebugString("\n MONO");
	else OutputDebugString("\n STEREO");

//	freq = pInfo->rate;
	char buf[100];
	sprintf(buf, "\n frequency: %d", pInfo->rate);
	OutputDebugString(buf);

	/* create the audio buffer (directsound or whatever) */
	CreateVorbisAudioBuffer(pInfo->channels, pInfo->rate, &bufferSize);

	/* init some temp audio data storage */
	audioData = new char[bufferSize];
	
	halfBufferSize = bufferSize / 2;

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

	/* fill entire buffer initially */
	UpdateVorbisAudioBuffer(audioData, bytesReadTotal, 0);

	/* start playing */
	PlayVorbis();
}

void UpdateVorbisBuffer(void *arg) 
{
	int lockOffset = 0;

	OutputDebugString("created vorbis thread\n");

	while(oggIsPlaying)
	{
		int wait_value = WaitForMultipleObjects(2, hHandles, FALSE, 1);

		if((wait_value != WAIT_TIMEOUT) && (wait_value != WAIT_FAILED))
		{
			if(wait_value == 0) 
			{
				lockOffset = 0;
				OutputDebugString("locking at offset 0\n");
			}
			else 
			{
				OutputDebugString("locking at halfway offset\n");
				lockOffset = halfBufferSize;
			}

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

				if(bytesReadPerLoop < 0) 
				{
					LogDxErrorString("ov_read encountered an error\n");
				}
				/* if we reach the end of the file, go back to start */
				if(bytesReadPerLoop == 0)
				{
					OutputDebugString("\n end of ogg file");
					ov_raw_seek(&oggFile, 0);
				}
				if(bytesReadPerLoop == OV_HOLE) 
				{
					LogDxErrorString("OV_HOLE\n");
				}
				if(bytesReadPerLoop == OV_EBADLINK) 
				{
					LogDxErrorString("OV_EBADLINK\n");
				}
			}
			UpdateVorbisAudioBuffer(audioData, bytesReadTotal, lockOffset);
		}
	}
}

void PlayVorbis() 
{
	if(FAILED(vorbisBuffer->Play(0,0,DSBPLAY_LOOPING)))
	{
		LogDxErrorString("couldn't play ogg vorbis buffer\n");
	}
	else 
	{
		oggIsPlaying = true;
		SetVorbisBufferVolume(CDPlayerVolume);
		 _beginthread(UpdateVorbisBuffer, 0, 0);
	}
}

void StopVorbis() 
{
	if (oggIsPlaying)
	{
		vorbisBuffer->Stop();
		oggIsPlaying = false;
	}
	ov_clear(&oggFile);
//	fclose(file);

	delete[] audioData;

	bytesReadTotal = 0;
	bytesReadPerLoop = 0;
}

// cleanup function
void CleanupVorbis()
{
	if(vorbisBuffer != NULL) 
	{
		vorbisBuffer->Release();
		vorbisBuffer = NULL;
	}
	// move to dx_audio.cpp 
//	CloseHandle(hHandles[0]);
//	CloseHandle(hHandles[1]);
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