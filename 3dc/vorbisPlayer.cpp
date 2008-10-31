#include <vorbis/vorbisfile.h>
#include <stdio.h>
#include <windows.h>
#include "vorbisPlayer.h"

#include "dsound.h"

#include "Dxerr9.h"
//#pragma comment(lib, "Dxerr8.lib")

#include <vector>
#include <string>
#include <fstream>

extern "C" {
	extern LPDIRECTSOUND DSObject;
}

FILE* file;
vorbis_info *pInfo;
OggVorbis_File oggFile;

unsigned int BytesReadTotal		= 0; //keep track of how many bytes we have read so far
unsigned int BytesReadPerLoop	= 0; //keep track of how many bytes we read per ov_read invokation (1 to ensure that while loop is entered below)
//int nBitStream				= 0; //used to specify logical bitstream 0

LPVOID audioPtr1;
DWORD audioBytes1;
LPVOID audioPtr2;   
DWORD audioBytes2;

LPDIRECTSOUNDBUFFER vorbisBuffer = NULL;
DSBUFFERDESC dsbufferdesc;
LPDIRECTSOUNDNOTIFY pDSNotify = NULL;
HANDLE hHandles[2];

WAVEFORMATEX waveFormat;

bool ogg_is_playing = false;
unsigned int buffer_size = 0;
unsigned int half_buffer_size = 0;

std::vector<std::string> TrackList;
const std::string tracklistFilename = "Music/ogg_tracks.txt";
const std::string musicFolderName = "Music/";

#pragma comment(lib, "vorbisfile_static.lib")
#pragma comment(lib, "ogg_static.lib")
#pragma comment(lib, "vorbis_static.lib")
// for dsound notify
#pragma comment(lib,"dxguid.lib")


void LoadVorbisTrack(int track) 
{
	/* check if we have some files to play */
//	if(!loadOggTrackList()) return;

	/* if we're already playing a track, stop it */
	if (ogg_is_playing) StopVorbis();

	/* if user enters 1, decrement to 0 to align to array (enters 2, decrement to 1 etc) */
	if(track != 0) track--;

	if (track > TrackList.size()) return;

	file = fopen(TrackList[track].c_str(),"rb");
	if (!file) 
	{
		OutputDebugString("\n ogg file not found");
		return;
	}

	if (ov_open_callbacks(file, &oggFile, NULL, 0, OV_CALLBACKS_DEFAULT) < 0) 
	{
		OutputDebugString("\n not an ogg file?");
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

	buffer_size = dsbufferdesc.dwBufferBytes;
	half_buffer_size = dsbufferdesc.dwBufferBytes / 2;

	if(FAILED(DSObject->CreateSoundBuffer(&dsbufferdesc, &vorbisBuffer, NULL)))
	{
		OutputDebugString("\n couldnt create buffer");
	}

	// lock the entire buffer
	if(FAILED(vorbisBuffer->Lock(0, buffer_size,
		&audioPtr1,&audioBytes1,&audioPtr2,&audioBytes2,DSBLOCK_ENTIREBUFFER))) 
	{
			OutputDebugString("\n couldn't lock buffer");
	}

	// ov_read wont read all the data we request in one go
	// we need to loop, adding to our buffer and keeping track of how much its 
	// reading per loop, to ensure we fill our buffer

	while(BytesReadTotal < buffer_size) 
	{
		BytesReadPerLoop = ov_read(
			&oggFile,							//what file to read from
			(char*)audioPtr1 + BytesReadTotal,	//where to put the decoded data
			buffer_size - BytesReadTotal,		//how much data to read
			0,									//0 specifies little endian decoding mode
			2,									//2 specifies 16-bit samples
			1,									//1 specifies signed data
			0
		);

		if(BytesReadPerLoop < 0) 
		{
			OutputDebugString("\n ov_read error");
		}
		// if we reach the end of the file, go back to start
		if(BytesReadPerLoop == 0)
		{
			ov_raw_seek(&oggFile, 0);
		}

		BytesReadTotal += BytesReadPerLoop;
	}

	if(FAILED(vorbisBuffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2))) 
	{
		OutputDebugString("\n couldn't unlock ds buffer");
	}

	if(FAILED(vorbisBuffer->QueryInterface( IID_IDirectSoundNotify, 
                                        (void**)&pDSNotify ) ) )
	{
		OutputDebugString("\n query interface for dsnotify didnt work");
	}

	DSBPOSITIONNOTIFY notifyPosition[2];

	hHandles[0] = CreateEvent(NULL,FALSE,FALSE,NULL);
	hHandles[1] = CreateEvent(NULL,FALSE,FALSE,NULL);

	// halfway
	notifyPosition[0].dwOffset = half_buffer_size;
	notifyPosition[0].hEventNotify = hHandles[0];

	// end
	notifyPosition[1].dwOffset = buffer_size - 1;
	notifyPosition[1].hEventNotify = hHandles[1];

	if(FAILED(pDSNotify->SetNotificationPositions(2, notifyPosition)))
	{
		OutputDebugString("\n couldn't set notifications for ogg");
		pDSNotify->Release();
		return;
	}

	hHandles[0] = notifyPosition[0].hEventNotify;
	hHandles[1] = notifyPosition[1].hEventNotify;

	pDSNotify->Release();
	pDSNotify = NULL;

	// start playing
	PlayVorbis();
}

