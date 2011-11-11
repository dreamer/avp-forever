// Copyright (C) 2011 Barry Duncan. All Rights Reserved.
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

#include "TheoraPlayback.h"
#include "FmvPlayback.h"
#include "console.h"
#include <assert.h>
#include <process.h>
#include "logstring.h"

static const int kAudioBufferCount = 3;
static const int kAudioBufferSize  = 4096;

static const int kQuantum = 1000 / 60;

unsigned int __stdcall TheoraDecodeThread(void *args);
unsigned int __stdcall TheoraAudioThread(void *args);

TheoraPlayback::~TheoraPlayback()
{
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

		delete audioStream;
		delete[] mAudioData;
		delete[] mAudioDataBuffer;
		delete mRingBuffer;
	}

	ogg_sync_clear(&mState);

	// clear stream map
	for (streamMap::iterator it = mStreams.begin(); it != mStreams.end(); ++it)
	{
		delete it->second;
	}

	for (uint32_t i = 0; i < frameTextureIDs.size(); i++)
	{
		Tex_Release(frameTextureIDs[i]);
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

int TheoraPlayback::Open(const std::string &fileName)
{
#ifdef _XBOX
	mFileName = "d:/";
	mFileName += fileName;
#else
	mFileName = fileName;
#endif

	// hack to make the menu background FMV loop
	if (mFileName == "fmvs/menubackground.ogv")
	{
		isLooped = true;
	}

#ifdef _XBOX
	
	// change forwardslashes in path to backslashes
	std::replace(mFileName.begin(), mFileName.end(), '/', '\\');

#endif

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
		this->audioStream = new AudioStream();

		if (!this->audioStream->Init(mAudio->mVorbis.mInfo.channels, mAudio->mVorbis.mInfo.rate, 16, kAudioBufferSize, kAudioBufferCount))
		{
			Con_PrintError("Failed to create mAudio stream buffer for FMV playback");
			return FMV_ERROR;
		}

		// we need some temporary mAudio data storage space and a ring buffer instance
		mAudioData = new uint8_t[kAudioBufferSize];
		if (mAudioData == NULL)
		{
			Con_PrintError("Failed to create mAudioData stream buffer for FMV playback");
			return FMV_ERROR;
		}

		mRingBuffer = new RingBuffer(kAudioBufferSize * kAudioBufferCount);
		if (mRingBuffer == NULL)
		{
			Con_PrintError("Failed to create mRingBuffer for FMV playback");
			return FMV_ERROR;
		}

		// TODO: can we allocate this up front?
		mAudioDataBuffer = NULL;
	}

	mFrameWidth  = mVideo->mTheora.mInfo.frame_width;
	mFrameHeight = mVideo->mTheora.mInfo.frame_height;

	// determine how many textures we need
	if (/* can gpu do shader yuv->rgb? */ 1)
	{
		frameTextureIDs.resize(3);
		frameTextureIDs[0] = MISSING_TEXTURE;
		frameTextureIDs[1] = MISSING_TEXTURE;
		frameTextureIDs[2] = MISSING_TEXTURE;
		mNumTextureBits = 8;
	}
	else
	{
		// do CPU YUV->RGB with a single 32bit texture
		frameTextureIDs.resize(1);
		frameTextureIDs[0] = MISSING_TEXTURE;
		mNumTextureBits = 32;
	}

	for (uint32_t i = 0; i < frameTextureIDs.size(); i++)
	{
		uint32_t width  = mFrameWidth;
		uint32_t height = mFrameHeight;

		// the first texture is fullsize, the second two are quartersize
		if (i > 0)
		{
			width /= 2;
			height /= 2;
		}

		// create the texture with desired parameters
		frameTextureIDs[i] = Tex_Create(/*"CUTSCENE_"*/fileName + IntToString(i), width, height, mNumTextureBits, TextureUsage_Dynamic);
		if (frameTextureIDs[i] == MISSING_TEXTURE)
		{
			Con_PrintError("Unable to create texture(s) for FMV playback");
			return FMV_ERROR;
		}
	}

	mFmvPlaying = true;
	mAudioStarted = false;
	mFrameReady = false;

	InitializeCriticalSection(&mFrameCriticalSection);
	mFrameCriticalSectionInited = true;

	// now start the threads
	mDecodeThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, TheoraDecodeThread, static_cast<void*>(this), 0, NULL));

	if (mAudio)
	{
		mAudioThreadHandle = reinterpret_cast<HANDLE>(_beginthreadex(NULL, 0, TheoraAudioThread, static_cast<void*>(this), 0, NULL));
	}

	return FMV_OK;
}

void TheoraPlayback::Close()
{
	mFmvPlaying = false;
}

bool TheoraPlayback::HandleTheoraHeader(OggStream* stream, ogg_packet* packet)
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

bool TheoraPlayback::HandleSkeletonHeader(OggStream* stream, ogg_packet* packet)
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
	assert(0);
	return true;
}

bool TheoraPlayback::HandleVorbisHeader(OggStream* stream, ogg_packet* packet)
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

bool TheoraPlayback::IsPlaying()
{
	return mFmvPlaying;
}

