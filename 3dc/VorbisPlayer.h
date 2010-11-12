// Copyright (C) 2010 Barry Duncan. All Rights Reserved.
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

#ifndef _VorbisPlayer_h_
#define _VorbisPlayer_h_

#include "os_header.h"

#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include <vorbis/vorbisfile.h>
#include "audioStreaming.h"
#include <string>

class VorbisPlayback
{
	public:
		FILE *mVorbisFile;
		vorbis_info *mVorbisInfo;
		OggVorbis_File mOggFile;

		AudioStream *audioStream;

		bool mIsPlaying;

		uint32_t mBufferSize;
		uint32_t mHalfBufferSize;

		HANDLE mPlaybackThreadFinished;

		uint8_t *mAudioData;

		VorbisPlayback() :
			mVorbisFile(0),
			mVorbisInfo(0),
			audioStream(0),
			mIsPlaying(false),
			mBufferSize(0),
			mHalfBufferSize(0),
			mPlaybackThreadFinished(0),
			mAudioData(0)
		{
			memset(&mOggFile, 0, sizeof(OggVorbis_File));
		}
		~VorbisPlayback();

		bool Open(const std::string &fileName);
		void Stop();
		uint32_t GetVorbisData(uint32_t sizeToRead);
};

#ifdef __cplusplus
extern "C" {
#endif

void Vorbis_CloseSystem();
extern void LoadVorbisTrack(size_t track);
extern bool LoadVorbisTrackList();
bool IsVorbisPlaying();
size_t CheckNumberOfVorbisTracks();

#ifdef __cplusplus
}
#endif

#endif
