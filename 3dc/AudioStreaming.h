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

#ifndef _AudioStreaming_h_
#define _AudioStreaming_h_

#include <stdint.h>

enum
{
	AUDIOSTREAM_OK,
	AUDIOSTREAM_ERROR,
	AUDIOSTREAM_OUTOFMEMORY
};

#ifdef USE_XAUDIO2

#include <xaudio2.h>

struct StreamingVoiceContext : public IXAudio2VoiceCallback
{
	STDMETHOD_(void, OnVoiceProcessingPassStart)(UINT32)
	{
	}
	STDMETHOD_(void, OnVoiceProcessingPassEnd)()
	{
	}
	STDMETHOD_(void, OnStreamEnd)()
	{
	}
	STDMETHOD_(void, OnBufferStart)(void*)
	{
	}
	STDMETHOD_(void, OnBufferEnd)(void*)
	{
		SetEvent(hBufferEndEvent);
	}
	STDMETHOD_(void, OnLoopEnd)(void*)
	{
	}
	STDMETHOD_(void, OnVoiceError)(void*, HRESULT)
	{
	}

	HANDLE hBufferEndEvent;

	StreamingVoiceContext() : hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
	{
	}

	virtual ~StreamingVoiceContext()
	{
		CloseHandle(hBufferEndEvent);
	}
};

class AudioStream
{
	private:
		uint32_t	_bufferSize;
		uint32_t	_bufferCount;
		uint32_t	_currentBuffer;
		uint32_t	_nChannels;
		uint32_t	_rate;
		uint32_t	_bytesPerSample;
		uint64_t	_totalBytesPlayed;
		uint64_t	_totalSamplesWritten;
		bool		_isPaused;
		uint32_t    _volume;
		uint8_t		*_buffers;
		IXAudio2SourceVoice *_pSourceVoice;

	public:
		AudioStream();
		bool Init(uint32_t nChannels, uint32_t rate, uint32_t bitsPerSample, uint32_t bufferSize, uint32_t nBuffers);
		int32_t  Stop();
		int32_t  Play();
		int32_t  SetVolume(uint32_t volume);
		uint32_t GetVolume();
		int32_t  SetPan(uint32_t pan);
		uint32_t WriteData(uint8_t *audioData, uint32_t size);
		uint32_t GetNumFreeBuffers();
		uint64_t GetNumSamplesPlayed();
		uint64_t GetNumSamplesWritten();
		uint32_t GetWritableBufferSize();
		uint32_t GetBytesWritten();
		uint32_t GetBufferSize();
		void WaitForFreeBuffer();

		StreamingVoiceContext *_voiceContext;

		~AudioStream();
};

#endif

#ifdef USE_OPENAL

#include <AL\al.h>
#include <pthread.h>
#include <deque>

struct sndSource;

void *CheckProcessedBuffers(void *argument);

class AudioStream
{
	private:
		uint32_t	_currentBuffer;
		uint32_t	_nChannels;
		uint32_t	_rate;
		uint32_t	_bytesPerSample;
		uint64_t	_totalSamplesWritten;
		ALuint		*_buffers;
		ALenum		_format;
		uint8_t		*_dataBuffers;
		uint32_t	_volume;

	public:
		AudioStream();
		~AudioStream();
		bool Init(uint32_t nChannels, uint32_t rate, uint32_t bitsPerSample, uint32_t bufferSize, uint32_t nBuffers);
		int32_t  Stop();
		int32_t  Play();
		int32_t  SetVolume(uint32_t volume);
		uint32_t GetVolume();
		int32_t  SetPan(uint32_t pan);
		uint32_t WriteData(uint8_t *audioData, uint32_t size);
		uint32_t GetNumFreeBuffers();
		uint64_t GetNumSamplesPlayed();
		uint64_t GetNumSamplesWritten();
		uint32_t GetWritableBufferSize();
		uint32_t GetBytesWritten();
		uint32_t GetBufferSize();
		void WaitForFreeBuffer();

		uint32_t	_bufferCount;
		sndSource	*_source;
		pthread_t	_playbackThread;
		uint64_t	_totalBytesPlayed;
		uint32_t	_bufferSize;
		pthread_mutex_t _bufferMutex;

		bool _isPaused;
		bool _isPlaying;

		std::deque<ALuint> _freeBufferQueue;
};

#endif

#ifdef _XBOX

#include <xtl.h>
#include <vector>

struct StreamingVoiceContext
{
	HANDLE hBufferEndEvent;

	void TriggerEvent()
	{
		SetEvent(hBufferEndEvent);
	}

	StreamingVoiceContext() : hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
	{
	}
	virtual ~StreamingVoiceContext()
	{
		CloseHandle(hBufferEndEvent);
	}
};

class AudioStream
{
	private:
		uint32_t	_bufferSize;
		uint32_t	_bufferCount;
		uint32_t	_currentBuffer;
		uint32_t	_nChannels;
		uint32_t	_rate;
		uint32_t	_bytesPerSample;
		uint64_t	_totalSamplesWritten;
		bool		_isPaused;
		uint32_t    _volume;
		uint8_t		*_buffers;
		std::vector<DWORD> _packetStatus;
		LPDIRECTSOUNDSTREAM _dsStreamBuffer;

	public:
		AudioStream();
		bool Init(uint32_t channels, uint32_t rate, uint32_t bitsPerSample, uint32_t bufferSize, uint32_t nBuffers);
		int32_t  Stop();
		int32_t  Play();
		int32_t  SetVolume(uint32_t volume);
		uint32_t GetVolume();
		int32_t  SetPan(uint32_t pan);
		uint32_t WriteData(uint8_t *audioData, uint32_t size);
		uint32_t GetNumFreeBuffers();
		uint64_t GetNumSamplesPlayed();
		uint64_t GetNumSamplesWritten();
		uint32_t GetWritableBufferSize();
		uint32_t GetBufferSize();
		void WaitForFreeBuffer();

		StreamingVoiceContext *voiceContext;
		uint64_t totalBytesPlayed;

		~AudioStream();
};

#endif

#endif
