#ifndef _audioStreaming_h_
#define _audioStreaming_h_

extern "C" {

#ifdef USE_XAUDIO2

#include <xaudio2.h>

const static int STREAMBUFFERSIZE = (8192 * 2);//32768
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

int GetWritableAudioStreamBufferSize(StreamingAudioBuffer *streamStruct);
int GetNumFreeAudioStreamBuffers(StreamingAudioBuffer *streamStruct);
int CreateAudioStreamBuffer(StreamingAudioBuffer *streamStruct, int channels, int rate);
int WriteAudioStreamData(StreamingAudioBuffer *streamStruct, char *audioData, int size);
int SetAudioStreamBufferVolume(StreamingAudioBuffer *streamStruct, int volume);
int ReleaseAudioStreamBuffer(StreamingAudioBuffer *streamStruct);
int StopAudioStreamBuffer(StreamingAudioBuffer *streamStruct);
int PlayAudioStreamBuffer(StreamingAudioBuffer *streamStruct);

};

#endif