//#include <process.h>

void UpdateVorbisBuffer() 
//static unsigned __cdecl updateOggBuffer(void*)
{
	if (ogg_is_playing != true) return;

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
		lockOffset = half_buffer_size;
	}

	/* lock buffer at offset */
	if(FAILED(vorbisBuffer->Lock(0, half_buffer_size, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, NULL))) 
	{
		OutputDebugString("couldn't lock buffer\n");
		return;
	}

	if(audioBytes1 != half_buffer_size) 
	{
		OutputDebugString("couldn't lock desired half of buffer\n");
	}

	if(audioPtr2) 
	{
		OutputDebugString("Ogg player needs to loop\n");
	}

	// ov_read wont read all the data we request in one go
	// we need to loop, adding to our buffer and keeping track of how much its 
	// reading per loop, to ensure we fill our buffer
	BytesReadPerLoop = 0;
	BytesReadTotal = 0;

	while(BytesReadTotal < half_buffer_size) 
	{
		BytesReadPerLoop = ov_read(
			&oggFile,								//what file to read from
			(char*)audioPtr1 + BytesReadTotal,		//destination + offset into destination data
			half_buffer_size - BytesReadTotal,		//how much data to read
			0,										//0 specifies little endian decoding mode
			2,										//2 specifies 16-bit samples
			1,										//1 specifies signed data
			0
		);

		BytesReadTotal += BytesReadPerLoop;

		if(BytesReadPerLoop < 0) 
		{
			OutputDebugString("\n ov_read error");
		}
		// if we reach the end of the file, go back to start
		if(BytesReadPerLoop == 0)
		{
			OutputDebugString("\n end of ogg file");
			ov_raw_seek(&oggFile, 0);
		}
		if(BytesReadPerLoop == OV_HOLE) 
		{
			OutputDebugString("\n OV_HOLE");
		}
		if(BytesReadPerLoop == OV_EBADLINK) {
			OutputDebugString("\n OV_EBADLINK");
		}
	}

	if(FAILED(vorbisBuffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2)))
	{	
		OutputDebugString("\n couldn't unlock ds buffer");
	}
}

void PlayVorbis() 
{
	if(FAILED(vorbisBuffer->Play(0,0,DSBPLAY_LOOPING)))
	{
		OutputDebugString("\n couldnt play ds buffer");
	}
	else {
		ogg_is_playing = true;
	}
}

void StopVorbis() 
{
	if (ogg_is_playing)
	{
		vorbisBuffer->Stop();
	}
	ov_clear(&oggFile);
//	fclose(file);
	ogg_is_playing = false;

	BytesReadTotal = 0;
	BytesReadPerLoop = 0;
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
		OutputDebugString("\n can't find tracklist file");
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
			OutputDebugString("\n Added track:");
			OutputDebugString(trackName.c_str());
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
	return ogg_is_playing;
}	