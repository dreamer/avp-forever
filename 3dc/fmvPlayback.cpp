#include "fmvPlayback.h"
#include "3dc.h"
#include "console.h"
#include <stdint.h>
#include "assert.h"
#include <process.h>
#define restrict
#include <emmintrin.h>
#include <math.h>

#ifdef _XBOX
#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

static const int BUFFER_SIZE  = 4096;
static const int BUFFER_COUNT = 3;

#define VANILLA

unsigned int __stdcall decodeThread(void *args);
unsigned int __stdcall audioThread(void *args);

inline int CLAMP(int value)
{
	if (value > 255)
		return 255;
	else if (value < 0)
		return 0;
	else return value;
}

/* structure holds pointers to y, u, v channels */
typedef struct _OggPlayYUVChannels {
    unsigned char * ptry;
    unsigned char * ptru;
    unsigned char * ptrv;
    int             y_width;
    int             y_height;
	int				y_pitch;
    int             uv_width;
    int             uv_height;
	int				uv_pitch;
} OggPlayYUVChannels;

/* structure holds pointers to y, u, v channels */
typedef struct _OggPlayRGBChannels {
    unsigned char * ptro;
    int             rgb_width;
    int             rgb_height;
	int				rgb_pitch;
} OggPlayRGBChannels;

void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb);

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

TheoraFMV::~TheoraFMV()
{
	if (mFmvPlaying)
		mFmvPlaying = false;
		
	// wait for decode thread to finish
	if (mDecodeThreadHandle)
	{
		WaitForSingleObject(mDecodeThreadHandle, INFINITE);
		CloseHandle(mDecodeThreadHandle);
	}

	if (mAudio)
	{

		// wait for audio thread to finish
		if (mAudioThreadHandle)
		{
			WaitForSingleObject(mAudioThreadHandle, INFINITE);
			CloseHandle(mAudioThreadHandle);
		}
		
	//FIXME	int ret = ogg_sync_clear(&mState);
	//	assert(ret == 0);

		if (audioStream)
			delete audioStream;

		if (mAudioData)
		{
			delete []mAudioData;
		}

		if (mAudioDataBuffer)
		{
			delete []mAudioDataBuffer;
		}

		delete mRingBuffer;
	}

	for (uint32_t i = 0; i < 3; i++)
	{
		if (frameTextures[i].texture)
			frameTextures[i].texture->Release();
	}

	if (mFrameCriticalSectionInited)
	{
		DeleteCriticalSection(&mFrameCriticalSection);
		mFrameCriticalSectionInited = false;
	}
}

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

int TheoraFMV::Open(const std::string &fileName)
{

#ifdef _XBOX
	mFileName = "d:\\";
	mFileName += fileName;
#else
	mFileName = fileName;
#endif

	frameTextures[0].texture = 0;
	frameTextures[1].texture = 0;
	frameTextures[2].texture = 0;

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
		this->audioStream = new AudioStream(mAudio->mVorbis.mInfo.channels, mAudio->mVorbis.mInfo.rate, BUFFER_SIZE, BUFFER_COUNT);
		if (this->audioStream == NULL)
		{
			Con_PrintError("Failed to create mAudio stream buffer for fmv playback");
			return FMV_ERROR;
		}

		// we need some temporary mAudio data storage space and a ring buffer instance
		mAudioData = new uint8_t[BUFFER_SIZE];
		mRingBuffer = new RingBuffer(BUFFER_SIZE * BUFFER_COUNT);

		mAudioDataBuffer = NULL;
	}
