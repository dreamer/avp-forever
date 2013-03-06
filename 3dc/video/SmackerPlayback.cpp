// Copyright (C) 2011 Barry Duncan. All Rights Reserved.
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

#include "SmackerPlayback.h"
#include "FmvPlayback.h"
#include "console.h"
#include <process.h>
#include <assert.h>
#include <new>

static const int kAudioBufferSize  = 4096;
static const int kAudioBufferCount = 3;
static const int kQuantum = 1000 / 60;

void *SmackerDecodeThread(void *args);
void *SmackerAudioThread(void *args);

SmackerPlayback::~SmackerPlayback()
{
	_fmvPlaying = false;

	// wait for threads to finish
	if (_decodeThreadInited) {
		pthread_join(_decodeThreadHandle, NULL);
	}
	if (_audioThreadInited) {
		pthread_join(_audioThreadHandle, NULL);
	}
	
	if (_frameCriticalSectionInited) {
		pthread_mutex_destroy(&_frameCriticalSection);
	}

	delete[] _frame;

	if (_nAudioTracks)
	{
		delete _audioStream;
		delete[] _audioData;
		delete[] _audioDataBuffer;
		delete _ringBuffer;
	}
}

int SmackerPlayback::Open(const std::string &fileName)
{
#ifdef _XBOX
	_fileName = "d:/";
	_fileName += fileName;
#else
	_fileName = fileName;
#endif

	// open the file
	_handle = Smacker_Open(_fileName.c_str());
	if (!_handle.isValid)
	{
		Con_PrintError("can't open Smacker file " + _fileName);
		return FMV_INVALID_FILE;
	}

	Smacker_GetFrameSize(_handle, _frameWidth, _frameHeight);

	_frame = new(std::nothrow) uint8_t[_frameWidth * _frameHeight];
	if (!_frame) {
		Con_PrintError("can't allocate frame memory for Smacker file " + _fileName);
		return FMV_ERROR;
	}

	_nAudioTracks = Smacker_GetNumAudioTracks(_handle);

	if (_nAudioTracks)
	{
		// get audio details (just for the first track)
		SmackerAudioInfo info = Smacker_GetAudioTrackDetails(_handle, 0);

		_sampleRate = info.sampleRate;

		// create mAudio streaming buffer
		_audioStream = new AudioStream();

		assert(info.bitsPerSample != 0);

		if (!_audioStream->Init(info.nChannels, info.sampleRate, info.bitsPerSample, kAudioBufferSize, kAudioBufferCount))
		{
			Con_PrintError("Failed to create _audioStream for Smacker playback");
			return FMV_ERROR;
		}

		// we need some temporary mAudio data storage space and a ring buffer instance
		_audioData = new(std::nothrow) uint8_t[info.idealBufferSize];
		if (_audioData == NULL)
		{
			Con_PrintError("Failed to create _audioData stream buffer for Smacker playback");
			return FMV_ERROR;
		}

		_ringBuffer = new(std::nothrow) RingBuffer();
		if (!_ringBuffer->Init(info.idealBufferSize * kAudioBufferCount))
		{
			Con_PrintError("Failed to create _ringBuffer for Smacker playback");
			return FMV_ERROR;
		}

		_audioDataBuffer = new(std::nothrow) uint8_t[info.idealBufferSize];
		_audioDataBufferSize = info.idealBufferSize;
	}

	_frameRate = Smacker_GetFrameRate(_handle);

	_fmvPlaying = true;
	_audioStarted = false;

	if (pthread_mutex_init(&_frameCriticalSection, NULL) != 0) {
		return false;
	}
	_frameCriticalSectionInited = true;

	// now start the threads
	if (pthread_create(&_decodeThreadHandle, NULL, SmackerDecodeThread, static_cast<void*>(this)) != 0) {
		return false;
	}
	_decodeThreadInited = true;

	if (_nAudioTracks) {
		if (pthread_create(&_audioThreadHandle, NULL, SmackerAudioThread, static_cast<void*>(this)) != 0) {
			return false;
		}
		_audioThreadInited = true;
	}

	return FMV_OK;
}

void SmackerPlayback::Close()
{
	_fmvPlaying = false;
}

bool SmackerPlayback::IsPlaying()
{
	return _fmvPlaying;
}

