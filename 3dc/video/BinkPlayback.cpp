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
#include <process.h>
#include <assert.h>
#include "utilities.h"

static const int kAudioBufferSize  = 4096;
static const int kAudioBufferCount = 3;
static const int kQuantum = 1000 / 60;

unsigned int __stdcall BinkDecodeThread(void *args);
unsigned int __stdcall BinkAudioThread(void *args);

int BinkPlayback::Open(const std::string &fileName)
{
#ifdef _XBOX
	mFileName = "d:/";
	mFileName += fileName;
#else
	mFileName = fileName;
#endif

	// hack to make the menu background FMV loop
	if (mFileName == "fmvs/menubackground.bik") {
		isLooped = true;
	}

#ifdef _XBOX
	
	// change forwardslashes in path to backslashes
	std::replace(mFileName.begin(), mFileName.end(), '/', '\\');

#endif

	// open the file
	handle = Bink_Open(mFileName.c_str());
	if (!handle.isValid)
	{
		Con_PrintError("can't open FMV file " + mFileName);
		return FMV_INVALID_FILE;
	}

	frameRate = Bink_GetFrameRate(handle);

	// get number of audio tracks available
	nAudioTracks = Bink_GetNumAudioTracks(handle);

	audioTrackInfos.resize(nAudioTracks);

	// init the sound and mVideo api stuff we need
	if (nAudioTracks)
	{
		// get audio information for all available tracks
		for (uint32_t i = 0; i < audioTrackInfos.size(); i++) {
			audioTrackInfos[i] = Bink_GetAudioTrackDetails(handle, i);
		}

		audioTrackIndex = 0;

		uint32_t idealBufferSize = audioTrackInfos[audioTrackIndex].idealBufferSize;

		// create mAudio streaming buffer (just for 1 track for now)
		this->audioStream = new AudioStream();

		if (!this->audioStream->Init(audioTrackInfos[audioTrackIndex].nChannels, audioTrackInfos[audioTrackIndex].sampleRate, 16, kAudioBufferSize, kAudioBufferCount))
		{
			Con_PrintError("Failed to create mAudio stream buffer for FMV playback");
			return FMV_ERROR;
		}

		// we need some temporary mAudio data storage space and a ring buffer instance
		mAudioData = new uint8_t[idealBufferSize];
		if (mAudioData == NULL)
		{
			Con_PrintError("Failed to create mAudioData stream buffer for FMV playback");
			return FMV_ERROR;
		}

		mRingBuffer = new RingBuffer(idealBufferSize * kAudioBufferCount);
		if (mRingBuffer == NULL)
		{
			Con_PrintError("Failed to create mRingBuffer for FMV playback");
			return FMV_ERROR;
		}

		mAudioDataBuffer = new uint8_t[idealBufferSize];
		mAudioDataBufferSize = idealBufferSize;
	}

	Bink_GetFrameSize(handle, mFrameWidth, mFrameHeight);

	// determine how many textures we need
	if (/* can gpu do shader yuv->rgb? */ 1)
	{
		frameTextureIDs.resize(3);
		frameTextureIDs[0] = MISSING_TEXTURE;
		frameTextureIDs[1] = MISSING_TEXTURE;
		frameTextureIDs[2] = MISSING_TEXTURE;
		mNumTextureBits = 8;
	}
	else
	{
		// do CPU YUV->RGB with a single 32bit texture
		frameTextureIDs.resize(1);
		frameTextureIDs[0] = MISSING_TEXTURE;
		mNumTextureBits = 32;
	}

	for (uint32_t i = 0; i < frameTextureIDs.size(); i++)
	{
		uint32_t width  = mFrameWidth;
		uint32_t height = mFrameHeight;

		// the first texture is fullsize, the second two are quartersize
		if (i > 0)
		{
			width  /= 2;
			height /= 2;
		}

		// create the texture with desired parameters
		frameTextureIDs[i] = Tex_Create(/*"CUTSCENE_"*/fileName + Util::IntToString(i), width, height, mNumTextureBits, TextureUsage_Dynamic);
		if (frameTextureIDs[i] == MISSING_TEXTURE)
		{
			Con_PrintError("Unable to create texture(s) for FMV playback");
			return FMV_ERROR;
		}
	}

	mFmvPlaying   = true;
	mAudioStarted = false;
	mFrameReady   = false;
	mTexturesReady = false;

	InitializeCriticalSection(&mFrameCriticalSection);
	mFrameCriticalSectionInited = true;

	// now start the threads
	mDecodeThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, BinkDecodeThread, static_cast<void*>(this), 0, NULL));

	if (nAudioTracks) {
		mAudioThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, BinkAudioThread, static_cast<void*>(this), 0, NULL));
	}

	return FMV_OK;
}

