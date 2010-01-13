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

// forward declare classes
class VorbisDecode;
class TheoraDecode;

class TheoraDecode
{
	public:
		th_info 		mInfo;
		th_comment 		mComment;
		th_setup_info 	*mSetupInfo;
		th_dec_ctx		*mDecodeContext;

		TheoraDecode() :
			mSetupInfo(0),
			mDecodeContext(0)
		{
			th_info_init(&mInfo);
			th_comment_init(&mComment);
		}
		~TheoraDecode()
		{
			th_setup_free(mSetupInfo);
			th_decode_free(mDecodeContext);
		}
		void Init();
};

class VorbisDecode
{
	public:
		vorbis_info 		mInfo;
		vorbis_comment 		mComment;
		vorbis_dsp_state 	mDspState;
		vorbis_block 		mBlock;

		VorbisDecode()
		{
			vorbis_info_init(&mInfo);
			vorbis_comment_init(&mComment);
		}

		void Init();
};

class OggStream
{
	public:
		int 				mSerialNumber;
		ogg_stream_state	mStreamState;
		bool				mHeadersRead;
		bool				mActive;
		streamType			mType;
		TheoraDecode		mTheora;
		VorbisDecode		mVorbis;

		OggStream(int serialNumber) :
			mSerialNumber(-1),
			mType(TYPE_UNKNOWN),
			mActive(true)
		{
		}
		~OggStream()
		{
			int ret = ogg_stream_clear(&mStreamState);
			assert (ret == 0);
		}
};

typedef std::map<int, OggStream*> streamMap;

class TheoraFMV
{
	public:
		std::string 	mFileName;
		std::ifstream 	mFileStream;

		ogg_sync_state 	mState;
		ogg_int64_t		mGranulePos;
		th_ycbcr_buffer mYuvBuffer;

		streamMap		mStreams;

		// audio
		StreamingAudioBuffer *mAudioStream;
		RingBuffer *mRingBuffer;
		uint8_t	*mAudioData;
		uint16_t *mAudioDataBuffer;
		int	mAudioDataBufferSize;
		CRITICAL_SECTION mAudioCriticalSection;

		// video
		D3DTEXTURE mDisplayTexture;
		int mTextureWidth;
		int mTextureHeight;
		int mFrameWidth;
		int mFrameHeight;
		CRITICAL_SECTION mFrameCriticalSection;

		// thread handles
		HANDLE mDecodeThreadHandle;
		HANDLE mAudioThreadHandle;

		// so we can easily reference these from the threads.
		OggStream *mVideo;
		OggStream *mAudio;

		bool mFmvPlaying;
		bool mFrameReady;
		bool mAudioStarted;

		TheoraFMV(/*const char* fileName*/) :
			mGranulePos(0),
				mAudioStream(0),
				mRingBuffer(0),
				mAudioData(0),
				mAudioDataBuffer(0),
				mAudioDataBufferSize(0),
				mDisplayTexture(0),
				mTextureWidth(0),
				mTextureHeight(0),
				mFrameWidth(0),
				mFrameHeight(0),
				mDecodeThreadHandle(0),
				mAudioThreadHandle(0),
				mVideo(0),
				mAudio(0),
				mFmvPlaying(false),
				mFrameReady(false),
				mAudioStarted(false)
		{

		}
		~TheoraFMV()
		{
			if (mDecodeThreadHandle)
			{
				WaitForSingleObject(mDecodeThreadHandle, INFINITE);
				CloseHandle(mDecodeThreadHandle);
			}
			if (mAudioThreadHandle)
			{
				WaitForSingleObject(mAudioThreadHandle, INFINITE);
				CloseHandle(mAudioThreadHandle);
			}
			
			int ret = ogg_sync_clear(&mState);
			assert(ret == 0);

			for (streamMap::iterator it = mStreams.begin(); it != mStreams.end(); ++it)
			{
				OggStream *tempStream = (*it).second;

				// delete stream; we have no class so no destructor. do it manually.
				int ret = ogg_stream_clear(&tempStream->mStreamState);
				assert(ret == 0);
				th_setup_free(tempStream->mTheora.mSetupInfo);
				th_decode_free(tempStream->mTheora.mDecodeContext);
				delete tempStream;
			}
			if (mAudioStream)
			{
				AudioStream_ReleaseBuffer(mAudioStream);
			}
			if (mAudioData)
			{
				delete []mAudioData;
			}
			if (mAudioDataBuffer)
			{
				delete []mAudioDataBuffer;
			}
			if (mDisplayTexture)
			{
				mDisplayTexture->Release();
			}
		}