// converts a decoded Theora YUV frame to RGB texture using CPU conversion routine
bool TheoraPlayback::ConvertFrame(uint32_t width, uint32_t height, uint8_t *bufferPtr, uint32_t pitch)
{
	if (mFmvPlaying == false)
		return false;

	// critical section
	EnterCriticalSection(&mFrameCriticalSection);

	OggPlayYUVChannels oggYuv;
	oggYuv.ptry = mYuvBuffer[0].data;
	oggYuv.ptru = mYuvBuffer[2].data;
	oggYuv.ptrv = mYuvBuffer[1].data;

	oggYuv.y_width  = mYuvBuffer[0].width;
	oggYuv.y_height = mYuvBuffer[0].height;
	oggYuv.y_pitch  = mYuvBuffer[0].stride;

	oggYuv.uv_width  = mYuvBuffer[1].width;
	oggYuv.uv_height = mYuvBuffer[1].height;
	oggYuv.uv_pitch  = mYuvBuffer[1].stride;

	OggPlayRGBChannels oggRgb;
	oggRgb.ptro = static_cast<uint8_t*>(bufferPtr);
	oggRgb.rgb_height = height;
	oggRgb.rgb_width  = width;
	oggRgb.rgb_pitch  = pitch;

	oggplay_yuv2rgb(&oggYuv, &oggRgb);

	mFrameReady = false;

	LeaveCriticalSection(&mFrameCriticalSection);

	return true;
}

// copies a decoded Theora YUV frame to texture(s) for GPU to convert via shader
bool TheoraPlayback::ConvertFrame()
{
	if (!mFmvPlaying)
		return false;

	// critical section
	EnterCriticalSection(&mFrameCriticalSection);

	for (uint32_t i = 0; i < frameTextureIDs.size(); i++)
	{
		uint8_t *originalDestPtr = NULL;
		uint32_t pitch = 0;

		uint32_t width  = 0;
		uint32_t height = 0;

		// get width and height
		Tex_GetDimensions(frameTextureIDs[i], width, height);

		// lock the texture
		if (Tex_Lock(frameTextureIDs[i], &originalDestPtr, &pitch, TextureLock_Discard)) // only do below if lock succeeds
		{
			for (uint32_t y = 0; y < height; y++)
			{
				uint8_t *destPtr = originalDestPtr + y * pitch;
				uint8_t *srcPtr  = mYuvBuffer[i].data + (y * mYuvBuffer[i].stride);

				// copy entire width row in one go
				memcpy(destPtr, srcPtr, width * sizeof(uint8_t));

				destPtr += width * sizeof(uint8_t);
				srcPtr  += width * sizeof(uint8_t);
			}

			// unlock texture
			Tex_Unlock(frameTextureIDs[i]);
		}
	}

	// set this value to true so we can now begin to draw the textured fmv frame
	mTexturesReady = true;

	mFrameReady = false;

	LeaveCriticalSection(&mFrameCriticalSection);

	return true;
}

ogg_int64_t TheoraPlayback::ReadPage(ogg_page *page)
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
			if (isLooped)
			{
				// we have to loop, so reset back to the start and keep going (dont return)
				mFileStream.clear();
				mFileStream.seekg(static_cast<long>(mDataOffset), std::ios::beg);
				mPageOffset = mDataOffset;
				ogg_sync_reset(&mState);
			}
			else
			{
				return -1;
			}
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

		ret = ogg_sync_wrote(&mState, static_cast<long>(amountRead));
		assert(ret == 0);
	}

	offset = mPageOffset;
	mPageOffset += page->header_len + page->body_len;

	return offset;
}

bool TheoraPlayback::ReadPacket(OggStream *stream, ogg_packet *packet)
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

void TheoraPlayback::ReadHeaders()
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

void TheoraPlayback::HandleTheoraData(OggStream *stream, ogg_packet *packet)
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

unsigned int __stdcall TheoraDecodeThread(void *args)
{
	TheoraPlayback *fmv = static_cast<TheoraPlayback*>(args);

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

			int32_t timeToSleep = static_cast<DWORD>((1.0f / framerate) * 1000);

			// just to be sure, clamp the value to 0 and greater
			if (timeToSleep < 0)
			{
				timeToSleep = 1;
			}

			Sleep(timeToSleep);
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
							delete[] fmv->mAudioDataBuffer;
							OutputDebugString("deleted mAudioDataBuffer to resize\n");
						}

						// double the bufer size
						fmv->mAudioDataBuffer = new uint16_t[dataSize * 2];
						if (!fmv->mAudioDataBuffer)
						{
							// TODO: break cleanly from FMV system here
							Con_PrintError("Out of memory for mAudioDataBuffer alloc!");
							break;
						}

						OutputDebugString("alloced mAudioDataBuffer\n");
						fmv->mAudioDataBufferSize = dataSize * 2;
					}

					uint16_t* p = fmv->mAudioDataBuffer;

					for (int32_t i = 0; i < samples; ++i)
					{
						for (int32_t j = 0; j < fmv->mAudio->mVorbis.mInfo.channels; ++j)
						{
							int v = static_cast<int>(floorf(0.5f + pcm[j][i]*32767.0f));
							if (v > 32767)  v = 32767;
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
							WaitForSingleObject(fmv->audioStream->voiceContext->hBufferEndEvent, /*INFINITE*/20);
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

unsigned int __stdcall TheoraAudioThread(void *args)
{
	#ifdef USE_XAUDIO2
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	#endif

	TheoraPlayback *fmv = static_cast<TheoraPlayback*>(args);

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
		timetoSleep -= kQuantum;

		if (timetoSleep < 0 || timetoSleep > kQuantum)
		{
			timetoSleep = 1;
		}

		Sleep(timetoSleep);
	}

	_endthreadex(0);
	return 0;
}