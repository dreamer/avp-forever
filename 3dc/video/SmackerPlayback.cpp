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
#include <new>

static const int kAudioBufferSize  = 4096;
static const int kAudioBufferCount = 3;
static const int kQuantum = 1000 / 60;

unsigned int __stdcall SmackerDecodeThread(void *args);
unsigned int __stdcall SmackerAudioThread(void *args);

SmackerPlayback::~SmackerPlayback()
{
	mFmvPlaying = false;

	// wait for decode thread to finish
	if (mDecodeThreadHandle)
	{
		WaitForSingleObject(mDecodeThreadHandle, INFINITE);
		CloseHandle(mDecodeThreadHandle);
	}

	delete[] frame;

// TODO	if (mAudio)
	{
		// wait for audio thread to finish
		if (mAudioThreadHandle)
		{
			WaitForSingleObject(mAudioThreadHandle, INFINITE);
			CloseHandle(mAudioThreadHandle);
		}

		delete audioStream;
		delete[] mAudioData;
		delete[] mAudioDataBuffer;
		delete mRingBuffer;
	}

	if (mFrameCriticalSectionInited)
	{
		DeleteCriticalSection(&mFrameCriticalSection);
		mFrameCriticalSectionInited = false;
	}
}

int SmackerPlayback::Open(const std::string &fileName)
{
#ifdef _XBOX
	mFileName = "d:/";
	mFileName += fileName;
#else
	mFileName = fileName;
#endif

	// open the file
	handle = Smacker_Open(mFileName.c_str());
	if (!handle.isValid)
	{
		Con_PrintError("can't open FMV file " + mFileName);
		return FMV_INVALID_FILE;
	}

	Smacker_GetFrameSize(handle, frameWidth, frameHeight);

	frame = new(std::nothrow) uint8_t[frameWidth * frameHeight];
	if (!frame) {
		Con_PrintError("can't allocate frame memory for FMV file " + mFileName);
		return FMV_ERROR;
	}

	uint32_t nAudioTracks = Smacker_GetNumAudioTracks(handle);

	if (nAudioTracks)
	{
		// get audio details (just for the first track)
		SmackerAudioInfo info = Smacker_GetAudioTrackDetails(handle, 0);

		this->sampleRate = info.sampleRate;

		// create mAudio streaming buffer
		this->audioStream = new AudioStream();
		if (!this->audioStream->Init(info.nChannels, info.sampleRate, info.bitsPerSample, kAudioBufferSize, kAudioBufferCount))
		{
			Con_PrintError("Failed to create mAudio stream buffer for FMV playback");
			return FMV_ERROR;
		}

		// we need some temporary mAudio data storage space and a ring buffer instance
		mAudioData = new uint8_t[info.idealBufferSize];
		if (mAudioData == NULL)
		{
			Con_PrintError("Failed to create mAudioData stream buffer for FMV playback");
			return FMV_ERROR;
		}

		mRingBuffer = new RingBuffer(info.idealBufferSize * kAudioBufferCount);
		if (mRingBuffer == NULL)
		{
			Con_PrintError("Failed to create mRingBuffer for FMV playback");
			return FMV_ERROR;
		}

		mAudioDataBuffer = new uint8_t[info.idealBufferSize];
		mAudioDataBufferSize = info.idealBufferSize;
	}

	this->frameRate = Smacker_GetFrameRate(handle);

	mFmvPlaying = true;
	mAudioStarted = false;

	InitializeCriticalSection(&mFrameCriticalSection);
	mFrameCriticalSectionInited = true;

	// now start the threads
	mDecodeThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, SmackerDecodeThread, static_cast<void*>(this), 0, NULL));

	if (nAudioTracks) {
		mAudioThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, SmackerAudioThread, static_cast<void*>(this), 0, NULL));
	}

	return FMV_OK;
}

void SmackerPlayback::Close()
{
	mFmvPlaying = false;
}

bool SmackerPlayback::IsPlaying()
{
	return mFmvPlaying;
}