		int	Open(const char* fileName);
		bool ReadPage(ogg_page *page);
		bool ReadPacket(OggStream *stream, ogg_packet *packet);
		void ReadHeaders();
		void HandleTheoraData(OggStream *stream, ogg_packet *packet);
};

void TheoraDecode::Init()
{
	mDecodeContext = th_decode_alloc(&mInfo, mSetupInfo);
	assert(mDecodeContext != NULL);

	int ppmax = 0;
	int ret = th_decode_ctl(mDecodeContext, TH_DECCTL_GET_PPLEVEL_MAX, &ppmax, sizeof(ppmax));
	assert (ret == 0);

	ppmax = 0;

	ret = th_decode_ctl(mDecodeContext, TH_DECCTL_SET_PPLEVEL, &ppmax, sizeof(ppmax));
	assert(ret == 0);
}

void VorbisDecode::Init()
{
	int ret = vorbis_synthesis_init(&mDspState, &mInfo);
	assert(ret == 0);

	ret = vorbis_block_init(&mDspState, &mBlock);
	assert(ret == 0);
}

TheoraFMV *newFmv;

void fmvTest(const char* file)
{
	newFmv = new TheoraFMV();
	newFmv->Open(file);
}

int TheoraFMV::Open(const char* fileName)
{
	mFileName = fileName;

	// open the file
	mFileStream.open(mFileName.c_str(), std::ios::in | std::ios::binary);

	if (!mFileStream.is_open())
	{
		Con_PrintError("can't find FMV file " + mFileName);
		return FMV_INVALID_FILE;
	}

	if (ogg_sync_init(&mState) != 0)
	{
		Con_PrintError("ogg_sync_init failed");
		return FMV_ERROR;
	}

	// now read the headers in the ogg file
	ReadHeaders();

	// initialise the mStreams we found while reading the headers
	for (streamMap::iterator it = mStreams.begin(); it != mStreams.end(); ++it)
	{
		OggStream *stream = (*it).second;

		if (!mVideo && stream->mType == TYPE_THEORA)
		{
			mVideo = stream;
			mVideo->mTheora.Init();
		}

		if (!mAudio && stream->mType == TYPE_VORBIS)
		{
			mAudio = stream;
			mAudio->mVorbis.Init();
		}
		else stream->mActive = false;
	}

	// init the sound and mVideo api stuff we need
	if (mAudio)
	{
		// create mAudio streaming buffer
		mAudioStream = AudioStream_CreateBuffer(mAudio->mVorbis.mInfo.channels, mAudio->mVorbis.mInfo.rate, 4096, 3);
		if (mAudioStream == NULL)
		{
			Con_PrintError("Failed to create mAudio stream buffer for fmv playback");
			return FMV_ERROR;
		}

		// we need some temporary mAudio data storage space and a ring buffer instance
		mAudioData = new uint8_t[mAudioStream->bufferSize];
		mRingBuffer = new RingBuffer(mAudioStream->bufferSize * mAudioStream->bufferCount);

		mAudioDataBuffer = NULL;
	}

	if (!mDisplayTexture)
	{
		mFrameWidth = mVideo->mTheora.mInfo.frame_width;
		mFrameHeight = mVideo->mTheora.mInfo.frame_height;

		mTextureWidth = mFrameWidth;
		mTextureHeight = mTextureHeight;

		// create a new texture, passing width and height which will be set to the actual size of the created texture
		mDisplayTexture = CreateFmvTexture(&mTextureWidth, &mTextureHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
		if (!mDisplayTexture)
		{
			Con_PrintError("can't create FMV texture");
			return FMV_ERROR;
		}
	}

	mFmvPlaying = true;
	mAudioStarted = false;
	mFrameReady = false;

	InitializeCriticalSection(&mFrameCriticalSection);
	InitializeCriticalSection(&mAudioCriticalSection);

	// time to start our threads
	mDecodeThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, decodeThread, static_cast<void*>(this), 0, NULL));

	if (mAudio)
	{
		mAudioThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, audioThread, static_cast<void*>(this), 0, NULL));
	}

	Con_PrintMessage("fmv should be playing now");

	return FMV_OK;
}

bool TheoraFMV::ReadPage(ogg_page *page)
{
	int amountRead = 0;
	int ret = 0;

	// if end of file, keep processing any remaining buffered pages
	if (!mFileStream.good())
		return ogg_sync_pageout(&mState, page) == 1;

	while (ogg_sync_pageout(&mState, page) != 1)
	{
		char *buffer = ogg_sync_buffer(&mState, 4096);
		assert(buffer);

		mFileStream.read(buffer, 4096);
		amountRead = mFileStream.gcount();

		if (amountRead == 0)
		{
			// eof
			continue;
		}

		ret = ogg_sync_wrote(&mState, amountRead);
		assert(ret == 0);
	}

	return true;
}

