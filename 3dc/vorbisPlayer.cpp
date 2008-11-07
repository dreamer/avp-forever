#include <vorbis/vorbisfile.h>
#include <stdio.h>
#include "vorbisPlayer.h"
#include "logString.h"
#include "dsound.h"

#include <vector>
#include <string>
#include <fstream>

extern "C" {
	extern LPDIRECTSOUND DSObject;
	extern LPDIRECTSOUNDBUFFER vorbisBuffer;
//	extern struct AUDIOBUFFER vorbisBuffer;
	extern int CreateVorbisAudioBuffer(int channels, int rate, unsigned int *bufferSize);
	extern int UpdateVorbisAudioBuffer(char *audioData, int dataSize, int offset);
}

FILE* file;
vorbis_info *pInfo;
OggVorbis_File oggFile;

unsigned int bytesReadTotal		= 0; //keep track of how many bytes we have read so far
unsigned int bytesReadPerLoop	= 0; //keep track of how many bytes we read per ov_read invokation (1 to ensure that while loop is entered below)
//int nBitStream				= 0; //used to specify logical bitstream 0

LPVOID audioPtr1;
DWORD audioBytes1;
LPVOID audioPtr2;   
DWORD audioBytes2;

//extern LPDIRECTSOUNDBUFFER vorbisBuffer;
DSBUFFERDESC dsbufferdesc;
LPDIRECTSOUNDNOTIFY pDSNotify = NULL;
HANDLE hHandles[2];

WAVEFORMATEX waveFormat;

bool oggIsPlaying = false;
unsigned int bufferSize = 0;
unsigned int halfBufferSize = 0;

std::vector<std::string> TrackList;
const std::string tracklistFilename = "Music/ogg_tracks.txt";
const std::string musicFolderName = "Music/";

#pragma comment(lib, "vorbisfile_static.lib")
#pragma comment(lib, "ogg_static.lib")
#pragma comment(lib, "vorbis_static.lib")
// for dsound notify
//#pragma comment(lib,"dxguid.lib")

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

	// Check the number of channels... always use 16-bit samples
	if (pInfo->channels == 1) OutputDebugString("\n MONO");
	else OutputDebugString("\n STEREO");

//	freq = pInfo->rate;
	char buf[100];
	sprintf(buf, "\n frequency: %d", pInfo->rate);
	OutputDebugString(buf);
#if 0
	memset(&waveFormat, 0, sizeof(waveFormat));
	waveFormat.cbSize			= sizeof(waveFormat);	//how big this structure is
	waveFormat.nChannels		= pInfo->channels;	//how many channels the OGG contains
	waveFormat.wBitsPerSample	= 16;					//always 16 in OGG
	waveFormat.nSamplesPerSec	= pInfo->rate;	
	waveFormat.nAvgBytesPerSec	= waveFormat.nSamplesPerSec * waveFormat.nChannels * 2;	//average bytes per second
	waveFormat.nBlockAlign		= 2 * waveFormat.nChannels;								//what block boundaries exist
	waveFormat.wFormatTag		= 1;

	memset(&dsbufferdesc, 0, sizeof(dsbufferdesc));

	dsbufferdesc.dwSize			= sizeof(dsbufferdesc);		//how big this structure is
	dsbufferdesc.lpwfxFormat	= &waveFormat;						//information about the sound that the buffer will contain
	dsbufferdesc.dwBufferBytes	= waveFormat.nAvgBytesPerSec * 2;				//total buffer size = 2 * half size
	dsbufferdesc.dwFlags		= DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE;		//buffer must support notifications

	bufferSize = dsbufferdesc.dwBufferBytes;
	halfBufferSize = dsbufferdesc.dwBufferBytes / 2;

	if(FAILED(DSObject->CreateSoundBuffer(&dsbufferdesc, &vorbisBuffer, NULL)))
	{
		LogDxErrorString("couldn't create buffer for ogg vorbis\n");
	}
#endif

	CreateVorbisAudioBuffer(pInfo->channels, pInfo->rate, &bufferSize);

	/* init some temp audio data storage */
	audioData = new char[bufferSize];
	
	halfBufferSize = bufferSize / 2;

#if 0
	// lock the entire buffer
	if(FAILED(vorbisBuffer->Lock(0, bufferSize,
		&audioPtr1,&audioBytes1,&audioPtr2,&audioBytes2,DSBLOCK_ENTIREBUFFER))) 
	{
		LogDxErrorString("couldn't lock ogg vorbis buffer for update\n");
	}

	// ov_read wont read all the data we request in one go
	// we need to loop, adding to our buffer and keeping track of how much its 
	// reading per loop, to ensure we fill our buffer
