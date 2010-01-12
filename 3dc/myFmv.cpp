#include "console.h"
#include "d3_func.h"
#include "stdint.h"
#include "assert.h"
#include <process.h>
#include "ringbuffer.h"
#include "audioStreaming.h"
#include <fstream>
#include <map>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <vorbis/vorbisfile.h>
#include <math.h>

unsigned int __stdcall decodeThread(void *args);
unsigned int __stdcall audioThread(void *args);

enum fmvErrors
{
	FMV_OK,
	FMV_INVALID_FILE,
	FMV_ERROR
};

enum streamType
{
	TYPE_VORBIS,
	TYPE_THEORA,
	TYPE_UNKNOWN
};

// forward declare struct
struct oggStream;

typedef std::map<int, oggStream*> streamMap;

struct fmv
{
	std::string 	fileName;
	std::ifstream 	fileStream;

	ogg_sync_state 	state;
	ogg_int64_t		granulePos;
	th_ycbcr_buffer yuvBuffer;

	streamMap		streams;

	// audio
	StreamingAudioBuffer *audioStream;
	RingBuffer *ringBuffer;
	uint8_t	*audioData;
	uint16_t *audioDataBuffer;
	int	audioDataBufferSize;
	CRITICAL_SECTION audioCriticalSection;

	// video
	D3DTEXTURE displayTexture;
	int textureWidth;
	int textureHeight;
	int frameWidth;
	int frameHeight;
	CRITICAL_SECTION frameCriticalSection;

	// thread handles
	HANDLE decodeThreadHandle;
	HANDLE audioThreadHandle;

	// so we can easily reference these from the threads.
	oggStream *video;
	oggStream *audio;

	bool fmvPlaying;
	bool frameReady;
	bool audioStarted;

	int open(const char* file);
	bool readPage(ogg_page *page);
	bool readPacket(oggStream *stream, ogg_packet *packet);
	void readHeaders();
	void handleTheoraData(oggStream *stream, ogg_packet *packet);
};

struct theoraDecode
{
	th_info 		info;
	th_comment 		comment;
	th_setup_info 	*setupInfo;
	th_dec_ctx		*decodeContext;

	void init();
};

void theoraDecode::init()
{
	decodeContext = th_decode_alloc(&info, setupInfo);
	assert(decodeContext != NULL);

	int ppmax = 0;
	int ret = th_decode_ctl(decodeContext, TH_DECCTL_GET_PPLEVEL_MAX, &ppmax, sizeof(ppmax));
	assert (ret == 0);

	ppmax = 0;

	ret = th_decode_ctl(decodeContext, TH_DECCTL_SET_PPLEVEL, &ppmax, sizeof(ppmax));
	assert(ret == 0);
}

struct vorbisDecode
{
	vorbis_info 		info;
	vorbis_comment 		comment;
	vorbis_dsp_state 	dspState;
	vorbis_block 		block;

	void init();
};

void vorbisDecode::init()
{
	int ret = vorbis_synthesis_init(&dspState, &info);
	assert(ret == 0);

	ret = vorbis_block_init(&dspState, &block);
	assert(ret == 0);
}

struct oggStream
{
	int 				serialNumber;
	ogg_stream_state	streamState;
	bool				headersRead;
	bool				active;
	streamType			type;
	theoraDecode		theora;
	vorbisDecode		vorbis;

	void streamInit(int serialNumber);
};

void oggStream::streamInit(int serialNo)
{
	serialNumber = serialNo;
	type = TYPE_UNKNOWN;
	headersRead = false;
	active = true;

	// init theora stuff
	th_info_init(&theora.info);
	th_comment_init(&theora.comment);
	theora.decodeContext = 0;
	theora.setupInfo = 0;

	// init vorbis stuff
	vorbis_info_init(&vorbis.info);
	vorbis_comment_init(&vorbis.comment);
}

fmv *newFmv;

void fmvTest(const char* file)
{
	newFmv = new fmv;

	newFmv->open(file);

//	delete newFmv;
}