bool TheoraFMV::ReadPacket(OggStream *stream, ogg_packet *packet)
{
	int ret = 0;

	while ((ret = ogg_stream_packetout(&stream->mStreamState, packet)) != 1)
	{
		ogg_page page;

		if (!ReadPage(&page))
			return false;

		int serialNumber = ogg_page_serialno(&page);
		assert(mStreams.find(serialNumber) != mStreams.end());
		OggStream *pageStream = mStreams[serialNumber];

		// Drop data for mStreams we're not interested in.
		if (stream->mActive)
		{
			ret = ogg_stream_pagein(&pageStream->mStreamState, &page);
			assert(ret == 0);
    	}
	}
	return true;
}

void TheoraFMV::ReadHeaders()
{
	ogg_page page;
	int ret = 0;

	bool gotAllHeaders = false;
	bool gotTheora = false;
	bool gotVorbis = false;

	while (!gotAllHeaders && ReadPage(&page))
	{
		OggStream *stream = 0;

		int serialNumber = ogg_page_serialno(&page);

		if (ogg_page_bos(&page))
		{
			stream = new OggStream(serialNumber);
			ret = ogg_stream_init(&stream->mStreamState, serialNumber);
			assert(ret == 0);

			// store in the stream std::map
			mStreams[serialNumber] = stream;
		}

		assert(mStreams.find(serialNumber) != mStreams.end());

		if (mStreams.find(serialNumber) != mStreams.end())
		{
			// change our stream pointer to the one now in the map
//			stream = mStreams[serialNumber];
		}

		stream = mStreams[serialNumber];

		// add the completed page to the bitstream
		ret = ogg_stream_pagein(&stream->mStreamState, &page);
		assert(ret == 0);

		ogg_packet packet;

		while (!gotAllHeaders && (ret = ogg_stream_packetpeek(&stream->mStreamState, &packet)) != 0)
		{
			assert(ret == 1);

			// check for a theora header first
			if (!gotTheora)
			{
				ret = th_decode_headerin(&stream->mTheora.mInfo, &stream->mTheora.mComment, &stream->mTheora.mSetupInfo, &packet);
				if (ret > 0)
				{
					// we found the header packet
					stream->mType = TYPE_THEORA;
				}
				else if (stream->mType == TYPE_THEORA && ret == 0)
				{
					gotTheora = true;
				}
			}

			// check for a vorbis header
			if (!gotVorbis)
			{
				ret = vorbis_synthesis_headerin(&stream->mVorbis.mInfo, &stream->mVorbis.mComment, &packet);
				if (ret == 0)
				{
					stream->mType = TYPE_VORBIS;
				}
				else if (stream->mType == TYPE_VORBIS && ret == OV_ENOTVORBIS)
				{
					gotVorbis = true;
				}
			}

			if (gotVorbis && gotTheora)
				gotAllHeaders = true;

			if (!gotAllHeaders)
			{
				// we need to move forward and grab the next packet
				ret = ogg_stream_packetout(&stream->mStreamState, &packet);
				assert(ret == 1);
			}
		}
	}
}

void TheoraFMV::HandleTheoraData(OggStream *stream, ogg_packet *packet)
{
	int ret = th_decode_packetin(stream->mTheora.mDecodeContext, packet, &mGranulePos);

	if (ret == TH_DUPFRAME)  // same as previous frame, don't bother decoding it
		return;

	assert (ret == 0);

	// critical section

	// We have a frame. Get the YUV data
	ret = th_decode_ycbcr_out(stream->mTheora.mDecodeContext, mYuvBuffer);
	assert (ret == 0);

	// we have a new frame and we're ready to use it
//	mFrameReady = true;

	// leave critical section
}