/*
	if (!mDisplayTexture)
	{
		mFrameWidth = mVideo->mTheora.mInfo.frame_width;
		mFrameHeight = mVideo->mTheora.mInfo.frame_height;

		mTextureWidth = mFrameWidth;
		mTextureHeight = mFrameHeight;

		// create a new texture, passing width and height which will be set to the actual size of the created texture
		mDisplayTexture = CreateFmvTexture(&mTextureWidth, &mTextureHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
		if (!mDisplayTexture)
		{
			Con_PrintError("can't create FMV texture");
			return FMV_ERROR;
		}
	}
*/
	mFrameWidth = mVideo->mTheora.mInfo.frame_width;
	mFrameHeight = mVideo->mTheora.mInfo.frame_height;

	frameTextures[0].width = mFrameWidth;
	frameTextures[0].height = mFrameHeight;

	frameTextures[1].width = frameTextures[2].width = mFrameWidth / 2;
	frameTextures[1].height = frameTextures[2].height = mFrameHeight / 2;

	for (uint32_t i = 0; i < 3; i++)
	{
		// create a new texture, passing width and height which will be set to the actual size of the created texture
		frameTextures[i].texture = CreateFmvTexture2(&frameTextures[i].width, &frameTextures[i].height);
		if (!frameTextures[i].texture)
		{
			Con_PrintError("can't create FMV texture");
			return FMV_ERROR;
		}
	}

	mFmvPlaying = true;
	mAudioStarted = false;
	mFrameReady = false;

	InitializeCriticalSection(&mFrameCriticalSection);
	mFrameCriticalSectionInited = true;

	// time to start our threads
	mDecodeThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, decodeThread, static_cast<void*>(this), 0, NULL));

	if (mAudio)
	{
		mAudioThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, audioThread, static_cast<void*>(this), 0, NULL));
	}

	return FMV_OK;
}

void TheoraFMV::Close()
{
	mFmvPlaying = false;
}

bool TheoraFMV::HandleTheoraHeader(OggStream* stream, ogg_packet* packet)
{
	int ret = th_decode_headerin(&stream->mTheora.mInfo,
		&stream->mTheora.mComment,
		&stream->mTheora.mSetupInfo,
		packet);

	if (ret == TH_ENOTFORMAT)
		return false; // Not a theora header

	if (ret > 0) 
	{
		// This is a theora header packet
		stream->mType = TYPE_THEORA;
		return false;
	}

	// Any other return value is treated as a fatal error
	assert(ret == 0);

	// This is not a header packet. It is the first 
	// video data packet.
	return true;
}

bool TheoraFMV::HandleSkeletonHeader(OggStream* stream, ogg_packet* packet) 
{
	// Is it a "fishead" skeleton identifier packet?
	if (packet->bytes > 8 && memcmp(packet->packet, "fishead", 8) == 0) 
	{
		stream->mType = TYPE_SKELETON;
		return false;
	}
  
	if (stream->mType != TYPE_SKELETON) 
	{
		// The first packet must be the skeleton identifier.
		return false;
	}
  
	// "fisbone" stream info packet?
	if (packet->bytes >= 8 && memcmp(packet->packet, "fisbone", 8) == 0) 
	{
		return false;
	}
  
	// "index" keyframe index packet?
	if (packet->bytes > 6 && memcmp(packet->packet, "index", 6) == 0) 
	{
		return false;
	}
  
	if (packet->e_o_s) 
	{
		return false;
	}
  
	// Shouldn't actually get here.
	return true;
}

bool TheoraFMV::HandleVorbisHeader(OggStream* stream, ogg_packet* packet)
{
	int ret = vorbis_synthesis_headerin(&stream->mVorbis.mInfo,
		&stream->mVorbis.mComment,
		packet);

	// Unlike libtheora, libvorbis does not provide a return value to
	// indicate that we've finished loading the headers and got the
	// first data packet. To detect this I check if I already know the
	// stream type and if the vorbis_synthesis_headerin call failed.
	if (stream->mType == TYPE_VORBIS && ret == OV_ENOTVORBIS) 
	{
		// First data packet
		return true;
	}
	else if (ret == 0) 
	{
		stream->mType = TYPE_VORBIS;
	}
	return false;
}

