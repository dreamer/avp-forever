#ifndef _fmvPlayback_h_
#define _fmvPlayback_h_

#include "d3_func.h"
#include <fstream>
#include <map>
#include <string>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <vorbis/vorbisfile.h>
#include "ringbuffer.h"
#include "audioStreaming.h"

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
class OggStream;

typedef std::map<int, OggStream*> streamMap;

class TheoraFMV
{
	public:
		std::string 	mFileName;
		std::ifstream 	mFileStream;

		// so we can easily reference these from the threads.
		OggStream *mVideo;
		OggStream *mAudio;

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

		bool mFmvPlaying;
		bool mFrameReady;
		bool mAudioStarted;

		TheoraFMV() :
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
		~TheoraFMV();

		int	Open(const std::string &fileName);
		void Close();
		bool ReadPage(ogg_page *page);
		bool ReadPacket(OggStream *stream, ogg_packet *packet);
		void ReadHeaders();
		void HandleTheoraData(OggStream *stream, ogg_packet *packet);
		bool IsPlaying();
		bool NextFrame();
};

#endif