unsigned int __stdcall decodeThread(void *args)
{
	TheoraFMV *fmv = static_cast<TheoraFMV*>(args);

	ogg_packet packet;
	int ret = 0;
    float** pcm = 0;
    int samples = 0;

	while (fmv->ReadPacket(fmv->mAudio, &packet))
	{
		if (vorbis_synthesis(&fmv->mAudio->mVorbis.mBlock, &packet) == 0)
		{
			ret = vorbis_synthesis_blockin(&fmv->mAudio->mVorbis.mDspState, &fmv->mAudio->mVorbis.mBlock);
			assert(ret == 0);
		}

		// get pointer to array of floating point values for sound samples
		while ((samples = vorbis_synthesis_pcmout(&fmv->mAudio->mVorbis.mDspState, &pcm)) > 0)
		{
			if (samples > 0)
			{
				int dataSize = samples * fmv->mAudio->mVorbis.mInfo.channels;

				if (fmv->mAudioDataBuffer == NULL)
				{
					// make it twice the size we currently need to avoid future reallocations
					fmv->mAudioDataBuffer = new uint16_t[dataSize * 2];
					fmv->mAudioDataBufferSize = dataSize * 2;
				}

				if (fmv->mAudioDataBufferSize < dataSize)
				{
					if (fmv->mAudioDataBuffer != NULL)
					{
						delete []fmv->mAudioDataBuffer;
						OutputDebugString("deleted mAudioDataBuffer to resize\n");
					}

					fmv->mAudioDataBuffer = new uint16_t[dataSize * 2];
					OutputDebugString("alloc mAudioDataBuffer\n");

					if (!fmv->mAudioDataBuffer)
					{
						OutputDebugString("Out of memory\n");
						break;
					}
					fmv->mAudioDataBufferSize = dataSize * 2;
				}

				uint16_t* p = fmv->mAudioDataBuffer;

				for (int i = 0; i < samples; ++i)
				{
					for (int j = 0; j < fmv->mAudio->mVorbis.mInfo.channels; ++j)
					{
						int v = static_cast<int>(floorf(0.5f + pcm[j][i]*32767.0f));
						if (v > 32767) v = 32767;
						if (v < -32768) v = -32768;
						*p++ = v;
					}
				}

				int audioSize = samples * fmv->mAudio->mVorbis.mInfo.channels * sizeof(uint16_t);

				// move the critical section stuff into the ring buffer code itself?
				EnterCriticalSection(&fmv->mAudioCriticalSection);

				int freeSpace = fmv->mRingBuffer->GetWritableSize();

				// if we can't fit all our data..
				if (audioSize > freeSpace)
				{
					//while (audioSize > RingBuffer_GetWritableSpace())
					while (audioSize > fmv->mRingBuffer->GetWritableSize())
					{
						// little bit of insurance in case we get stuck in here
						//if (!fmvInstance->mFmvPlaying)
						//	break;

						Sleep(16);
					}
				}

				fmv->mRingBuffer->WriteData((uint8_t*)&fmv->mAudioDataBuffer[0], audioSize);

				LeaveCriticalSection(&fmv->mAudioCriticalSection);
			}

			// tell vorbis how many samples we consumed
			ret = vorbis_synthesis_read(&fmv->mAudio->mVorbis.mDspState, samples);
			assert(ret == 0);

			if (fmv->mVideo)
			{
				float audio_time = static_cast<float>(AudioStream_GetNumSamplesPlayed(fmv->mAudioStream)) / static_cast<float>(fmv->mAudio->mVorbis.mInfo.rate);
				float video_time = static_cast<float>(th_granule_time(fmv->mVideo->mTheora.mDecodeContext, fmv->mGranulePos));

//				char buf[100];
//				sprintf(buf, "audio_time: %f video_time: %f\n", audio_time, video_time);
//				OutputDebugString(buf);

				// if mAudio is ahead of frame time, display a new frame
				if ((audio_time > video_time))
				{
					// Decode one frame and display it. If no frame is available we
					// don't do anything.
					ogg_packet packet;
					if (fmv->ReadPacket(fmv->mVideo, &packet))
					{
						fmv->HandleTheoraData(fmv->mVideo, &packet);
						video_time = th_granule_time(fmv->mVideo->mTheora.mDecodeContext, fmv->mGranulePos);
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

	TheoraFMV *fmv = static_cast<TheoraFMV*>(args);

	DWORD dwQuantum = 1000 / 60;

	static int totalRead = 0;
	static int lastRead = 0;

	int startTime = 0;
	int endTime = 0;
	int timetoSleep = 0;

	while (fmv->mFmvPlaying)
	{
		startTime = timeGetTime();

		int numBuffersFree = AudioStream_GetNumFreeBuffers(fmv->mAudioStream);

		while (numBuffersFree)
		{
			int readableAudio = fmv->mRingBuffer->GetReadableSize();

			// we can fill a buffer
			if (readableAudio >= fmv->mAudioStream->bufferSize)
			{
				fmv->mRingBuffer->ReadData(fmv->mAudioData, fmv->mAudioStream->bufferSize);
				AudioStream_WriteData(fmv->mAudioStream, fmv->mAudioData, fmv->mAudioStream->bufferSize);

				numBuffersFree--;
			}
			else
			{
				// not enough mAudio available this time
				break;
			}
		}

		if (fmv->mAudioStarted == false)
		{
			AudioStream_PlayBuffer(fmv->mAudioStream);
			fmv->mAudioStarted = true;
		}

		endTime = timeGetTime();

		timetoSleep = endTime - startTime;

		Sleep(dwQuantum - timetoSleep);
	}

	_endthreadex(0);
	return 0;
}