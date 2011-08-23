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
		uint32_t	bufferSize;
		uint32_t	bufferCount;
		uint32_t	currentBuffer;
		uint32_t	numChannels;
		uint32_t	rate;
		uint32_t	bytesPerSample;
		uint64_t	totalBytesPlayed;
		uint64_t	totalSamplesWritten;
		bool		isPaused;
		uint8_t		*buffers;
		IXAudio2SourceVoice *pSourceVoice;

	public:
		AudioStream(uint32_t channels, uint32_t rate, uint32_t bufferSize, uint32_t numBuffers);
		int32_t  Stop();
		int32_t  Play();
		int32_t  SetVolume(uint32_t volume);
		int32_t  SetPan(uint32_t pan);
		uint32_t WriteData(uint8_t *audioData, uint32_t size);
		uint32_t GetNumFreeBuffers();
		uint64_t GetNumSamplesPlayed();
		uint64_t GetNumSamplesWritten();
		uint32_t GetWritableBufferSize();
		uint32_t GetBytesWritten();
		uint32_t GetBufferSize();
		StreamingVoiceContext *voiceContext;

		~AudioStream();
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
		uint32_t	bufferSize;
		uint32_t	bufferCount;
		uint32_t	currentBuffer;
		uint32_t	numChannels;
		uint32_t	rate;
		uint32_t	bytesPerSample;
		uint64_t	totalSamplesWritten;
		bool		isPaused;
		uint8_t		*buffers;
		std::vector<DWORD> PacketStatus;
		LPDIRECTSOUNDSTREAM dsStreamBuffer;

	public:
		AudioStream(uint32_t channels, uint32_t rate, uint32_t bufferSize, uint32_t numBuffers);
		int32_t  Stop();
		int32_t  Play();
		int32_t  SetVolume(uint32_t volume);
		int32_t  SetPan(uint32_t pan);
		uint32_t WriteData(uint8_t *audioData, uint32_t size);
		uint32_t GetNumFreeBuffers();
		uint64_t GetNumSamplesPlayed();
		uint64_t GetNumSamplesWritten();
		uint32_t GetWritableBufferSize();
		uint32_t GetBufferSize();
		StreamingVoiceContext *voiceContext;
		uint64_t totalBytesPlayed;

		~AudioStream();
};

#endif

#endif