int fmv::open(const char* file)
{
	fileName = file;

	// open the file
	fileStream.open(fileName.c_str(), std::ios::in | std::ios::binary);

	if (!fileStream.is_open())
	{
		Con_PrintError("can't find FMV file " + fileName);
		return FMV_INVALID_FILE;
	}

	if (ogg_sync_init(&state) != 0)
	{
		Con_PrintError("ogg_sync_init failed");
		return FMV_ERROR;
	}

	// now read the headers in the ogg file
	readHeaders();

//	oggStream *video = 0;
//	oggStream *audio = 0;
	video = 0;
	audio = 0;
	displayTexture = 0;
	granulePos = 0;

	// initialise the streams we found while reading the headers
	for (streamMap::iterator it = streams.begin(); it != streams.end(); ++it)
	{
		oggStream *stream = (*it).second;

		if (!video && stream->type == TYPE_THEORA)
		{
			video = stream;
			video->theora.init();
		}

		if (!audio && stream->type == TYPE_VORBIS)
		{
			audio = stream;
			audio->vorbis.init();
		}
		else stream->active = false;
	}

	// init the sound and video api stuff we need
	if (audio)
	{
		// create audio streaming buffer
		audioStream = AudioStream_CreateBuffer(audio->vorbis.info.channels, audio->vorbis.info.rate, 4096, 3);
		if (audioStream == NULL)
		{
			Con_PrintError("Failed to create audio stream buffer for fmv playback");
			return FMV_ERROR;
		}

		// we need some temporary audio data storage space and a ring buffer instance
		audioData = new uint8_t[audioStream->bufferSize];
		ringBuffer = new RingBuffer(audioStream->bufferSize * audioStream->bufferCount);

		audioDataBuffer = NULL;
	}

	if (!displayTexture)
	{
		frameWidth = video->theora.info.frame_width;
		frameHeight = video->theora.info.frame_height;

		textureWidth = frameWidth;
		textureHeight = textureHeight;

		// create a new texture, passing width and height which will be set to the actual size of the created texture
		displayTexture = CreateFmvTexture(&textureWidth, &textureHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
		if (!displayTexture)
		{
			Con_PrintError("can't create FMV texture");
			return FMV_ERROR;
		}
	}

	fmvPlaying = true;
	audioStarted = false;
	frameReady = false;

	InitializeCriticalSection(&frameCriticalSection);
	InitializeCriticalSection(&audioCriticalSection);

	// time to start our threads
	decodeThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, decodeThread, static_cast<void*>(this), 0, NULL));

	if (audio)
	{
		audioThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, audioThread, static_cast<void*>(this), 0, NULL));
	}

	Con_PrintMessage("fmv should be playing now");

	return FMV_OK;
}

bool fmv::readPage(ogg_page *page)
{
	int amountRead = 0;
	int ret = 0;

	// if end of file, keep processing any remaining buffered pages
	if (!fileStream.good())
		return ogg_sync_pageout(&state, page) == 1;

	while (ogg_sync_pageout(&state, page) != 1)
	{
		char *buffer = ogg_sync_buffer(&state, 4096);
		assert(buffer);

		fileStream.read(buffer, 4096);
		amountRead = fileStream.gcount();

		if (amountRead == 0)
		{
			// eof
			continue;
		}

		ret = ogg_sync_wrote(&state, amountRead);
		assert(ret == 0);
	}

	return true;
}

bool fmv::readPacket(oggStream *stream, ogg_packet *packet)
{
	int ret = 0;

	while ((ret = ogg_stream_packetout(&stream->streamState, packet)) != 1)
	{
		OutputDebugString("in readpacket\n");
		ogg_page page;

		if (!readPage(&page))
			return false;

		int serialNumber = ogg_page_serialno(&page);
		assert(streams.find(serialNumber) != streams.end());
		oggStream *pageStream = streams[serialNumber];

		// Drop data for streams we're not interested in.
		if (stream->active)
		{
			ret = ogg_stream_pagein(&pageStream->streamState, &page);
			assert(ret == 0);
    	}
	}
	OutputDebugString("leaving readpacket\n");
	return true;
}

