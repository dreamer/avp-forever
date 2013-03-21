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

#include "BinkPlayback.h"
#include "FmvPlayback.h"
#include "console.h"
#include <assert.h>
#include "utilities.h"

static const int kAudioBufferSize  = 4096;
static const int kAudioBufferCount = 3;
static const int kQuantum = 1000 / 60;

void *BinkDecodeThread(void *args);
void *BinkAudioThread(void *args);

#ifdef _XBOX
	#include <algorithm>
#endif

int BinkPlayback::Open(const std::string &fileName, bool isLooped)
{
#ifdef _XBOX
	_fileName = "d:/";
	_fileName += fileName;
#else
	_fileName = fileName;
#endif

	_isLooped = isLooped;

#ifdef _XBOX

	// change forwardslashes in path to backslashes
	std::replace(_fileName.begin(), _fileName.end(), '/', '\\');

#endif

	// open the file
	_handle = Bink_Open(_fileName.c_str());
	if (!_handle.isValid)
	{
		Con_PrintError("can't open Bink file " + _fileName);
		return FMV_INVALID_FILE;
	}

	_frameRate = Bink_GetFrameRate(_handle);

	// get number of audio tracks available
	_nAudioTracks = Bink_GetNumAudioTracks(_handle);

	_audioTrackInfos.resize(_nAudioTracks);

	// init the sound and video api stuff we need
	if (_nAudioTracks)
	{
		// get audio information for all available tracks
		for (uint32_t i = 0; i < _audioTrackInfos.size(); i++) {
			_audioTrackInfos[i] = Bink_GetAudioTrackDetails(_handle, i);
		}

		_audioTrackIndex = 0;

		uint32_t idealBufferSize = _audioTrackInfos[_audioTrackIndex].idealBufferSize;

		// create AudioStream (just for 1 track for now)
		_audioStream = new AudioStream();
		if (!_audioStream->Init(_audioTrackInfos[_audioTrackIndex].nChannels, _audioTrackInfos[_audioTrackIndex].sampleRate, 16, kAudioBufferSize, kAudioBufferCount))
		{
			Con_PrintError("Failed to create _audioStream for Bink playback");
			return FMV_ERROR;
		}

		// we need some temporary mAudio data storage space and a ring buffer instance
		_audioData = new(std::nothrow) uint8_t[idealBufferSize];
		if (_audioData == NULL)
		{
			Con_PrintError("Failed to create _audioData stream buffer for Bink playback");
			return FMV_ERROR;
		}

		_ringBuffer = new(std::nothrow) RingBuffer();
		if (!_ringBuffer->Init(idealBufferSize * kAudioBufferCount))
		{
			Con_PrintError("Failed to create _ringBuffer for Bink playback");
			return FMV_ERROR;
		}

		_audioDataBuffer = new(std::nothrow) uint8_t[idealBufferSize];
		_audioDataBufferSize = idealBufferSize;
	}

	Bink_GetFrameSize(_handle, _frameWidth, _frameHeight);

	// determine how many textures we need
	if (/* can gpu do shader yuv->rgb? */ 1) // TODO
	{
		_frameTextureIDs.resize(3);
		_frameTextureIDs[0] = MISSING_TEXTURE;
		_frameTextureIDs[1] = MISSING_TEXTURE;
		_frameTextureIDs[2] = MISSING_TEXTURE;
		_nTextureBits = 8;
	}
	else
	{
		// do CPU YUV->RGB with a single 32bit texture
		_frameTextureIDs.resize(1);
		_frameTextureIDs[0] = MISSING_TEXTURE;
		_nTextureBits = 32;
	}

	for (uint32_t i = 0; i < _frameTextureIDs.size(); i++)
	{
		uint32_t width  = _frameWidth;
		uint32_t height = _frameHeight;

		// the first texture is fullsize, the second two are quartersize
		if (i > 0)
		{
			width  /= 2;
			height /= 2;
		}

		// create the texture with desired parameters
		_frameTextureIDs[i] = Tex_Create(/*"CUTSCENE_"*/fileName + Util::IntToString(i), width, height, _nTextureBits, TextureUsage_Dynamic);
		if (_frameTextureIDs[i] == MISSING_TEXTURE)
		{
			Con_PrintError("Unable to create texture(s) for FMV playback");
			return FMV_ERROR;
		}
	}

	_fmvPlaying   = true;
	_audioStarted = false;
	_frameReady   = false;
	_texturesReady = false;

	if (pthread_mutex_init(&_frameCriticalSection, NULL) != 0) {
		return false;
	}
	_frameCriticalSectionInited = true;

	// now start the threads
	if (pthread_create(&_decodeThreadHandle, NULL, BinkDecodeThread, static_cast<void*>(this)) != 0) {
		return false;
	}

	if (_nAudioTracks) {
		if (pthread_create(&_audioThreadHandle, NULL, BinkAudioThread, static_cast<void*>(this)) != 0) {
			return false;
		}
		_audioThreadInited  = true;
	}

	_decodeThreadInited = true;

	return FMV_OK;
}