bool TheoraFMV::IsPlaying()
{
	return mFmvPlaying;
}

bool TheoraFMV::NextFrame(uint32_t width, uint32_t height, uint8_t *bufferPtr, uint32_t pitch)
{
	if (mFmvPlaying == false)
		return false;

	// critical section
	EnterCriticalSection(&mFrameCriticalSection);

	OggPlayYUVChannels oggYuv;
	oggYuv.ptry = mYuvBuffer[0].data;
	oggYuv.ptru = mYuvBuffer[2].data;
	oggYuv.ptrv = mYuvBuffer[1].data;

	oggYuv.y_width = mYuvBuffer[0].width;
	oggYuv.y_height = mYuvBuffer[0].height;
	oggYuv.y_pitch = mYuvBuffer[0].stride;

	oggYuv.uv_width = mYuvBuffer[1].width;
	oggYuv.uv_height = mYuvBuffer[1].height;
	oggYuv.uv_pitch = mYuvBuffer[1].stride;

	OggPlayRGBChannels oggRgb;
	oggRgb.ptro = static_cast<uint8_t*>(bufferPtr);
	oggRgb.rgb_height = height;
	oggRgb.rgb_width = width;
	oggRgb.rgb_pitch = pitch;

	oggplay_yuv2rgb(&oggYuv, &oggRgb);

	LeaveCriticalSection(&mFrameCriticalSection);

//	char buf[100];
//	sprintf(buf, "total time: %d\n", timeGetTime() - startTime);
//	OutputDebugString(buf);

	mFrameReady = false;

	return true;
}

bool TheoraFMV::NextFrame()
{
	if (mFmvPlaying == false)
		return false;
/*
	D3DLOCKED_RECT textureLock;
	if (FAILED(mDisplayTexture->LockRect(0, &textureLock, NULL, D3DLOCK_DISCARD)))
	{
		OutputDebugString("can't lock FMV texture\n");
		return false;
	}
*/
	// critical section
	EnterCriticalSection(&mFrameCriticalSection);

	D3DLOCKED_RECT texLock[3];

	for (uint32_t i = 0; i < 3; i++)
	{
		if (FAILED(frameTextures[i].texture->LockRect(0, &texLock[i], NULL, D3DLOCK_DISCARD)))
		{
			OutputDebugString("can't lock FMV texture\n");
			return false;
		}
	}

	for (uint32_t i = 0; i < 3; i++)
	{
		for (uint32_t y = 0; y < frameTextures[i].height; y++)
		{
			uint8_t *destPtr = static_cast<uint8_t*>(texLock[i].pBits) + y * texLock[i].Pitch;
			uint8_t *srcPtr = mYuvBuffer[i].data + (y * mYuvBuffer[i].stride);

			// copy entire width row in one go
			memcpy(destPtr, srcPtr, frameTextures[i].width * sizeof(uint8_t));

			destPtr += frameTextures[i].width * sizeof(uint8_t);
			srcPtr += frameTextures[i].width * sizeof(uint8_t);
/*
			for (uint32_t x = 0; x < frameTextures[i].width; x++)
			{
				memcpy(destPtr, srcPtr, sizeof(uint8_t));

				destPtr += sizeof(uint8_t);
				srcPtr += sizeof(uint8_t);
			}
*/
		}
	}

/*
	OggPlayYUVChannels oggYuv;
	oggYuv.ptry = mYuvBuffer[0].data;
	oggYuv.ptru = mYuvBuffer[2].data;
	oggYuv.ptrv = mYuvBuffer[1].data;

	oggYuv.y_width = mYuvBuffer[0].width;
	oggYuv.y_height = mYuvBuffer[0].height;
	oggYuv.y_pitch = mYuvBuffer[0].stride;

	oggYuv.uv_width = mYuvBuffer[1].width;
	oggYuv.uv_height = mYuvBuffer[1].height;
	oggYuv.uv_pitch = mYuvBuffer[1].stride;

	OggPlayRGBChannels oggRgb;
	oggRgb.ptro = static_cast<uint8_t*>(textureLock.pBits);
	oggRgb.rgb_height = mFrameHeight;
	oggRgb.rgb_width = mFrameWidth;
	oggRgb.rgb_pitch = textureLock.Pitch;

	oggplay_yuv2rgb(&oggYuv, &oggRgb);

	if (FAILED(mDisplayTexture->UnlockRect(0)))
	{
		OutputDebugString("can't unlock FMV texture\n");
		return false;
	}
*/
	frameTextures[0].texture->UnlockRect(0);
	frameTextures[1].texture->UnlockRect(0);
	frameTextures[2].texture->UnlockRect(0);

	// set this value to true so we can now begin to draw the textured fmv frame
	if (!mTexturesReady)
		mTexturesReady = true;
/*
	tex[0]->UnlockRect(0);
	tex[1]->UnlockRect(0);
	tex[2]->UnlockRect(0);
*/
	mFrameReady = false;

	LeaveCriticalSection(&mFrameCriticalSection);

	return true;
}

