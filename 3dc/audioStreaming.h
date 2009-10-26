#ifndef _audioStreaming_h_
#define _audioStreaming_h_

extern "C" {

#ifdef USE_XAUDIO2

#include <xaudio2.h>

const static int STREAMBUFFERSIZE = (4096);//32768
const static int STREAMBUFFERCOUNT = 3;

struct StreamingAudioBuffer
{
	int bufferSize;
	int bufferCount;
	int currentBuffer;
	unsigned char *buffers;
	IXAudio2SourceVoice *pSourceVoice;
};

#endif

#ifdef USE_DSOUND

#include <dsound.h>

const static int STREAMBUFFERSIZE = (8192 * 2);//32768
const static int STREAMBUFFERCOUNT = 3;

struct StreamingAudioBuffer
{
	int bufferSize;
	int bufferCount;
	int currentBuffer;
	unsigned char *buffers;
	LPDIRECTSOUNDBUFFER	dsBuffer;
};

#endif

#ifdef _XBOX

#include <xtl.h>

const static int STREAMBUFFERCOUNT = 3;
const static int STREAMBUFFERSIZE = 18432;

struct StreamingAudioBuffer
{
	int bufferSize;
	int bufferCount;
	int currentBuffer;
	unsigned char *buffers;
	DWORD PacketStatus[STREAMBUFFERCOUNT]; // Packet status array
	LPDIRECTSOUNDSTREAM	dsStreamBuffer;
};

#endif

int AudioStream_GetWritableBufferSize(StreamingAudioBuffer *streamStruct);
int AudioStream_GetNumFreeBuffers(StreamingAudioBuffer *streamStruct);
int AudioStream_CreateBuffer(StreamingAudioBuffer *streamStruct, int channels, int rate);
int AudioStream_WriteData(StreamingAudioBuffer *streamStruct, char *audioData, int size);
int AudioStream_SetBufferVolume(StreamingAudioBuffer *streamStruct, int volume);
int AudioStream_ReleaseBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_StopBuffer(StreamingAudioBuffer *streamStruct);
int AudioStream_PlayBuffer(StreamingAudioBuffer *streamStruct);
UINT64 AudioStream_GetNumSamplesPlayed(StreamingAudioBuffer *streamStruct);

};

#endif