BinkPlayback::~BinkPlayback()
{
	mFmvPlaying = false;

	// wait for decode thread to finish
	if (mDecodeThreadHandle)
	{
		WaitForSingleObject(mDecodeThreadHandle, INFINITE);
		CloseHandle(mDecodeThreadHandle);
	}

	if (nAudioTracks)
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

	for (uint32_t i = 0; i < frameTextureIDs.size(); i++) {
		Tex_Release(frameTextureIDs[i]);
	}

	if (mFrameCriticalSectionInited)
	{
		DeleteCriticalSection(&mFrameCriticalSection);
		mFrameCriticalSectionInited = false;
	}
}

void BinkPlayback::Close()
{
	mFmvPlaying = false;
}

bool BinkPlayback::IsPlaying()
{
	return mFmvPlaying;
}

// copies a decoded Theora YUV frame to texture(s) for GPU to convert via shader
bool BinkPlayback::ConvertFrame()
{
	if (!mFmvPlaying) {
		return false;
	}

	// critical section
	EnterCriticalSection(&mFrameCriticalSection);

	for (uint32_t i = 0; i < frameTextureIDs.size(); i++)
	{
		uint8_t *originalDestPtr = NULL;
		uint32_t pitch = 0;

		uint32_t width  = 0;
		uint32_t height = 0;

		// get width and height
		Tex_GetDimensions(frameTextureIDs[i], width, height);

		// lock the texture
		if (Tex_Lock(frameTextureIDs[i], &originalDestPtr, &pitch, TextureLock_Discard)) // only do below if lock succeeds
		{
			for (uint32_t y = 0; y < height; y++)
			{
				uint8_t *destPtr = originalDestPtr + (y * pitch);
				uint8_t *srcPtr  = yuvBuffer[i].data + (y * yuvBuffer[i].pitch);

				// copy entire width row in one go
				memcpy(destPtr, srcPtr, width * sizeof(uint8_t));

				destPtr += width * sizeof(uint8_t);
				srcPtr  += width * sizeof(uint8_t);
			}

			// unlock texture
			Tex_Unlock(frameTextureIDs[i]);
		}
	}

	// set this value to true so we can now begin to draw the textured fmv frame
	mTexturesReady = true;

	mFrameReady = false;

	LeaveCriticalSection(&mFrameCriticalSection);

	return true;
}

unsigned int __stdcall BinkDecodeThread(void *args)
{
	BinkPlayback *fmv = static_cast<BinkPlayback*>(args);

	while (Bink_GetCurrentFrameNum(fmv->handle) < Bink_GetNumFrames(fmv->handle))
	{
		// check if we should still be playing or not
		if (!fmv->IsPlaying()) {
			break;
		}

		uint32_t startTime = timeGetTime();

		// critical section
		EnterCriticalSection(&fmv->mFrameCriticalSection);

		Bink_GetNextFrame(fmv->handle, fmv->yuvBuffer);

		// we have a new frame and we're ready to use it
		fmv->mFrameReady = true;

		LeaveCriticalSection(&fmv->mFrameCriticalSection);

		uint32_t audioSize = Bink_GetAudioData(fmv->handle, 0, (int16_t*)fmv->mAudioDataBuffer);

		if (audioSize)
		{
			uint32_t freeSpace = fmv->mRingBuffer->GetWritableSize();

			assert(freeSpace >= audioSize);

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

			fmv->mRingBuffer->WriteData((uint8_t*)&fmv->mAudioDataBuffer[0], audioSize);
		}

		uint32_t endTime = timeGetTime();

		// video
//		double audio_time = static_cast<double>(fmv->audioStream->GetNumSamplesPlayed()) / static_cast<double>(fmv->audioTrackInfos[fmv->audioTrackIndex].sampleRate);
//		double video_time = (1.0f / fmv->frameRate) * (float)fmv->currentFrame;
		
		// sleep for frame time minus time to decode frame
		int32_t timeToSleep = (1000 / fmv->frameRate) - (endTime - startTime);
		if (timeToSleep < 0) {
			timeToSleep = 1;
		}

		Sleep(timeToSleep);

		if (fmv->isLooped)
		{
			// handle looping
			if (Bink_GetCurrentFrameNum(fmv->handle) >= Bink_GetNumFrames(fmv->handle)) {
				Bink_GotoFrame(fmv->handle, 0);
			}
		}
	}
	
	fmv->mFmvPlaying = false;
	_endthreadex(0);
	return 0;
}

unsigned int __stdcall BinkAudioThread(void *args)
{
	#ifdef USE_XAUDIO2
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	#endif

	BinkPlayback *fmv = static_cast<BinkPlayback*>(args);

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