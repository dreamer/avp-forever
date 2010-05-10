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
	uint32_t GetBufferSize();
	StreamingVoiceContext *voiceContext;

	~AudioStream();
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

struct StreamingVoiceContext
{
	HANDLE hBufferEndEvent;

	void TriggerEvent()
	{
		SetEvent( hBufferEndEvent );
	}

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
	uint8_t *buffers;
	UINT64 totalBytesPlayed;
	UINT64 totalSamplesWritten;
	bool isPaused;
	std::vector<DWORD> PacketStatus;
	LPDIRECTSOUNDSTREAM	dsStreamBuffer;
	StreamingVoiceContext *voiceContext;
};

#endif

/*
StreamingAudioBuffer * AudioStream_CreateBuffer(uint32_t channels, uint32_t rate, uint32_t bufferSize, uint32_t numBuffers);
int AudioStream_PlayBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_StopBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_ReleaseBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_SetBufferVolume(StreamingAudioBuffer *streamStruct, uint32_t);
int AudioStream_WriteData(StreamingAudioBuffer *streamStruct, uint8_t *audioData, uint32_t size);
int AudioStream_GetWritableBufferSize(StreamingAudioBuffer *streamStruct);
int AudioStream_GetNumFreeBuffers(StreamingAudioBuffer *streamStruct);
int AudioStream_SetPan(StreamingAudioBuffer *streamStruct, uint32_t pan);
UINT64 AudioStream_GetNumSamplesPlayed(StreamingAudioBuffer *streamStruct);
UINT64 AudioStream_GetNumSamplesWritten(StreamingAudioBuffer *streamStruct);
*/

#endif