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
#include <pthread.h>

class SmackerPlayback
{
	private:
		std::string _fileName;

		// thread handles
		pthread_t _decodeThreadHandle;
		pthread_t _audioThreadHandle;
		bool _decodeThreadInited;
		bool _audioThreadInited;

		bool _frameCriticalSectionInited;
		bool _isLooped;

		uint8_t _palette[768];
		uint8_t *_frame;

		uint32_t _frameWidth;
		uint32_t _frameHeight;

	public:

		SmackerHandle _handle;

		pthread_mutex_t _frameCriticalSection;

		uint32_t		_nAudioTracks;
		AudioStream		*_audioStream;
		RingBuffer		*_ringBuffer;
		uint8_t			*_audioData;
		uint8_t			*_audioDataBuffer;
		uint32_t		_audioDataBufferSize;
		uint32_t		_sampleRate;
		uint32_t		_currentFrame;
		float			_frameRate;
		volatile bool	_fmvPlaying;
		bool			_audioStarted;
		volatile bool	_frameReady;

		int	Open(const std::string &fileName);
		void Close();
		bool IsPlaying();
		bool ConvertFrame(uint32_t width, uint32_t height, uint8_t *bufferPtr, uint32_t pitch);

		~SmackerPlayback();
		SmackerPlayback() :
			_decodeThreadInited(false),
			_audioThreadInited(false),
			_frameCriticalSectionInited(false),
			_fmvPlaying(false),
			_audioStarted(false),
			_frameReady(false),
			_isLooped(false),
			_nAudioTracks(0),
			_audioStream(0),
			_ringBuffer(0),
			_audioData(0),
			_frameRate(0),
			_audioDataBuffer(0),
			_currentFrame(0),
			_audioDataBufferSize(0),
			_sampleRate(0),
			_frame(0)
			{}
};

#endif