BinkPlayback::~BinkPlayback()
{
	_fmvPlaying = false;

	if (_decodeThreadInited) {
		pthread_join(_decodeThreadHandle, NULL);
	}
	if (_audioThreadInited) {
		pthread_join(_audioThreadHandle, NULL);
	}

	if (_frameCriticalSectionInited)
	{
		pthread_mutex_destroy(&_frameCriticalSection);
		_frameCriticalSectionInited = false;
	}

	if (_nAudioTracks)
	{
		delete _audioStream;
		delete[] _audioData;
		delete[] _audioDataBuffer;
		delete _ringBuffer;
	}

	for (uint32_t i = 0; i < _frameTextureIDs.size(); i++) {
		Tex_Release(_frameTextureIDs[i]);
	}
}

void BinkPlayback::Close()
{
	_fmvPlaying = false;
}

bool BinkPlayback::IsPlaying()
{
	return _fmvPlaying;
}

// copies a decoded Theora YUV frame to texture(s) for GPU to convert via shader
bool BinkPlayback::ConvertFrame()
{
	if (!_fmvPlaying) {
		return false;
	}

	// critical section
	pthread_mutex_lock(&_frameCriticalSection);

	for (uint32_t i = 0; i < _frameTextureIDs.size(); i++)
	{
		uint8_t *originalDestPtr = NULL;
		uint32_t pitch = 0;

		uint32_t width  = 0;
		uint32_t height = 0;

		// get width and height
		Tex_GetDimensions(_frameTextureIDs[i], width, height);

		// lock the texture
		if (Tex_Lock(_frameTextureIDs[i], &originalDestPtr, &pitch, TextureLock_Discard)) // only do below if lock succeeds
		{
			for (uint32_t y = 0; y < height; y++)
			{
				uint8_t *destPtr = originalDestPtr + (y * pitch);
				uint8_t *srcPtr  = _yuvBuffer[i].data + (y * _yuvBuffer[i].pitch);

				// copy entire width row in one go
				memcpy(destPtr, srcPtr, width * sizeof(uint8_t));

				destPtr += width * sizeof(uint8_t);
				srcPtr  += width * sizeof(uint8_t);
			}

			// unlock texture
			Tex_Unlock(_frameTextureIDs[i]);
		}
	}

	// set this value to true so we can now begin to draw the textured fmv frame
	_texturesReady = true;

	_frameReady = false;

	pthread_mutex_unlock(&_frameCriticalSection);

	return true;
}

void *BinkDecodeThread(void *args)
{
	BinkPlayback *fmv = static_cast<BinkPlayback*>(args);

	while (Bink_GetCurrentFrameNum(fmv->_handle) < Bink_GetNumFrames(fmv->_handle))
	{
		// check if we should still be playing or not
		if (!fmv->IsPlaying()) {
			break;
		}

		uint32_t startTime = ::timeGetTime();

		// critical section
		pthread_mutex_lock(&fmv->_frameCriticalSection);

		Bink_GetNextFrame(fmv->_handle, fmv->_yuvBuffer);

		// we have a new frame and we're ready to use it
		fmv->_frameReady = true;

		pthread_mutex_unlock(&fmv->_frameCriticalSection);

		uint32_t audioSize = Bink_GetAudioData(fmv->_handle, 0, (int16_t*)fmv->_audioDataBuffer);

		if (audioSize)
		{
			uint32_t freeSpace = fmv->_ringBuffer->GetWritableSize();

//			assert(freeSpace >= audioSize);

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

			fmv->_ringBuffer->WriteData((uint8_t*)&fmv->_audioDataBuffer[0], audioSize);
		}

		uint32_t endTime = ::timeGetTime();
		
		// sleep for frame time minus time to decode frame
		int32_t timeToSleep = (1000 / (int32_t)fmv->_frameRate) - (endTime - startTime);
		if (timeToSleep < 0) {
			timeToSleep = 1;
		}

		::Sleep(timeToSleep);

		if (fmv->_isLooped)
		{
			// handle looping
			if (Bink_GetCurrentFrameNum(fmv->_handle) >= Bink_GetNumFrames(fmv->_handle)) {
				Bink_GotoFrame(fmv->_handle, 0);
			}
		}
	}
	
	/*
	  We might get to this point because we've played all frames - rather than the game code asking us to stop, 
	  or the user interrupting the FMV. Setting _fmvPlaying to false will ensure that the audio decoding thread knows to quit
	 */
	fmv->_fmvPlaying = false;
	return 0;
}

void *BinkAudioThread(void *args)
{
	#ifdef USE_XAUDIO2
		::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	#endif

	BinkPlayback *fmv = static_cast<BinkPlayback*>(args);

	int startTime = 0;
	int endTime = 0;
	int timetoSleep = 0;

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
	}

	return 0;
}
