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

#ifndef _SmackerPlayback_h_
#define _SmackerPlayback_h_

#include "SmackerDecoder.h"
#include "RingBuffer.h"
#include "AudioStreaming.h"
#include "TextureManager.h"

class SmackerPlayback
{
	private:
		std::string mFileName;

		// thread handles
		HANDLE mDecodeThreadHandle;
		HANDLE mAudioThreadHandle;

		bool mFrameCriticalSectionInited;
		bool isLooped;

		uint8_t palette[768];
		uint8_t *frame;

		uint32_t frameWidth;
		uint32_t frameHeight;

	public:

		SmackerHandle handle;

		CRITICAL_SECTION mFrameCriticalSection;

		AudioStream		*audioStream;
		RingBuffer		*mRingBuffer;
		uint8_t			*mAudioData;
		uint8_t			*mAudioDataBuffer;
		uint32_t		mAudioDataBufferSize;

		uint32_t		sampleRate;

		uint32_t currentFrame;
		float frameRate;
		volatile bool mFmvPlaying;
		bool mAudioStarted;
		volatile bool mFrameReady;

		int	Open(const std::string &fileName);
		void Close();
		bool IsPlaying();
		bool ConvertFrame(uint32_t width, uint32_t height, uint8_t *bufferPtr, uint32_t pitch);

		~SmackerPlayback();
		SmackerPlayback() :
			mFmvPlaying(false),
			mAudioStarted(false),
			mFrameReady(false),
			isLooped(false),
			mFrameCriticalSectionInited(false),
			mDecodeThreadHandle(0),
			mAudioThreadHandle(0),
			audioStream(0),
			mRingBuffer(0),
			mAudioData(0),
			frameRate(0),
			mAudioDataBuffer(0),
			currentFrame(0),
			mAudioDataBufferSize(0),
			sampleRate(0),
			frame(0)
			{}
};

#endif