#endif

	while(bytesReadTotal < bufferSize) 
	{
		bytesReadPerLoop = ov_read(
			&oggFile,							//what file to read from
			/*(char*)audioPtr1*/audioData + bytesReadTotal,	//where to put the decoded data
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
		// if we reach the end of the file, go back to start
		if(bytesReadPerLoop == 0)
		{
			ov_raw_seek(&oggFile, 0);
		}

		bytesReadTotal += bytesReadPerLoop;
	}

	/* fill entire buffer initially */
	UpdateVorbisAudioBuffer(audioData, bytesReadTotal, 0);

#if 0
	if(FAILED(vorbisBuffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2))) 
	{
		LogDxErrorString("couldn't unlock ogg vorbis buffer\n");
	}

	if(FAILED(vorbisBuffer->QueryInterface( IID_IDirectSoundNotify, 
                                        (void**)&pDSNotify ) ) )
	{
		LogDxErrorString("couldn't query interface for ogg vorbis buffer notifications\n");
	}

	DSBPOSITIONNOTIFY notifyPosition[2];

	hHandles[0] = CreateEvent(NULL,FALSE,FALSE,NULL);
	hHandles[1] = CreateEvent(NULL,FALSE,FALSE,NULL);

	// halfway
	notifyPosition[0].dwOffset = halfBufferSize;
	notifyPosition[0].hEventNotify = hHandles[0];

	// end
	notifyPosition[1].dwOffset = bufferSize - 1;
	notifyPosition[1].hEventNotify = hHandles[1];

	if(FAILED(pDSNotify->SetNotificationPositions(2, notifyPosition)))
	{
		LogDxErrorString("couldn't set notifications for ogg vorbis buffer\n");
		pDSNotify->Release();
		return;
	}

	hHandles[0] = notifyPosition[0].hEventNotify;
	hHandles[1] = notifyPosition[1].hEventNotify;

	pDSNotify->Release();
	pDSNotify = NULL;
#endif

	// start playing
	PlayVorbis();
}

//#include <process.h>

void UpdateVorbisBuffer() 
//static unsigned __cdecl updateOggBuffer(void*)
{
	if (oggIsPlaying != true) return;

	int wait_value = WaitForMultipleObjects(2, hHandles, FALSE,1);

	if (wait_value == WAIT_TIMEOUT) return;
	else if (wait_value == WAIT_FAILED) return;

	wait_value -= WAIT_OBJECT_0;

	int lockOffset = 0;
	
	if(wait_value == 0) 
	{
		lockOffset = 0;
	}
	else 
	{
		lockOffset = halfBufferSize;
	}
#if 0
	/* lock buffer at offset */
	if(FAILED(vorbisBuffer->Lock(lockOffset, halfBufferSize, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, NULL))) 
	{
		LogDxErrorString("couldn't lock ogg vorbis buffer for update\n");
		return;
	}

	if(audioBytes1 != halfBufferSize) 
	{
		LogDxErrorString("couldn't lock desired half of ogg vorbis buffer\n");
	}

	if(audioPtr2) 
	{
		//OutputDebugString("Ogg player needs to loop\n");
	}
#endif
	// ov_read wont read all the data we request in one go
	// we need to loop, adding to our buffer and keeping track of how much its 
	// reading per loop, to ensure we fill our buffer
	bytesReadPerLoop = 0;
	bytesReadTotal = 0;

	while(bytesReadTotal < halfBufferSize) 
	{
		bytesReadPerLoop = ov_read(
			&oggFile,								//what file to read from
			/*(char*)audioPtr1*/audioData + bytesReadTotal,		//destination + offset into destination data
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
		// if we reach the end of the file, go back to start
		if(bytesReadPerLoop == 0)
		{
			OutputDebugString("\n end of ogg file");
			ov_raw_seek(&oggFile, 0);
		}
		if(bytesReadPerLoop == OV_HOLE) 
		{
			LogDxErrorString("OV_HOLE\n");
		}
		if(bytesReadPerLoop == OV_EBADLINK) {
			LogDxErrorString("OV_EBADLINK\n");
		}
	}

	UpdateVorbisAudioBuffer(audioData, bytesReadTotal, lockOffset);

/*
	if(FAILED(vorbisBuffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2)))
	{	
		LogDxErrorString("couldn't unlock ogg vorbis buffer\n");
	}
*/
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
	}
}

void StopVorbis() 
{
	if (oggIsPlaying)
	{
		vorbisBuffer->Stop();
	}
	ov_clear(&oggFile);
//	fclose(file);
	oggIsPlaying = false;

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

	CloseHandle(hHandles[0]);
	CloseHandle(hHandles[1]);
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