void fmv::readHeaders()
{
	ogg_page page;
	int ret = 0;

	bool gotAllHeaders = false;
	bool gotTheora = false;
	bool gotVorbis = false;

	while (!gotAllHeaders && readPage(&page))
	{
		oggStream *stream = 0;

		int serialNumber = ogg_page_serialno(&page);

		if (ogg_page_bos(&page))
		{
			stream = new oggStream;
			stream->streamInit(serialNumber);
			ret = ogg_stream_init(&stream->streamState, serialNumber);
			assert(ret == 0);

			// store in the stream std::map
			streams[serialNumber] = stream;
		}

		assert(streams.find(serialNumber) != streams.end());

		if (streams.find(serialNumber) != streams.end())
		{
			// change our stream pointer to the one now in the map
//			stream = streams[serialNumber];
		}

		stream = streams[serialNumber];

		// add the completed page to the bitstream
		ret = ogg_stream_pagein(&stream->streamState, &page);
		assert(ret == 0);

		ogg_packet packet;

		while (!gotAllHeaders && (ret = ogg_stream_packetpeek(&stream->streamState, &packet)) != 0)
		{
			assert(ret == 1);

			// we have a packet, check if its a header packet

			// check for a theora header first
			if (!gotTheora)
			{
				ret = th_decode_headerin(&stream->theora.info, &stream->theora.comment, &stream->theora.setupInfo, &packet);
				if (ret > 0)
				{
					// we found the header packet
					stream->type = TYPE_THEORA;
				}
				else if (stream->type == TYPE_THEORA && ret == 0)
				{
					gotTheora = true;
				}
			}

			// check for a vorbis header
			if (!gotVorbis)
			{
				ret = vorbis_synthesis_headerin(&stream->vorbis.info, &stream->vorbis.comment, &packet);
				if (ret == 0)
				{
					stream->type = TYPE_VORBIS;
				}
				else if (stream->type == TYPE_VORBIS && ret == OV_ENOTVORBIS)
				{
					gotVorbis = true;
				}
			}

			if (gotVorbis && gotTheora)
				gotAllHeaders = true;

			if (!gotAllHeaders)
			{
				// we need to move forward and grab the next packet
				ret = ogg_stream_packetout(&stream->streamState, &packet);
				assert(ret == 1);
			}
		}
	}
}

void fmv::handleTheoraData(oggStream *stream, ogg_packet *packet)
{
	int ret = th_decode_packetin(stream->theora.decodeContext, packet, &granulePos);

	if (ret == TH_DUPFRAME)  // same as previous frame, don't bother decoding it
		return;

	assert (ret == 0);

	// critical section

	// We have a frame. Get the YUV data
	ret = th_decode_ycbcr_out(stream->theora.decodeContext, yuvBuffer);
	assert (ret == 0);

	OutputDebugString("got a frame\n");

	// we have a new frame and we're ready to use it
//	frameReady = true;

	// leave critical section
}

