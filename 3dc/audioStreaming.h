#ifndef _audioStreaming_h_
#define _audioStreaming_h_

//extern "C" {

#ifdef USE_XAUDIO2

#include <xaudio2.h>

struct StreamingAudioBuffer
{
	int bufferSize;
	int bufferCount;
	int currentBuffer;
	int numChannels;
	int rate;
	int bitsPerSample;
	UINT64 totalSamplesWritten;
	unsigned char *buffers;
	IXAudio2SourceVoice *pSourceVoice;
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
	UINT64 totalBytesPlayed;
	UINT64 totalSamplesWritten;
	unsigned char *buffers;
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
	unsigned char *buffers;
	std::vector<DWORD> PacketStatus;
	LPDIRECTSOUNDSTREAM	dsStreamBuffer;
};

#endif

int AudioStream_CreateBuffer(StreamingAudioBuffer *streamStruct, int channels, int rate, int bufferSize, int numBuffers);
int AudioStream_PlayBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_StopBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_ReleaseBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_SetBufferVolume(StreamingAudioBuffer *streamStruct, int volume);
int AudioStream_WriteData(StreamingAudioBuffer *streamStruct, char *audioData, int size);
int AudioStream_GetWritableBufferSize(StreamingAudioBuffer *streamStruct);
int AudioStream_GetNumFreeBuffers(StreamingAudioBuffer *streamStruct);
UINT64 AudioStream_GetNumSamplesPlayed(StreamingAudioBuffer *streamStruct);
UINT64 AudioStream_GetNumSamplesWritten(StreamingAudioBuffer *streamStruct);

//};

#endif