bool SmackerPlayback::ConvertFrame(uint32_t width, uint32_t height, uint8_t *bufferPtr, uint32_t pitch)
{
	if (!mFmvPlaying) {
		return false;
	}

	if (!bufferPtr)
	{
		mFrameReady = false;
		return true;
	}

	// critical section
	EnterCriticalSection(&mFrameCriticalSection);

	Smacker_GetPalette(handle, palette);
	Smacker_GetFrame(handle, frame);

	uint8_t alpha = 255;

	uint32_t index = 0;

	for (uint32_t y = 0; y < frameHeight; y++)
	{
		uint8_t *destPtr = bufferPtr + y * pitch;

		for (uint32_t x = 0; x < frameWidth; x++)
		{
			*destPtr++ = palette[(frame[index] * 3) + 2];
			*destPtr++ = palette[(frame[index] * 3) + 1];
			*destPtr++ = palette[(frame[index] * 3) + 0];
			*destPtr++ = alpha;

			index++;
		}
	}

	mFrameReady = false;

	LeaveCriticalSection(&mFrameCriticalSection);

	return true;
}

unsigned int __stdcall SmackerDecodeThread(void *args)
{
	SmackerPlayback *fmv = static_cast<SmackerPlayback*>(args);

	uint32_t trackIndex = 0;

	uint32_t nFrames = Smacker_GetNumFrames(fmv->handle);

	while (fmv->currentFrame < nFrames)
	{
		// check if we should still be playing or not
		if (!fmv->IsPlaying()) {
			break;
		}

		uint32_t startTime = timeGetTime();

		// critical section
		EnterCriticalSection(&fmv->mFrameCriticalSection);

		Smacker_GetNextFrame(fmv->handle);

		// we have a new frame and we're ready to use it
		fmv->mFrameReady = true;

		LeaveCriticalSection(&fmv->mFrameCriticalSection);

		uint32_t audioSize = Smacker_GetAudioData(fmv->handle, trackIndex, (int16_t*)fmv->mAudioDataBuffer);

		if (audioSize)
		{
			uint32_t freeSpace = fmv->mRingBuffer->GetWritableSize();

			// if we can't fit all our data..
			if (audioSize > freeSpace)
			{
				while (audioSize > fmv->mRingBuffer->GetWritableSize())
				{
					// little bit of insurance in case we get stuck in here
					if (!fmv->mFmvPlaying) {
						break;
					}

					// wait for the audio buffer to tell us it's just freed up another audio buffer for us to fill
					WaitForSingleObject(fmv->audioStream->voiceContext->hBufferEndEvent, /*INFINITE*/20);
				}
			}

			fmv->mRingBuffer->WriteData(fmv->mAudioDataBuffer, audioSize);
		}

		uint32_t endTime = timeGetTime();

		// sleep for frame time minus time to decode frame
		int32_t timeToSleep = (1000 / fmv->frameRate) - (endTime - startTime);
		if (timeToSleep < 0) {
			timeToSleep = 1;
		}
		
		Sleep(timeToSleep);

		fmv->currentFrame++;
	}

	fmv->mFmvPlaying = false;
	_endthreadex(0);
	return 0;
}

unsigned int __stdcall SmackerAudioThread(void *args)
{
	#ifdef USE_XAUDIO2
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	#endif

	SmackerPlayback *fmv = static_cast<SmackerPlayback*>(args);
	
	int startTime = 0;
	int endTime = 0;
	int timetoSleep = 0;

	while (fmv->mFmvPlaying)
	{
		startTime = timeGetTime();

		uint32_t numBuffersFree = fmv->audioStream->GetNumFreeBuffers();

		while (numBuffersFree)
		{
			uint32_t readableAudio = fmv->mRingBuffer->GetReadableSize();

			// we can fill a buffer
			if (readableAudio >= fmv->audioStream->GetBufferSize())
			{
				fmv->mRingBuffer->ReadData(fmv->mAudioData, fmv->audioStream->GetBufferSize());
				fmv->audioStream->WriteData(fmv->mAudioData, fmv->audioStream->GetBufferSize());

				numBuffersFree--;
			}
			else
			{
				// not enough mAudio available this time
				break;
			}
		}

		if (fmv->mAudioStarted == false)
		{
			fmv->audioStream->Play();
			fmv->mAudioStarted = true;
		}

		endTime = timeGetTime();

		timetoSleep = endTime - startTime;
		timetoSleep -= kQuantum;

		if (timetoSleep < 0 || timetoSleep > kQuantum) {
			timetoSleep = 1;
		}

		Sleep(timetoSleep);
	}

	_endthreadex(0);
	return 0;
}