bool SmackerPlayback::ConvertFrame(uint32_t width, uint32_t height, uint8_t *bufferPtr, uint32_t pitch)
{
	if (!_fmvPlaying) {
		return false;
	}

	if (!bufferPtr)
	{
		_frameReady = false;
		return true;
	}

	// critical section
	pthread_mutex_lock(&_frameCriticalSection);

	Smacker_GetPalette(_handle, _palette);
	Smacker_GetFrame(_handle, _frame);

	uint32_t index = 0;

	for (uint32_t y = 0; y < _frameHeight; y++)
	{
		uint8_t *destPtr = bufferPtr + y * pitch;

		for (uint32_t x = 0; x < _frameWidth; x++)
		{
			*destPtr++ = _palette[(_frame[index] * 3) + 2];
			*destPtr++ = _palette[(_frame[index] * 3) + 1];
			*destPtr++ = _palette[(_frame[index] * 3) + 0];
			*destPtr++ = 255; // alpha
			index++;
		}
	}

	_frameReady = false;

	pthread_mutex_unlock(&_frameCriticalSection);

	return true;
}

void *SmackerDecodeThread(void *args)
{
	SmackerPlayback *fmv = static_cast<SmackerPlayback*>(args);

	uint32_t trackIndex = 0;

	uint32_t nFrames = Smacker_GetNumFrames(fmv->_handle);

	while (fmv->_currentFrame < nFrames)
	{
		// check if we should still be playing or not
		if (!fmv->IsPlaying()) {
			break;
		}

		uint32_t startTime = ::timeGetTime();

		// critical section
		pthread_mutex_lock(&fmv->_frameCriticalSection);

		Smacker_GetNextFrame(fmv->_handle);

		// we have a new frame and we're ready to use it
		fmv->_frameReady = true;

		pthread_mutex_unlock(&fmv->_frameCriticalSection);

		uint32_t audioSize = Smacker_GetAudioData(fmv->_handle, trackIndex, (int16_t*)fmv->_audioDataBuffer);

		if (audioSize)
		{
			uint32_t freeSpace = fmv->_ringBuffer->GetWritableSize();

			// if we can't fit all our data..
			if (audioSize > freeSpace)
			{
				while (audioSize > fmv->_ringBuffer->GetWritableSize())
				{
					// little bit of insurance in case we get stuck in here
					if (!fmv->_fmvPlaying) {
						break;
					}

					// wait for the audio buffer to tell us it's just freed up another audio buffer for us to fill
					fmv->_audioStream->WaitForFreeBuffer();
				}
			}

			fmv->_ringBuffer->WriteData(fmv->_audioDataBuffer, audioSize);
		}

		uint32_t endTime = ::timeGetTime();

		// sleep for frame time minus time to decode frame
		int32_t timeToSleep = (1000 / (int32_t)fmv->_frameRate) - (endTime - startTime);
		if (timeToSleep < 0) {
			timeToSleep = 1;
		}
		
		::Sleep(timeToSleep);

		fmv->_currentFrame++;
	}

	/*
	  We might get to this point because we've played all frames - rather than the game code asking us to stop, 
	  or the user interrupting the FMV. Setting _fmvPlaying to false will ensure that the audio decoding thread knows to quit
	 */
	fmv->_fmvPlaying = false;
	return 0;
}

void *SmackerAudioThread(void *args)
{
	#ifdef USE_XAUDIO2
		::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	#endif

	SmackerPlayback *fmv = static_cast<SmackerPlayback*>(args);
	
	int32_t startTime = 0;
	int32_t endTime = 0;
	int32_t timetoSleep = 0;

	while (fmv->_fmvPlaying)
	{
		startTime = ::timeGetTime();

		uint32_t nBuffersFree = fmv->_audioStream->GetNumFreeBuffers();

		while (nBuffersFree)
		{
			uint32_t readableAudio = fmv->_ringBuffer->GetReadableSize();

			// can we fill a buffer
			uint32_t bufferSize = fmv->_audioStream->GetBufferSize();

			if (readableAudio >= bufferSize)
			{
				fmv->_ringBuffer->ReadData  (fmv->_audioData, bufferSize);
				fmv->_audioStream->WriteData(fmv->_audioData, bufferSize);
				nBuffersFree--;

				if (fmv->_audioStarted == false)
				{
					fmv->_audioStream->Play();
					fmv->_audioStarted = true;
				}
			}
			else
			{
				// not enough _audioData available this time
				break;
			}
		}

		endTime = ::timeGetTime();

		timetoSleep = endTime - startTime;
		timetoSleep -= kQuantum;

		if (timetoSleep < 0 || timetoSleep > kQuantum) {
			timetoSleep = 1;
		}

		::Sleep(timetoSleep);
	}
	return 0;
}