ogg_int64_t TheoraFMV::ReadPage(ogg_page *page)
{
	ogg_int64_t offset = 0;
	int ret = 0;

	// if end of file, keep processing any remaining buffered pages
	if (!mFileStream.good())
	{
		if (ogg_sync_pageout(&mState, page) == 1) 
		{
			offset = mPageOffset;
			mPageOffset += page->header_len + page->body_len;
			return offset;
		}
		else
		{
			return -1;
		}
	}

	while (ogg_sync_pageout(&mState, page) != 1)
	{
		char *buffer = ogg_sync_buffer(&mState, 4096);
		assert(buffer);

		mFileStream.read(buffer, 4096);
		std::streamsize amountRead = mFileStream.gcount();

		if (amountRead == 0)
		{
			// eof
			continue;
		}

		ret = ogg_sync_wrote(&mState, amountRead);
		assert(ret == 0);
	}

	offset = mPageOffset;
	mPageOffset += page->header_len + page->body_len;
	return offset;
}

bool TheoraFMV::ReadPacket(OggStream *stream, ogg_packet *packet)
{
	int ret = 0;

	while ((ret = ogg_stream_packetout(&stream->mStreamState, packet)) != 1)
	{
		ogg_page page;

		if (ReadPage(&page) == -1)
			return false;

		int serialNumber = ogg_page_serialno(&page);
		assert(mStreams.find(serialNumber) != mStreams.end());
		OggStream *pageStream = mStreams[serialNumber];

		// Drop data for mStreams we're not interested in.
//		if (stream->mActive)
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
	ogg_int64_t offset = 0;
	int ret = 0;

	bool headersDone = false;

	while (!headersDone && (offset = ReadPage(&page)) != -1)
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

		while (!headersDone && (ret = ogg_stream_packetpeek(&stream->mStreamState, &packet)) != 0)
		{
			assert(ret == 1);

			// A packet is available. If it is not a header packet we exit.
			// If it is a header packet, process it as normal.
			headersDone = headersDone || HandleTheoraHeader(stream, &packet);
			headersDone = headersDone || HandleVorbisHeader(stream, &packet);
			headersDone = headersDone || HandleSkeletonHeader(stream, &packet);
			if (!headersDone) 
			{
				// Consume the packet
				ret = ogg_stream_packetout(&stream->mStreamState, &packet);
				assert(ret == 1);
			} 
			else 
			{
				// First non-header page. Remember its location, so we can seek
				// to time 0.
				mDataOffset = offset;
			}
		}
	}
	assert(mDataOffset != 0);
}

