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

#ifndef _TheoraPlayback_h_
#define _TheoraPlayback_h_

#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include <ogg/ogg.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <vorbis/vorbisfile.h>
#include <fstream>
#include <map>
#include <string>
#include <stdint.h>
#include "RingBuffer.h"
#include "AudioStreaming.h"
#include "TextureManager.h"
#include <assert.h>

enum streamType
{
	TYPE_VORBIS,
	TYPE_THEORA,
	TYPE_SKELETON,
	TYPE_UNKNOWN
};

// forward declare classes
class VorbisDecode;
class TheoraDecode;
class OggStream;

typedef std::map<int, OggStream*> streamMap;

class TheoraDecode
{
	public:
		th_info			mInfo;
		th_comment		mComment;
		th_setup_info	*mSetupInfo;
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
		vorbis_info			mInfo;
		vorbis_comment		mComment;
		vorbis_dsp_state	mDspState;
		vorbis_block		mBlock;

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
		int					mSerialNumber;
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

class TheoraPlayback
{
	private:
		std::string		mFileName;
		std::ifstream	mFileStream;

		ogg_sync_state	mState;
		th_ycbcr_buffer	mYuvBuffer;
		streamMap		mStreams;

		// thread handles
		HANDLE mDecodeThreadHandle;
		HANDLE mAudioThreadHandle;

		CRITICAL_SECTION mFrameCriticalSection;
		bool mFrameCriticalSectionInited;

		// Offset of the page which was last read.
		ogg_int64_t mPageOffset;

		// Offset of first non-header page in file.
		ogg_int64_t mDataOffset;

		bool isLooped;

	public:
		// so we can easily reference these from the threads.
		OggStream		*mVideo;
		OggStream		*mAudio;

		ogg_int64_t		mGranulePos;

		// audio
		AudioStream		*audioStream;
		RingBuffer		*mRingBuffer;
		uint8_t			*mAudioData;
		uint16_t		*mAudioDataBuffer;
		uint32_t		mAudioDataBufferSize;

		// textures for video frames
		std::vector<texID_t> frameTextureIDs;

		uint32_t mTextureWidth;
		uint32_t mTextureHeight;
		uint32_t mFrameWidth;
		uint32_t mFrameHeight;
		uint8_t  mNumTextureBits;

		volatile bool mFmvPlaying;
		volatile bool mFrameReady;
		bool mAudioStarted;
		volatile bool mTexturesReady;

		TheoraPlayback() :
			mGranulePos(0),
			audioStream(0),
			mRingBuffer(0),
			mAudioData(0),
			mAudioDataBuffer(0),
			mAudioDataBufferSize(0),
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
			mAudioStarted(false),
			mTexturesReady(false),
			isLooped(false),
			mPageOffset(0),
			mDataOffset(0),
			mFrameCriticalSectionInited(false)
		{
			memset(&mState, 0, sizeof(ogg_sync_state));
		}
		~TheoraPlayback();

		int	Open(const std::string &fileName);
		void Close();
		ogg_int64_t ReadPage(ogg_page *page);
		bool ReadPacket(OggStream *stream, ogg_packet *packet);
		void ReadHeaders();
		void HandleTheoraData(OggStream *stream, ogg_packet *packet);
		bool IsPlaying();
		bool ConvertFrame();
		bool ConvertFrame(uint32_t width, uint32_t height, uint8_t *bufferPtr, uint32_t pitch);
		bool HandleTheoraHeader(OggStream* stream, ogg_packet* packet);
		bool HandleVorbisHeader(OggStream* stream, ogg_packet* packet);
		bool HandleSkeletonHeader(OggStream* stream, ogg_packet* packet);
};

#endif