unsigned int __stdcall decodeThread(void *args)
{
	fmv *fmvInstance = static_cast<fmv*>(args);

	ogg_packet packet;
	int ret = 0;
    float** pcm = 0;
    int samples = 0;

	while (fmvInstance->readPacket(fmvInstance->audio, &packet))
	{
		if (vorbis_synthesis(&fmvInstance->audio->vorbis.block, &packet) == 0)
		{
			ret = vorbis_synthesis_blockin(&fmvInstance->audio->vorbis.dspState, &fmvInstance->audio->vorbis.block);
			assert(ret == 0);
		}

		// get pointer to array of floating point values for sound samples
		while ((samples = vorbis_synthesis_pcmout(&fmvInstance->audio->vorbis.dspState, &pcm)) > 0)
		{
			if (samples > 0)
			{
				int dataSize = samples * fmvInstance->audio->vorbis.info.channels;

				if (fmvInstance->audioDataBuffer == NULL)
				{
					// make it twice the size we currently need to avoid future reallocations
					fmvInstance->audioDataBuffer = new uint16_t[dataSize * 2];
					fmvInstance->audioDataBufferSize = dataSize * 2;
				}

				if (fmvInstance->audioDataBufferSize < dataSize)
				{
					if (fmvInstance->audioDataBuffer != NULL)
					{
						delete []fmvInstance->audioDataBuffer;
						OutputDebugString("deleted audioDataBuffer to resize\n");
					}

					fmvInstance->audioDataBuffer = new uint16_t[dataSize * 2];
					OutputDebugString("alloc audioDataBuffer\n");

					if (!fmvInstance->audioDataBuffer)
					{
						OutputDebugString("Out of memory\n");
						break;
					}
					fmvInstance->audioDataBufferSize = dataSize * 2;
				}

				uint16_t* p = fmvInstance->audioDataBuffer;

				for (int i = 0; i < samples; ++i)
				{
					for (int j = 0; j < fmvInstance->audio->vorbis.info.channels; ++j)
					{
						int v = static_cast<int>(floorf(0.5f + pcm[j][i]*32767.0f));
						if (v > 32767) v = 32767;
						if (v < -32768) v = -32768;
						*p++ = v;
					}
				}

				int audioSize = samples * fmvInstance->audio->vorbis.info.channels * sizeof(uint16_t);

				// move the critical section stuff into the ring buffer code itself?
				EnterCriticalSection(&fmvInstance->audioCriticalSection);

				int freeSpace = fmvInstance->ringBuffer->GetWritableSize();

				// if we can't fit all our data..
				if (audioSize > freeSpace)
				{
					//while (audioSize > RingBuffer_GetWritableSpace())
					while (audioSize > fmvInstance->ringBuffer->GetWritableSize())
					{
						// little bit of insurance in case we get stuck in here
						//if (!fmvInstance->fmvPlaying)
						//	break;

						Sleep(16);
					}
				}

				fmvInstance->ringBuffer->WriteData((uint8_t*)&fmvInstance->audioDataBuffer[0], audioSize);

				LeaveCriticalSection(&fmvInstance->audioCriticalSection);
			}

			// tell vorbis how many samples we consumed
			ret = vorbis_synthesis_read(&fmvInstance->audio->vorbis.dspState, samples);
			assert(ret == 0);

			if (fmvInstance->video)
			{
				// video stuff
				float audio_time = static_cast<float>(AudioStream_GetNumSamplesPlayed(fmvInstance->audioStream)) / static_cast<float>(fmvInstance->audio->vorbis.info.rate);
				float video_time = static_cast<float>(th_granule_time(fmvInstance->video->theora.decodeContext, fmvInstance->granulePos));

				char buf[100];
				sprintf(buf, "audio_time: %f video_time: %f\n", audio_time, video_time);
				OutputDebugString(buf);

				// if audio is ahead of frame time, display a new frame
				if ((audio_time > video_time))
				{
					// Decode one frame and display it. If no frame is available we
					// don't do anything.
					ogg_packet packet;
					if (fmvInstance->readPacket(fmvInstance->video, &packet))
					{
						fmvInstance->handleTheoraData(fmvInstance->video, &packet);
						video_time = th_granule_time(fmvInstance->video->theora.decodeContext, fmvInstance->granulePos);
					}
				}
			}
		}
	}

	_endthreadex(0);
	return 0;
}

unsigned int __stdcall audioThread(void *args)
{
	#ifdef USE_XAUDIO2
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	#endif

	fmv *fmvInstance = static_cast<fmv*>(args);

	DWORD dwQuantum = 1000 / 60;

	static int totalRead = 0;
	static int lastRead = 0;

	int startTime = 0;
	int endTime = 0;
	int timetoSleep = 0;

	while (fmvInstance->fmvPlaying)
	{
		startTime = timeGetTime();

		int numBuffersFree = AudioStream_GetNumFreeBuffers(fmvInstance->audioStream);

		while (numBuffersFree)
		{
			int readableAudio = fmvInstance->ringBuffer->GetReadableSize();

			// we can fill a buffer
			if (readableAudio >= fmvInstance->audioStream->bufferSize)
			{
				fmvInstance->ringBuffer->ReadData(fmvInstance->audioData, fmvInstance->audioStream->bufferSize);
				AudioStream_WriteData(fmvInstance->audioStream, fmvInstance->audioData, fmvInstance->audioStream->bufferSize);

				numBuffersFree--;
			}
			else
			{
				// not enough audio available this time
				break;
			}
		}

		if (fmvInstance->audioStarted == false)
		{
			AudioStream_PlayBuffer(fmvInstance->audioStream);
			fmvInstance->audioStarted = true;
		}

		endTime = timeGetTime();

		timetoSleep = endTime - startTime;

		Sleep(dwQuantum - timetoSleep);
	}

	_endthreadex(0);
	return 0;
}