void TheoraFMV::HandleTheoraData(OggStream *stream, ogg_packet *packet)
{
	int ret = th_decode_packetin(stream->mTheora.mDecodeContext, packet, &mGranulePos);

	if (ret == TH_DUPFRAME)  // same as previous frame, don't bother decoding it
		return;

	assert (ret == 0);

	// critical section
	EnterCriticalSection(&mFrameCriticalSection);

	// We have a frame. Get the YUV data
	ret = th_decode_ycbcr_out(stream->mTheora.mDecodeContext, mYuvBuffer);
	assert (ret == 0);

	// we have a new frame and we're ready to use it
	mFrameReady = true;

	// leave critical section
	LeaveCriticalSection(&mFrameCriticalSection);
}

unsigned int __stdcall decodeThread(void *args)
{
	TheoraFMV *fmv = static_cast<TheoraFMV*>(args);

	ogg_packet packet;
	int ret = 0;
    float** pcm = 0;
    int samples = 0;

	// this is the loop we run if our video has no audio (time to fps)
	if (!fmv->mAudio)
	{
		while (fmv->ReadPacket(fmv->mVideo, &packet))
		{
			if (!fmv->IsPlaying())
				break;

			fmv->HandleTheoraData(fmv->mVideo, &packet);
			float framerate = float(fmv->mVideo->mTheora.mInfo.fps_numerator) / float(fmv->mVideo->mTheora.mInfo.fps_denominator);
			Sleep(static_cast<DWORD>((1.0f / framerate) * 1000));
		}
	}
	else // we have audio (and assume we have video..)
	{
		while (fmv->ReadPacket(fmv->mAudio, &packet))
		{
			// check if we should still be playing or not
			if (!fmv->IsPlaying())
				break;

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
					uint32_t dataSize = samples * fmv->mAudio->mVorbis.mInfo.channels;

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

					for (uint32_t i = 0; i < samples; ++i)
					{
						for (uint32_t j = 0; j < fmv->mAudio->mVorbis.mInfo.channels; ++j)
						{
							int v = static_cast<int>(floorf(0.5f + pcm[j][i]*32767.0f));
							if (v > 32767) v = 32767;
							if (v < -32768) v = -32768;
							*p++ = v;
						}
					}

					uint32_t audioSize = samples * fmv->mAudio->mVorbis.mInfo.channels * sizeof(uint16_t);

					uint32_t freeSpace = fmv->mRingBuffer->GetWritableSize();

					// if we can't fit all our data..
					if (audioSize > freeSpace)
					{
						while (audioSize > fmv->mRingBuffer->GetWritableSize())
						{
							// little bit of insurance in case we get stuck in here
							if (!fmv->mFmvPlaying)
								break;

							// wait for the audio buffer to tell us it's just freed up another audio buffer for us to fill
							WaitForSingleObject(fmv->audioStream->voiceContext->hBufferEndEvent, INFINITE );
						}
					}

					fmv->mRingBuffer->WriteData((uint8_t*)&fmv->mAudioDataBuffer[0], audioSize);
				}

				// tell vorbis how many samples we consumed
				ret = vorbis_synthesis_read(&fmv->mAudio->mVorbis.mDspState, samples);
				assert(ret == 0);

				if (fmv->mVideo)
				{
					double audio_time = static_cast<double>(fmv->audioStream->GetNumSamplesPlayed()) / static_cast<double>(fmv->mAudio->mVorbis.mInfo.rate);
					double video_time = static_cast<double>(th_granule_time(fmv->mVideo->mTheora.mDecodeContext, fmv->mGranulePos));

//					char buf[200];
//					sprintf(buf, "audio_time: %f, video_time: %f\n", audio_time, video_time);
//					OutputDebugString(buf);

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
	}

	fmv->mFmvPlaying = false;
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

		uint32_t numBuffersFree = fmv->audioStream->GetNumFreeBuffers();

		while (numBuffersFree)
		{
			uint32_t readableAudio = fmv->mRingBuffer->GetReadableSize();

			// we can fill a buffer
			if (readableAudio >= fmv->audioStream->GetBufferSize())
			{
				fmv->mRingBuffer->ReadData(fmv->mAudioData, fmv->audioStream->GetBufferSize());
				fmv->audioStream->WriteData(fmv->mAudioData, fmv->audioStream->GetBufferSize());

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
			fmv->audioStream->Play();
			fmv->mAudioStarted = true;
		}

		endTime = timeGetTime();

		timetoSleep = endTime - startTime;

		Sleep(dwQuantum - timetoSleep);
	}

	_endthreadex(0);
	return 0;
}

/* Vanilla implementation if YUV->RGB conversion */
void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb)
{
#ifdef VANILLA
	unsigned char * ptry = yuv->ptry;
	unsigned char * ptru = yuv->ptru;
	unsigned char * ptrv = yuv->ptrv;
	unsigned char * ptro = rgb->ptro;
	unsigned char * ptro2;
	int i, j;

	for (i = 0; i < yuv->y_height; i++)
	{
		ptro2 = ptro;
		for (j = 0; j < yuv->y_width; j += 2)
		{
			short pr, pg, pb, y;
			short r, g, b;

			pr = (-56992 + ptrv[j/2] * 409) >> 8;
			pg = (34784 - ptru[j/2] * 100 - ptrv[j/2] * 208) >> 8;
			pb = (-70688 + ptru[j/2] * 516) >> 8;

			y = 298*ptry[j] >> 8;
			r = y + pr;
			g = y + pg;
			b = y + pb;

			*ptro2++ = CLAMP(r);
			*ptro2++ = CLAMP(g);
			*ptro2++ = CLAMP(b);
			*ptro2++ = 255;

			y = 298*ptry[j + 1] >> 8;
			r = y + pr;
			g = y + pg;
			b = y + pb;

			*ptro2++ = CLAMP(r);
			*ptro2++ = CLAMP(g);
			*ptro2++ = CLAMP(b);
			*ptro2++ = 255;
		}

		ptry += yuv->y_pitch;

		if (i & 1)
		{
			ptru += yuv->uv_pitch;
			ptrv += yuv->uv_pitch;
		}

		ptro += rgb->rgb_pitch;
	}
#else // mmx
	int               i;
	unsigned char   * restrict ptry;
	unsigned char   * restrict ptru;
	unsigned char   * restrict ptrv;
	unsigned char   * ptro;

	register __m64    *y, *o;
	register __m64    zero, ut, vt, imm, imm2;
	register __m64    r, g, b;
	register __m64    tmp, tmp2;

	zero = _mm_setzero_si64();

	ptro = rgb->ptro;
	ptry = yuv->ptry;
	ptru = yuv->ptru;
	ptrv = yuv->ptrv;

	for (i = 0; i < yuv->y_height; i++)
	{
		int j;
		o = (__m64*)ptro;
		ptro += rgb->rgb_pitch;

		for (j = 0; j < yuv->y_width; j += 8)
		{
			y = (__m64*)&ptry[j];

			ut = _m_from_int(*(int *)(ptru + j/2));
			vt = _m_from_int(*(int *)(ptrv + j/2));

			ut = _m_punpcklbw(ut, zero);
			vt = _m_punpcklbw(vt, zero);

			/* subtract 128 from u and v */
			imm = _mm_set1_pi16(128);
			ut = _m_psubw(ut, imm);
			vt = _m_psubw(vt, imm);

			/* transfer and multiply into r, g, b registers */
			imm = _mm_set1_pi16(-51);
			g = _m_pmullw(ut, imm);
			imm = _mm_set1_pi16(130);
			b = _m_pmullw(ut, imm);
			imm = _mm_set1_pi16(146);
			r = _m_pmullw(vt, imm);
			imm = _mm_set1_pi16(-74);
			imm = _m_pmullw(vt, imm);
			g = _m_paddsw(g, imm);

			/* add 64 to r, g and b registers */
			imm = _mm_set1_pi16(64);
			r = _m_paddsw(r, imm);
			g = _m_paddsw(g, imm);
			imm = _mm_set1_pi16(32);
			b = _m_paddsw(b, imm);

			/* shift r, g and b registers to the right */
			r = _m_psrawi(r, 7);
			g = _m_psrawi(g, 7);
			b = _m_psrawi(b, 6);

			/* subtract 16 from r, g and b registers */
			imm = _mm_set1_pi16(16);
			r = _m_psubsw(r, imm);
			g = _m_psubsw(g, imm);
			b = _m_psubsw(b, imm);

			y = (__m64*)&ptry[j];

			/* duplicate u and v channels and add y
			 * each of r,g, b in the form [s1(16), s2(16), s3(16), s4(16)]
			 * first interleave, so tmp is [s1(16), s1(16), s2(16), s2(16)]
			 * then add y, then interleave again
			 * then pack with saturation, to get the desired output of
			 *   [s1(8), s1(8), s2(8), s2(8), s3(8), s3(8), s4(8), s4(8)]
			 */
			tmp = _m_punpckhwd(r, r);
			imm = _m_punpckhbw(*y, zero);
			//printf("tmp: %llx imm: %llx\n", tmp, imm);
			tmp = _m_paddsw(tmp, imm);
			tmp2 = _m_punpcklwd(r, r);
			imm2 = _m_punpcklbw(*y, zero);
			tmp2 = _m_paddsw(tmp2, imm2);
			r = _m_packuswb(tmp2, tmp);

			tmp = _m_punpckhwd(g, g);
			tmp2 = _m_punpcklwd(g, g);
			tmp = _m_paddsw(tmp, imm);
			tmp2 = _m_paddsw(tmp2, imm2);
			g = _m_packuswb(tmp2, tmp);

			tmp = _m_punpckhwd(b, b);
			tmp2 = _m_punpcklwd(b, b);
			tmp = _m_paddsw(tmp, imm);
			tmp2 = _m_paddsw(tmp2, imm2);
			b = _m_packuswb(tmp2, tmp);
			//printf("duplicated r g and b: %llx %llx %llx\n", r, g, b);

			/* now we have 8 8-bit r, g and b samples.  we want these to be packed
			 * into 32-bit values.
			 */
			//r = _m_from_int(0);
			//b = _m_from_int(0);
			imm = _mm_set1_pi32(0xFFFFFFFF);
			tmp = _m_punpcklbw(r, b);
			tmp2 = _m_punpcklbw(g, imm);
			*o++ = _m_punpcklbw(tmp, tmp2);
			*o++ = _m_punpckhbw(tmp, tmp2);
			//printf("tmp, tmp2, write1, write2: %llx %llx %llx %llx\n", tmp, tmp2,
			//                _m_punpcklbw(tmp, tmp2), _m_punpckhbw(tmp, tmp2));
			tmp = _m_punpckhbw(r, b);
			tmp2 = _m_punpckhbw(g, imm);
			*o++ = _m_punpcklbw(tmp, tmp2);
			*o++ = _m_punpckhbw(tmp, tmp2);
		}
		if (i & 0x1)
		{
			ptru += yuv->uv_pitch;
			ptrv += yuv->uv_pitch;
		}
		ptry += yuv->y_pitch;
	}
	_m_empty();
#endif
}

// this code is based on plogg by Chris Double. Also contains code written by Rebellion and myself. Below is the license from plogg

// Copyright (C) 2009 Chris Double. All Rights Reserved.
// The original author of this code can be contacted at: chris.double@double.co.nz
//
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
//

// this code contains code from the liboggplay library. the yuv->rgb routines are taken from their source code. Below is the license that is
// included with liboggplay

/*
   Copyright (C) 2003 CSIRO Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the CSIRO nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/