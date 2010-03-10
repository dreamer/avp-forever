#ifndef _audioStreaming_h_
#define _audioStreaming_h_

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
	STDMETHOD_( void, OnVoiceProcessingPassStart )( UINT32 )
	{
	}
	STDMETHOD_( void, OnVoiceProcessingPassEnd )()
	{
	}
	STDMETHOD_( void, OnStreamEnd )()
	{
	}
	STDMETHOD_( void, OnBufferStart )( void* )
	{
	}
	STDMETHOD_( void, OnBufferEnd )( void* )
	{
		SetEvent( hBufferEndEvent );
	}
	STDMETHOD_( void, OnLoopEnd )( void* )
	{
	}
	STDMETHOD_( void, OnVoiceError )( void*, HRESULT )
	{
	}

    HANDLE hBufferEndEvent;

            StreamingVoiceContext() : hBufferEndEvent( CreateEvent( NULL, FALSE, FALSE, NULL ) )
            {
            }
    virtual ~StreamingVoiceContext()
    {
        CloseHandle( hBufferEndEvent );
    }
};

struct StreamingAudioBuffer
{
	int bufferSize;
	int bufferCount;
	int currentBuffer;
	int numChannels;
	int rate;
	int bytesPerSample;
	UINT64 totalBytesPlayed;
	UINT64 totalSamplesWritten;
	bool isPaused;
	uint8_t *buffers;
	IXAudio2SourceVoice *pSourceVoice;
	StreamingVoiceContext *voiceContext;
};

#endif

#ifdef USE_DSOUND

#include <dsound.h>

struct StreamingAudioBuffer
{
	int bufferSize;
	int bufferCount;
	int currentBuffer;
	int numChannels;
	int rate;
	int bytesPerSample;
	int writeOffset;
	int lastPlayCursor;
	int msPerBuffer;
	int lastCheckedTime;
	UINT64 totalBytesPlayed;
	UINT64 totalSamplesWritten;
	uint8_t *buffers;
	LPDIRECTSOUNDBUFFER	dsBuffer;
};

#endif

#ifdef _XBOX

#include <xtl.h>
#include <vector>

struct StreamingAudioBuffer
{
	int bufferSize;
	int bufferCount;
	int currentBuffer;
	int numChannels;
	int rate;
	int bytesPerSample;
	uint8_t *buffers;
	UINT64 totalBytesPlayed;
	UINT64 totalSamplesWritten;
	bool isPaused;
	std::vector<DWORD> PacketStatus;
	LPDIRECTSOUNDSTREAM	dsStreamBuffer;
};

#endif

StreamingAudioBuffer * AudioStream_CreateBuffer(int channels, int rate, int bufferSize, int numBuffers);
int AudioStream_PlayBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_StopBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_ReleaseBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_SetBufferVolume(StreamingAudioBuffer *streamStruct, int volume);
int AudioStream_WriteData(StreamingAudioBuffer *streamStruct, uint8_t *audioData, int size);
int AudioStream_GetWritableBufferSize(StreamingAudioBuffer *streamStruct);
int AudioStream_GetNumFreeBuffers(StreamingAudioBuffer *streamStruct);
int AudioStream_SetPan(StreamingAudioBuffer *streamStruct, int pan);
UINT64 AudioStream_GetNumSamplesPlayed(StreamingAudioBuffer *streamStruct);
UINT64 AudioStream_GetNumSamplesWritten(StreamingAudioBuffer *streamStruct);

#endif