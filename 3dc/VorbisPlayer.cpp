// Copyright (C) 2010 Barry Duncan. All Rights Reserved.
// The original author of this code can be contacted at: bduncan22@hotmail.com

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include "VorbisPlayer.h"
#include "logString.h"
#include <process.h>
#include <vector>
#include <fstream>
#include <assert.h>
#include "utilities.h" // avp_open()
#include "console.h"
#include <new>

#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

static const int kBufferSize  = 32768;
static const int kBufferCount = 3;
static DWORD kQuantum = 1000 / 60;

unsigned int __stdcall VorbisUpdateThread(void *args);

extern int SetStreamingMusicVolume(int volume);
extern int CDPlayerVolume; // volume control from menus

std::vector<std::string> TrackList;

#ifdef WIN32
	const std::string tracklistFilename = "Music/ogg_tracks.txt";
	const std::string musicFolderName = "Music/";
#endif
#ifdef _XBOX
	const std::string tracklistFilename = "d:/Music/ogg_tracks.txt";
	const std::string musicFolderName = "d:/Music/";
#endif

static VorbisPlayback *inGameMusic = NULL;

bool VorbisPlayback::Open(const std::string &fileName)
{
	mVorbisFile = fopen(fileName.c_str(), "rb");

	if (!mVorbisFile)
	{
		Con_PrintError("Can't find OGG Vorbis file " + fileName);
		return false;
	}

	if (ov_open_callbacks(mVorbisFile, &mOggFile, NULL, 0, OV_CALLBACKS_DEFAULT) < 0)
	{
		Con_PrintError("File " + fileName + "is not a valid OGG Vorbis file");
		return false;
	}

	// get some audio info
	mVorbisInfo = ov_info(&mOggFile, -1);

	// create the streaming audio buffer
	this->audioStream = new(std::nothrow) AudioStream(mVorbisInfo->channels, mVorbisInfo->rate, kBufferSize, kBufferCount);
	if (audioStream == NULL)
	{
		return false;
	}

	// init some temp audio data storage
	mAudioData = new(std::nothrow) uint8_t[kBufferSize];
	if (mAudioData == NULL)
	{
		Con_PrintError("Couldn't allocate memory for Vorbis temp buffer");
		return false;
	}

	GetVorbisData(kBufferSize);

	// fill the first buffer
	this->audioStream->WriteData(mAudioData, kBufferSize);

	// start playing
	if (this->audioStream->Play() == AUDIOSTREAM_OK)
	{
		mIsPlaying = true;
		this->audioStream->SetVolume(CDPlayerVolume);
		mPlaybackThreadFinished = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, VorbisUpdateThread, static_cast<void*>(this), 0, NULL));
	}
	else
	{
		LogErrorString("couldn't play ogg vorbis buffer\n");
	}

	return true;
}

VorbisPlayback::~VorbisPlayback()
{
	Stop();

	delete audioStream;

	// clear out the OggVorbis_File struct
	ov_clear(&mOggFile);

	delete[] mAudioData;
}

uint32_t VorbisPlayback::GetVorbisData(uint32_t sizeToRead)
{
	uint32_t bytesReadTotal = 0;
	int32_t bytesReadPerLoop = 0;

	while (bytesReadTotal < sizeToRead)
	{
		bytesReadPerLoop = ov_read(
			&mOggFile,									// what file to read from
			reinterpret_cast<char*>(mAudioData + bytesReadTotal),
			sizeToRead - bytesReadTotal,				// how much data to read
			0,											// 0 specifies little endian decoding mode
			2,											// 2 specifies 16-bit samples
			1,											// 1 specifies signed data
			0
		);

		if (bytesReadPerLoop < 0)
		{
			LogErrorString("ov_read encountered an error", __LINE__, __FILE__);
		}
		// if we reach the end of the file, go back to start
		else if (bytesReadPerLoop == 0)
		{
			ov_raw_seek(&mOggFile, 0);
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

unsigned int __stdcall VorbisUpdateThread(void *args)
{
	VorbisPlayback *vorbis = static_cast<VorbisPlayback*>(args);

#ifdef USE_XAUDIO2
	CoInitializeEx (NULL, COINIT_MULTITHREADED);
#endif

	while (vorbis->mIsPlaying)
	{
		uint32_t numBuffersFree = vorbis->audioStream->GetNumFreeBuffers();

		if (numBuffersFree)
		{
			vorbis->GetVorbisData(kBufferSize);
			vorbis->audioStream->WriteData(vorbis->mAudioData, kBufferSize);
		}

		Sleep(kQuantum);
	}

	_endthreadex(0);
	return 0;
}

void VorbisPlayback::Stop()
{
	if (mIsPlaying)
	{
		this->audioStream->Stop();
		mIsPlaying = false;
	}

	// wait until audio processing thread has finished running before continuing
	if (mPlaybackThreadFinished)
	{
		WaitForSingleObject(mPlaybackThreadFinished, INFINITE);
		CloseHandle(mPlaybackThreadFinished);
	}
}

void LoadVorbisTrack(size_t track)
{
	// if we're already playing a track, stop it
	if (inGameMusic)
	{
		delete inGameMusic;
		inGameMusic = NULL;
	}

	// TODO? rather than return, pick a random track or just play last?
	if (track > TrackList.size())
	{
		return;
	}

	// if user enters 1, decrement to 0 to align to array (enters 2, decrement to 1 etc)
	if (track != 0)
	{
		track--;
	}

	inGameMusic = new VorbisPlayback;
	if (!inGameMusic->Open(TrackList[track]))
	{
		delete inGameMusic;
		inGameMusic = NULL;
	}
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

	while (std::getline(file, trackName))
	{
		TrackList.push_back(musicFolderName + trackName);
	}

	file.close();
	return true;
}

size_t CheckNumberOfVorbisTracks()
{
	return TrackList.size();
}

bool IsVorbisPlaying()
{
	if (inGameMusic)
	{
		return inGameMusic->mIsPlaying;
	}
	
	return false;
}

int SetStreamingMusicVolume(int volume)
{
	if (inGameMusic) // hack to stop this call before the audio stream is initialised
	{
		return inGameMusic->audioStream->SetVolume(volume);
	}
	
	return 0;
}

void Vorbis_CloseSystem()
{
	delete inGameMusic;
	inGameMusic = NULL;
}

