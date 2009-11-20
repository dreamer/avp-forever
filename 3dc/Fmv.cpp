
//#ifdef USE_FMV
#if 1

#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include "d3_func.h"
#include <fstream>
#include <map>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <vorbis/vorbisfile.h>
#include <process.h>
#ifdef _XBOX
#include <d3dx8.h>
#include <xtl.h>
#define D3DLOCK_DISCARD 0
#else
#include <d3dx9.h>
#endif

#include "logString.h"
#include "vorbisPlayer.h"

#ifdef WIN32
//#include <sndfile.h>
//SNDFILE *sndFile;
#endif

#include "ringbuffer.h"
#include "audioStreaming.h"
#define restrict
#include <emmintrin.h>

enum StreamType
{
	TYPE_VORBIS,
	TYPE_THEORA,
	TYPE_UNKNOWN
};

struct TheoraDecode
{
	th_info		mInfo;
	th_comment	mComment;
	th_setup_info *mSetup;
	th_dec_ctx	*mCtx;
};

struct VorbisDecode
{
	vorbis_info mInfo;
	vorbis_comment mComment;
	vorbis_dsp_state mDsp;
	vorbis_block mBlock;
};

struct OggStream
{
	int mSerial;
	ogg_stream_state mState;
	StreamType mType;
	bool mActive;
	TheoraDecode mTheora;
	VorbisDecode mVorbis;
};

typedef std::map<int, OggStream*> StreamMap;

StreamMap mStreams;
ogg_int64_t  mGranulepos;

#define CLAMP(v)    ((v) > 255 ? 255 : (v) < 0 ? 0 : (v))

enum
{
	PLAYONCE,
	PLAYLOOP
};

extern "C" {

#include "fmv.h"
#include "avp_userprofile.h"
#include "inline.h"
#include <math.h>
#include <assert.h>
#include "showcmds.h"

extern unsigned char GotAnyKey;
extern int NumActiveBlocks;
extern DISPLAYBLOCK *ActiveBlockList[];
extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);
extern IMAGEHEADER ImageHeaderArray[];
#if MaxImageGroups>1
	extern int NumImagesArray[];
#else
	extern int NumImages;
#endif

int FmvSoundVolume = 65536/512;
int MoviesAreActive;
int IntroOutroMoviesAreActive = 1;
int VolumeOfNearestVideoScreen = 0;
int PanningOfNearestVideoScreen = 0;

#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];
int NumberOfFMVTextures;

int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;

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

extern unsigned char DebouncedGotAnyKey;

int NextFMVFrame();
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);
int CloseTheoraVideo();
bool HandleTheoraHeader(OggStream *stream, ogg_packet *packet);
bool HandleVorbisHeader(OggStream *stream, ogg_packet *packet);
bool ReadOggPage(ogg_sync_state *state, ogg_page *page);
bool ReadPacket(ogg_sync_state *state, OggStream *stream, ogg_packet *packet);
int CreateFMVAudioBuffer(int channels, int rate);
int GetWritableBufferSize();
int WriteToDsound(int dataSize);
void oggplay_yuv2rgb(OggPlayYUVChannels * yuv, OggPlayRGBChannels * rgb);

std::ifstream oggFile;
ogg_sync_state state;
char buf[100];
bool frameReady = false;
bool fmvPlaying = false;
bool MenuBackground = false;
bool started = false;
bool loop = false;

long postHeaderFileOffset = 0;
int textureWidth = 0;
int textureHeight = 0;
int frameWidth = 0;
int frameHeight = 0;

int playing = 0;

th_ycbcr_buffer buffer;

HANDLE callbackEvent;

D3DTEXTURE mDisplayTexture = NULL;
byte *textureData = NULL;

HRESULT LastError;

HANDLE decodeThreadHandle = NULL;
HANDLE audioThreadHandle = NULL;
CRITICAL_SECTION frameCriticalSection;
CRITICAL_SECTION audioCriticalSection;

OggStream *video = NULL;
OggStream *audio = NULL;

StreamingAudioBuffer *fmvAudioStream;

/* audio buffer for liboggplay */
static short	*audioDataBuffer = NULL;
static int		audioDataBufferSize = 0;
byte *audioData = NULL;

VorbisCodec *menuMusic = NULL;

void TheoraInitForData(OggStream* stream) 
{
	OutputDebugString("TheoraInitForData\n");
	stream->mTheora.mCtx = th_decode_alloc(&stream->mTheora.mInfo, stream->mTheora.mSetup);

	assert(stream->mTheora.mCtx != NULL);
	int ppmax = 0;

	int ret = th_decode_ctl(stream->mTheora.mCtx,
			  TH_DECCTL_GET_PPLEVEL_MAX,
			  &ppmax,
			  sizeof(ppmax));

	assert(ret == 0);

	// Set to a value between 0 and ppmax inclusive to experiment with
	// this parameter.
	ppmax = 0;
	ret = th_decode_ctl(stream->mTheora.mCtx,
		      TH_DECCTL_SET_PPLEVEL,
		      &ppmax,
		      sizeof(ppmax));
	
	assert(ret == 0);
}

void VorbisInitForData(OggStream* stream) 
{
	OutputDebugString("VorbisInitForData\n");

	int ret = vorbis_synthesis_init(&stream->mVorbis.mDsp, &stream->mVorbis.mInfo);
	assert(ret == 0);

	ret = vorbis_block_init(&stream->mVorbis.mDsp, &stream->mVorbis.mBlock);
	assert(ret == 0);
}

void HandleTheoraData(OggStream* stream, ogg_packet* packet)
{
	int ret = th_decode_packetin(stream->mTheora.mCtx,
			packet,
			&mGranulepos);

	if (ret == TH_DUPFRAME) // same as previous frame, don't bother decoding it
		return;

	assert (ret == 0);

	/* enter critical section so we can update buffer variable */
	EnterCriticalSection(&frameCriticalSection);

	// We have a frame. Get the YUV data
	ret = th_decode_ycbcr_out(stream->mTheora.mCtx, buffer);
	assert(ret == 0);

	/* we have a new frame and we're ready to use it */
	frameReady = true;

	LeaveCriticalSection(&frameCriticalSection);
}

bool ReadPacket(ogg_sync_state* state, OggStream* stream, ogg_packet* packet)
{
	int ret = 0;

	if (stream == NULL) 
		return false;

	while ((ret = ogg_stream_packetout(&stream->mState, packet)) != 1) 
	{
		ogg_page page;
		if (!ReadOggPage(state, &page))
		{
			fmvPlaying = false;
			return false;
		}

		int serial = ogg_page_serialno(&page);
		assert(mStreams.find(serial) != mStreams.end());
		OggStream* pageStream = mStreams[serial];

		// Drop data for streams we're not interested in.
		if (stream->mActive) 
		{
			ret = ogg_stream_pagein(&pageStream->mState, &page);
			assert(ret == 0);
		}
	}
	return true;
}

void ReadHeaders(ogg_sync_state* state) 
{
	ogg_page page;

	bool headersDone = false;
	
	while (!headersDone && ReadOggPage(state, &page)) 
	{
		int ret = 0;
		int serial = ogg_page_serialno(&page);
		OggStream* stream = 0;

		if (ogg_page_bos(&page)) 
		{
			// At the beginning of the stream, read headers
			// Initialize the stream, giving it the serial
			// number of the stream for this page.
			stream = new OggStream;
			stream->mSerial = serial;
			stream->mType = TYPE_UNKNOWN;

			/* init the theora decode stuff */
			th_info_init(&stream->mTheora.mInfo);
			th_comment_init(&stream->mTheora.mComment);
			stream->mTheora.mCtx = 0;
			stream->mTheora.mSetup = 0;

			/* init vorbis stuff */
			vorbis_info_init(&stream->mVorbis.mInfo);
			vorbis_comment_init(&stream->mVorbis.mComment);  

			ret = ogg_stream_init(&stream->mState, serial);
			assert(ret == 0);
			mStreams[serial] = stream;
		}

		assert(mStreams.find(serial) != mStreams.end());
		stream = mStreams[serial];

		// Add a complete page to the bitstream
		ret = ogg_stream_pagein(&stream->mState, &page);
		assert(ret == 0);

		// Process all available header packets in the stream. When we hit
		// the first data stream we don't decode it, instead we
		// return. The caller can then choose to process whatever data
		// streams it wants to deal with.
		
		ogg_packet packet;
		while (!headersDone && (ret = ogg_stream_packetpeek(&stream->mState, &packet)) != 0)
		{
			assert(ret == 1);

			// A packet is available. If it is not a header packet we exit.
			// If it is a header packet, process it as normal.
			headersDone = headersDone || HandleTheoraHeader(stream, &packet);
			headersDone = headersDone || HandleVorbisHeader(stream, &packet);
			if (!headersDone) 
			{
				// Consume the packet
				ret = ogg_stream_packetout(&stream->mState, &packet);
				assert(ret == 1);
			}
			else
			{
				OutputDebugString("headers done!\n");
			}
		}
	} 
	// store this so we can loop videos
	postHeaderFileOffset = oggFile.tellg();
	sprintf(buf, "headers end at offset %d\n", postHeaderFileOffset);
	OutputDebugString(buf);
}

bool HandleVorbisHeader(OggStream* stream, ogg_packet* packet)
{
	int ret = vorbis_synthesis_headerin(&stream->mVorbis.mInfo, &stream->mVorbis.mComment, packet);

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

bool HandleTheoraHeader(OggStream* stream, ogg_packet* packet)
{
	int ret = th_decode_headerin(&stream->mTheora.mInfo,
			   &stream->mTheora.mComment,
			   &stream->mTheora.mSetup,
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

bool ReadOggPage(ogg_sync_state* state, ogg_page* page)
{
	if (!oggFile.good())
	{
		OutputDebugString("file isnt good!\n");
		return ogg_sync_pageout(state, page) == 1;
	}

	while (ogg_sync_pageout(state, page) != 1)
	{
		// Returns a buffer that can be written to
		// with the given size. This buffer is stored
		// in the ogg synchronisation structure.
		char* buffer = ogg_sync_buffer(state, 4096);
		assert(buffer);

		// Read from the file into the buffer
		oggFile.read(buffer, 4096);
		int bytes = oggFile.gcount();
		if (bytes == 0)
		{
			if (loop)
			{
				oggFile.seekg(postHeaderFileOffset , std::ios_base::beg);
			}
			else
//			OutputDebugString("EOF\n");
			// End of file
			//continue;
			break;
		}
		// Update the synchronisation layer with the number
		// of bytes written to the buffer
		int ret = ogg_sync_wrote(state, bytes);
		assert(ret == 0);
	}
	return true;
}

void AudioGrabThread(void *args)
{

#ifdef USE_XAUDIO2
	CoInitializeEx( NULL, COINIT_MULTITHREADED );
#endif

	DWORD dwQuantum = 1000 / 60;

	static int totalRead = 0;
	static int lastRead = 0;

	int startTime = 0;
	int endTime = 0;
	int timetoSleep = 0;

	while (fmvPlaying)
	{
		startTime = timeGetTime();

		int numBuffersFree = AudioStream_GetNumFreeBuffers(fmvAudioStream);

		while (numBuffersFree)
		{
			int readableAudio = RingBuffer_GetReadableSpace();

			// we can fill a buffer
			if (readableAudio >= fmvAudioStream->bufferSize)
			{
				RingBuffer_ReadData(audioData, fmvAudioStream->bufferSize);
				AudioStream_WriteData(fmvAudioStream, audioData, fmvAudioStream->bufferSize);

//					sprintf(buf, "send %d bytes to xaudio2\n", fmvAudioStream.bufferSize);
//					OutputDebugString(buf);

				numBuffersFree--;
			}
			else
			{
				// not enough audio available this time
				break;
			}
		}

		if (started == false)
		{
			AudioStream_PlayBuffer(fmvAudioStream);
			started = true;
		}

//		if (numBuffersFree == 1)
//			SetEvent(callbackEvent);

		endTime = timeGetTime();

		timetoSleep = endTime - startTime;

		Sleep(dwQuantum - timetoSleep);
	}
	SetEvent(audioThreadHandle);
}

void TheoraDecodeThread(void *args)
{
	int ret = 0;
	int audioSize = 0;
	int dataSize = 0;
	float** pcm = 0;
	int samples = 0;
	ogg_packet packet;

	if (!audio)
	{
		// we have no audio
		ogg_packet packet;
		while (ReadPacket(&state, video, &packet)) 
		{
			if (!fmvPlaying) 
				break;

			HandleTheoraData(video, &packet);

			float framerate = float(video->mTheora.mInfo.fps_numerator) / 
				float(video->mTheora.mInfo.fps_denominator);

				Sleep(static_cast<DWORD>((1.0f / framerate) * 1000));
		}
	}
	else
	{

		// Read audio packets, sending audio data to the sound hardware.
		// When it's time to display a frame, decode the frame and display it.
		while (ReadPacket(&state, audio, &packet)) 
		{
			if (!fmvPlaying) 
				break;

			if (vorbis_synthesis(&audio->mVorbis.mBlock, &packet) == 0) 
			{
				// copy data from packet into vorbis objects for decoding
				ret = vorbis_synthesis_blockin(&audio->mVorbis.mDsp, &audio->mVorbis.mBlock);
				assert(ret == 0);
			}

			// get pointer to array of floating point values for sound samples
			while ((samples = vorbis_synthesis_pcmout(&audio->mVorbis.mDsp, &pcm)) > 0) 
			{
				if (samples > 0) 
				{
					dataSize = samples * audio->mVorbis.mInfo.channels;

					if (audioDataBuffer == NULL)
					{	
						// make it twice the size we currently need to avoid future reallocations
						audioDataBuffer = new short[dataSize * 2];
						audioDataBufferSize = dataSize * 2;
					}

					if (audioDataBufferSize < dataSize)
					{
						if (audioDataBuffer != NULL)
						{
							delete []audioDataBuffer;
							OutputDebugString("deleted audioDataBuffer to resize\n");
						}

						audioDataBuffer = new short[dataSize * 2];
						OutputDebugString("alloc audioDataBuffer\n");

						if (!audioDataBuffer) 
						{
							OutputDebugString("Out of memory\n");
							break;
						}
						audioDataBufferSize = dataSize * 2;
					}

					short* p = audioDataBuffer;

					for (int i = 0; i < samples; ++i) 
					{
						for (int j = 0; j < audio->mVorbis.mInfo.channels; ++j) 
						{
							int v = static_cast<int>(floorf(0.5f + pcm[j][i]*32767.0f));
							if (v > 32767) v = 32767;
							if (v < -32768) v = -32768;
							*p++ = v;
						}
					}

					audioSize = samples * audio->mVorbis.mInfo.channels * sizeof(short);
	/*
					while (!(AudioStream_GetNumFreeBuffers(&fmvAudioStream)))
						WaitForSingleObject(callbackEvent, INFINITE);
	*/
					EnterCriticalSection(&audioCriticalSection);

					int freeSpace = RingBuffer_GetWritableSpace();

					// Sleep here if we cant fill the ring buffer?
	#if 1
					// if we can't fit all our data..
					if (audioSize > freeSpace) 
					{
						while (audioSize > RingBuffer_GetWritableSpace())
						{
							Sleep(16);
						}
						//char buf[100];
						//sprintf(buf, "not enough room for all data. need %d, have free %d\n", audioSize, freeSpace);
						//OutputDebugString(buf);
					}
	#endif
					RingBuffer_WriteData((byte*)&audioDataBuffer[0], audioSize);

	//					sprintf(buf, "send %d bytes to ring buffer\n", audioSize);
	//					OutputDebugString(buf);

					LeaveCriticalSection(&audioCriticalSection);
				}

				// tell vorbis how many samples we consumed
				ret = vorbis_synthesis_read(&audio->mVorbis.mDsp, samples);
				assert(ret == 0);

				// At this point we've written some audio data to the sound
				// system. Now we check to see if it's time to display a video
				// frame.
				//
				// The granule position of a video frame represents the time
				// that that frame should be displayed up to. So we get the
				// current time, compare it to the last granule position read.
				// If the time is greater than that it's time to display a new
				// video frame.
				//
				// The time is obtained from the audio system - this represents
				// the time of the audio data that the user is currently
				// listening to. In this way the video frame should be synced up
				// to the audio the user is hearing.
				//

				if (video) 
				{
					float audio_time = static_cast<float>(AudioStream_GetNumSamplesPlayed(fmvAudioStream)) / static_cast<float>(audio->mVorbis.mInfo.rate);

					float video_time = static_cast<float>(th_granule_time(video->mTheora.mCtx, mGranulepos));

					// if audio is ahead of frame time, display a new frame
					if ((audio_time > video_time))
					{
						// Decode one frame and display it. If no frame is available we
						// don't do anything.
						ogg_packet packet;
						if (ReadPacket(&state, video, &packet)) 
						{
							HandleTheoraData(video, &packet);
						}
					}
				}
			}
		}
	}

	/* signal that our thread is finished, so we can close */
	SetEvent(decodeThreadHandle);
}

int OpenTheoraVideo(const char *fileName, int playMode = PLAYONCE)
{	
	if (fmvPlaying)
		return -1;

	std::string filePath;
	loop = false;

	if (playMode == PLAYONCE)
		loop = true;

#ifdef _XBOX
	filePath += "D:\\";
#endif
	filePath += fileName;

	oggFile.open(filePath.c_str()/*"D://Development//experiments//Debug//big_buck_bunny_480p_stereo.ogv"*/, std::ios::in | std::ios::binary);

	if (!oggFile.is_open())
	{
		OutputDebugString("can't find FMV file..\n");
		return 0;
	}

	/* initialise our state struct in preparation for getting some data */
	int ret = ogg_sync_init(&state);
	assert(ret == 0);

	// Read headers for all streams
	ReadHeaders(&state);

	for (StreamMap::iterator it = mStreams.begin(); it != mStreams.end(); ++it) 
	{
		OggStream* stream = (*it).second;
		if (!video && stream->mType == TYPE_THEORA) 
		{
			video = stream;
			TheoraInitForData(video);
		}
		else if (!audio && stream->mType == TYPE_VORBIS) 
		{
			audio = stream;
			VorbisInitForData(audio);
		}
		else 
			stream->mActive = false;
	}

	if (audio)
	{
		sprintf(buf, "Audio stream is %d %d channels %d KHz\n", audio->mSerial, 
								audio->mVorbis.mInfo.channels,
								audio->mVorbis.mInfo.rate);
		OutputDebugString(buf);

		/* init audio buffer here */
//		if (AudioStream_CreateBuffer(&fmvAudioStream, audio->mVorbis.mInfo.channels, audio->mVorbis.mInfo.rate, 4096, 3) != AUDIOSTREAM_OK)

		fmvAudioStream = AudioStream_CreateBuffer(audio->mVorbis.mInfo.channels, audio->mVorbis.mInfo.rate, 4096, 3);
		if (fmvAudioStream == NULL)
		{
			LogErrorString("Can't create audio stream buffer for OGG Vorbis!");
		}

		/* init some temp audio data storage */
		audioData = new byte[fmvAudioStream->bufferSize];

		RingBuffer_Init(fmvAudioStream->bufferSize * fmvAudioStream->bufferCount);
	}

	if (video) 
	{
		sprintf(buf, "Video stream is %d %dx%d\n", video->mSerial, 
								video->mTheora.mInfo.frame_width,
								video->mTheora.mInfo.frame_height);
		OutputDebugString(buf);
	}

	if (!mDisplayTexture)
	{
		/* assume we'll want our texture the same size as the frame. this isn't likely to be the case though */
		frameWidth = video->mTheora.mInfo.frame_width;
		frameHeight = video->mTheora.mInfo.frame_height;

		textureWidth = frameWidth;
		textureHeight = frameHeight;

		/* create a new texture, passing width and height which will be set to the actual size of the created texture */
		mDisplayTexture = CreateFmvTexture(&textureWidth, &textureHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
		if (!mDisplayTexture)
		{
			OutputDebugString("can't create FMV texture\n");
			return 0;
		}
		else OutputDebugString("created FMV texture OK\n");
	}

#ifdef WIN32
	if (!InitializeCriticalSectionAndSpinCount(&frameCriticalSection, 0x80000400))
	{
		OutputDebugString("can't create frameCriticalSection\n");
		return -1;
	}

	if (!InitializeCriticalSectionAndSpinCount(&audioCriticalSection, 0x80000400))
	{
		OutputDebugString("can't create audioCriticalSection\n");
		return -1;
	}
#endif
#ifdef _XBOX
	InitializeCriticalSection(&CriticalSection);
	InitializeCriticalSection(&audioCriticalSection);
#endif
	
	fmvPlaying = true;
	frameReady = false;
	started = false;
	
	decodeThreadHandle = CreateEvent( NULL, FALSE, FALSE, "decodeThreadHandle" );
	_beginthread(TheoraDecodeThread, 0, 0);

	if (audio)
	{
		audioThreadHandle = CreateEvent( NULL, FALSE, FALSE, "audioThreadHandle" );
		_beginthread(AudioGrabThread, 0, 0);
	}

	OutputDebugString("fmv should be playing now.\n");

	return 0;
}

bool CheckTheoraPlayback()
{
	return fmvPlaying;
}

int CloseTheoraVideo()
{
	assert (fmvPlaying == false); // this should be false if we reach here

	OutputDebugString("CloseTheoraVideo..\n");

	/* wait until both threads have finished running before continuing */ 
	WaitForMultipleObjects(1, &decodeThreadHandle, TRUE, INFINITE);
	WaitForMultipleObjects(1, &audioThreadHandle, TRUE, INFINITE);

	CloseHandle(decodeThreadHandle);
	CloseHandle(audioThreadHandle);

	int ret = ogg_sync_clear(&state);
	assert(ret == 0);

	oggFile.close();
	oggFile.clear();

	for (StreamMap::iterator it = mStreams.begin(); it != mStreams.end(); ++it)
	{
		OggStream *stream = (*it).second;

		// delete stream; we have no class so no destructor. do it manually.
		int ret = ogg_stream_clear(&stream->mState);
		assert(ret == 0);
		th_setup_free(stream->mTheora.mSetup);
		th_decode_free(stream->mTheora.mCtx);
		delete stream;
		stream = NULL;
    }

	// clear the std::map
	mStreams.clear();

	audio = NULL;
	video = NULL;

	if (fmvAudioStream)
	{
		AudioStream_ReleaseBuffer(fmvAudioStream);
		fmvAudioStream = NULL;
	}

	if (audioData)
	{
		delete []audioData;
		audioData = NULL;
	}

	if (audioDataBuffer)
	{
		delete []audioDataBuffer;
		audioDataBuffer = NULL;
		audioDataBufferSize = 0;
	}

#ifdef USE_LIBSNDFILE
	sf_close(sndFile);
#endif

	RingBuffer_Unload();

	frameWidth = 0;
	frameHeight = 0;
	textureWidth = 0;
	textureHeight = 0;

	if (mDisplayTexture)
	{
		mDisplayTexture->Release();
		mDisplayTexture = NULL;
	}

	DeleteCriticalSection(&frameCriticalSection);
	DeleteCriticalSection(&audioCriticalSection);

	return 0;
}

extern void StartMenuBackgroundFmv()
{
	const char *filenamePtr = "fmvs\\menubackground.ogv";

	OpenTheoraVideo(filenamePtr, PLAYLOOP);

	MenuBackground = true;
}

extern int PlayMenuBackgroundFmv()
{
	if (!MenuBackground)
		return 0;

	int playing = 0;

	if (frameReady)
	{
		playing = NextFMVFrame();
		DrawFmvFrame(frameWidth, frameHeight, textureWidth, textureHeight, mDisplayTexture);
	}

	return 1;
}

extern void EndMenuBackgroundFmv()
{
	if (!MenuBackground) 
		return;

	playing = 0;
	fmvPlaying = false;

	CloseTheoraVideo();

	MenuBackground = false;
}

extern void PlayFMV(const char *filenamePtr)
{
	if (!IntroOutroMoviesAreActive)
		return;

	if (OpenTheoraVideo(filenamePtr) != 0)
		return;

	playing = 1;

	while (playing)
	{
//		OutputDebugString("while (playing)...\n");
		CheckForWindowsMessages();

		if (!CheckTheoraPlayback())
			playing = 0;

		if (frameReady)
			playing = NextFMVFrame();

		ThisFramesRenderingHasBegun();
		ClearScreenToBlack();

		//if ((frameWidth != 0) && (frameHeight != 0)) // don't draw if we don't know frame width or height
		if (mDisplayTexture)
		{
//			OutputDebugString("got new texture..\n");
			DrawFmvFrame(frameWidth, frameHeight, textureWidth, textureHeight, mDisplayTexture);
		}

		ThisFramesRenderingHasFinished();
		FlipBuffers();

		DirectReadKeyboard();

		/* added DebouncedGotAnyKey to ensure previous frame's key press for starting level doesn't count */
		if (GotAnyKey && DebouncedGotAnyKey)
		{
			playing = 0;
			fmvPlaying = false;
		}
	}

	CloseTheoraVideo();
}

int NextFMVFrame()
{
	if (fmvPlaying == false)
		return 0;

	EnterCriticalSection(&frameCriticalSection);

	D3DLOCKED_RECT textureLock;
	if (FAILED(mDisplayTexture->LockRect(0, &textureLock, NULL, D3DLOCK_DISCARD)))
	{
		OutputDebugString("can't lock FMV texture\n");
		return 0;
	}

	OggPlayYUVChannels oggYuv;
	oggYuv.ptry = buffer[0].data;
	oggYuv.ptru = buffer[2].data;
	oggYuv.ptrv = buffer[1].data;

	oggYuv.y_width = buffer[0].width;
	oggYuv.y_height = buffer[0].height;
	oggYuv.y_pitch = buffer[0].stride;

	oggYuv.uv_width = buffer[1].width;
	oggYuv.uv_height = buffer[1].height;
	oggYuv.uv_pitch = buffer[1].stride;

	OggPlayRGBChannels oggRgb;
	oggRgb.ptro = static_cast<unsigned char*>(textureLock.pBits);
	oggRgb.rgb_height = frameHeight;
	oggRgb.rgb_width = frameWidth;
	oggRgb.rgb_pitch = textureLock.Pitch;

	oggplay_yuv2rgb(&oggYuv, &oggRgb);

	LeaveCriticalSection(&frameCriticalSection);

	if (FAILED(mDisplayTexture->UnlockRect(0)))
	{
		OutputDebugString("can't unlock FMV texture\n");
		return 0;
	}

	frameReady = false;

	return 1;
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

void FmvClose()
{
}

void UpdateAllFMVTextures()
{
	extern void UpdateFMVTexture(FMVTEXTURE *ftPtr);
	int i = NumberOfFMVTextures;

	while(i--)
	{
		UpdateFMVTexture(&FMVTexture[i]);
	}
}

extern void StartTriggerPlotFMV(int number)
{
	OutputDebugString("StartTriggerPlotFMV\n");

	int i = NumberOfFMVTextures;
	char buffer[25];

	if (CheatMode_Active != CHEATMODE_NONACTIVE) 
		return;

	sprintf(buffer, "FMVs//message%d.ogv", number);
	{
/*
		FILE* file = avp_fopen(buffer,"rb");
		if (!file)
		{
			OutputDebugString("couldn't open fmv file: ");
			OutputDebugString(buffer);
			return;
		}
		fclose(file);
*/
	}
	while(i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			if (OpenTheoraVideo(buffer) < 0)
			{
				return;
			}

			if (FMVTexture[i].SmackHandle)
			{
				delete FMVTexture[i].SmackHandle;
				FMVTexture[i].SmackHandle = NULL;
			}

			Smack *smackHandle = new Smack;
			OutputDebugString("alloc smack\n");

			OutputDebugString("opening..");
			OutputDebugString(buffer);

			smackHandle->FrameNum = 0;
			smackHandle->Frames = 0;
			smackHandle->isValidFile = 1;

			playing = 1;

			FMVTexture[i].SmackHandle = smackHandle;
			FMVTexture[i].MessageNumber = number;

			/* we need somewhere to store temp image data */
			if (textureData == NULL) 
			{
				textureData = new byte[128 * 128 * 4];
			}
		}
	}
}

extern void StartFMVAtFrame(int number, int frame)
{
}

/* bjd - called during each level load */
void ScanImagesForFMVs()
{
	extern void SetupFMVTexture(FMVTEXTURE *ftPtr);
	int i;
	IMAGEHEADER *ihPtr;
	NumberOfFMVTextures = 0;

	#if MaxImageGroups>1
	for (j=0; j<MaxImageGroups; j++)
	{
		if (NumImagesArray[j])
		{
			ihPtr = &ImageHeaderArray[j*MaxImages];
			for (i = 0; i<NumImagesArray[j]; i++, ihPtr++)
			{
	#else
	{
		if(NumImages)
		{
			ihPtr = &ImageHeaderArray[0];
			for (i = 0; i<NumImages; i++, ihPtr++)
			{
	#endif
				char *strPtr;
				if(strPtr = strstr(ihPtr->ImageName,"FMVs"))
				{
					Smack *smackHandle = NULL;
					char filename[30];
					{
						char *filenamePtr = filename;
						do
						{
							*filenamePtr++ = *strPtr;
						}
						while(*strPtr++!='.');

						*filenamePtr++='o';
						*filenamePtr++='g';
						*filenamePtr++='v';
						*filenamePtr=0;
					}

					/* do a check here to see if it's a theora file rather than just any old file with the right name? */
					FILE* file = avp_fopen(filename, "rb");
					if (!file)
					{
						OutputDebugString("cant find smacker file!\n");
						OutputDebugString(filename);
					}
					else 
					{	
						smackHandle = new Smack;
						OutputDebugString("alloc smack\n");
						OutputDebugString("found smacker file!");
						OutputDebugString(filename);
						OutputDebugString("\n");
						smackHandle->isValidFile = 1;
						fclose(file);
					}

					if (smackHandle)
					{
						FMVTexture[NumberOfFMVTextures].IsTriggeredPlotFMV = 0;
					}
					else
					{
						FMVTexture[NumberOfFMVTextures].IsTriggeredPlotFMV = 1;
					}

					{
						FMVTexture[NumberOfFMVTextures].SmackHandle = smackHandle;
						FMVTexture[NumberOfFMVTextures].ImagePtr = ihPtr;
						FMVTexture[NumberOfFMVTextures].StaticImageDrawn = 0;
						SetupFMVTexture(&FMVTexture[NumberOfFMVTextures]);
						NumberOfFMVTextures++;
						OutputDebugString("NumberOfFMVTextures++;\n");
					}
				}
			}
		}
	}
}

/* called when player quits level and returns to main menu */
void ReleaseAllFMVTextures()
{
	OutputDebugString("exiting level..closing FMV system\n");
	FmvClose();

	for (int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;

		if (FMVTexture[i].RGBBuffer)
		{
			delete []FMVTexture[i].RGBBuffer;
			FMVTexture[i].RGBBuffer = NULL;
		}
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}
	NumberOfFMVTextures = 0;
}

void ReleaseAllFMVTexturesForDeviceReset()
{
	for (int i = 0; i < NumberOfFMVTextures; i++)
	{
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}

	// non ingame fmv?
	if (mDisplayTexture)
	{
		mDisplayTexture->Release();
		mDisplayTexture = NULL;
	}
}

void RecreateAllFMVTexturesAfterDeviceReset()
{
	for (int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].ImagePtr->Direct3DTexture = CreateFmvTexture(&FMVTexture[i].ImagePtr->ImageWidth, &FMVTexture[i].ImagePtr->ImageHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
	}

	// non ingame fmv? - use a better way to determine this..
	if (textureWidth && textureHeight)
	{
		mDisplayTexture = CreateFmvTexture(&textureWidth, &textureHeight, D3DUSAGE_DYNAMIC, D3DPOOL_DEFAULT);
	}
}

// bjd - the below three functions could maybe be moved out of this file altogether as vorbisPlayer can handle it
void StartMenuMusic()
{
	// we need to load IntroSound.ogg here using vorbisPlayer
	menuMusic = Vorbis_LoadFile("fmvs//IntroSound.ogg");
}

void PlayMenuMusic()
{
	// we can probably just leave this blank as a thread will handle the updates
}

void EndMenuMusic()
{
	Vorbis_Release(menuMusic);
	menuMusic = NULL;
}

extern void InitialiseTriggeredFMVs()
{
	int i = NumberOfFMVTextures;
	while(i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			if (FMVTexture[i].SmackHandle)
			{
				delete FMVTexture[i].SmackHandle;
				FMVTexture[i].SmackHandle = NULL;
				FMVTexture[i].MessageNumber = 0;
			}
		}
	}
}

extern void GetFMVInformation(int *messageNumberPtr, int *frameNumberPtr)
{
	int i = NumberOfFMVTextures;

	while(i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			if(FMVTexture[i].SmackHandle)
			{
				*messageNumberPtr = FMVTexture[i].MessageNumber;
				*frameNumberPtr = 0;
				return;
			}
		}
	}

	*messageNumberPtr = 0;
	*frameNumberPtr = 0;
}

/* not needed */
void CloseFMV()
{
}

void FindLightingValuesFromTriggeredFMV(unsigned char *bufferPtr, FMVTEXTURE *ftPtr)
{
	unsigned int totalRed=0;
	unsigned int totalBlue=0;
	unsigned int totalGreen=0;

	FmvColourRed = totalRed/48*16;
	FmvColourGreen = totalGreen/48*16;
	FmvColourBlue = totalBlue/48*16;
}

int NextFMVTextureFrame(FMVTEXTURE *ftPtr)
{
	int smackerFormat = 1;

	int w = ftPtr->ImagePtr->ImageWidth;
	int h = ftPtr->ImagePtr->ImageHeight;

	unsigned char *DestBufferPtr = ftPtr->RGBBuffer;

	if (MoviesAreActive && ftPtr->SmackHandle)
	{
		assert(textureData != NULL);

		int volume = MUL_FIXED(FmvSoundVolume*256, GetVolumeOfNearestVideoScreen());

// FIXME		FmvVolumePan(volume, PanningOfNearestVideoScreen);

		ftPtr->SoundVolume = FmvSoundVolume;

		if (!frameReady) 
			return 0;

		memcpy(DestBufferPtr, textureData, 128*128*4);

		if (playing == 0)
		{
			if (ftPtr->SmackHandle)
			{
				OutputDebugString("closing ingame fmv..\n");
				FmvClose();

				delete ftPtr->SmackHandle;
				ftPtr->SmackHandle = NULL;
			}
			
			ftPtr->MessageNumber = 0;
		}

		ftPtr->StaticImageDrawn=0;
	}
	else if (!ftPtr->StaticImageDrawn || smackerFormat)
	{
		int i = w * h;
		unsigned int seed = FastRandom();
		int *ptr = (int*)DestBufferPtr;
		do
		{
			seed = ((seed * 1664525) + 1013904223);
			*ptr++ = seed;
		}
		while(--i);
		ftPtr->StaticImageDrawn = 1;
	}
	FindLightingValuesFromTriggeredFMV((unsigned char*)ftPtr->RGBBuffer, ftPtr);
	return 1;
}

int GetVolumeOfNearestVideoScreen(void)
{
	extern VIEWDESCRIPTORBLOCK *Global_VDB_Ptr;
	int numberOfObjects = NumActiveBlocks;
	int leastDistanceRecorded = 0x7fffffff;
	VolumeOfNearestVideoScreen = 0;

	{
		extern char LevelName[];
		if (!_stricmp(LevelName,"invasion_a"))
		{
			VolumeOfNearestVideoScreen = ONE_FIXED;
			PanningOfNearestVideoScreen = ONE_FIXED/2;
		}
	}

	while (numberOfObjects)
	{
		DISPLAYBLOCK* objectPtr = ActiveBlockList[--numberOfObjects];
		STRATEGYBLOCK* sbPtr = objectPtr->ObStrategyBlock;

		if (sbPtr)
		{
			if (sbPtr->I_SBtype == I_BehaviourVideoScreen)
			{
				int dist;
				VECTORCH disp;

				disp.vx = objectPtr->ObWorld.vx - Global_VDB_Ptr->VDB_World.vx;
				disp.vy = objectPtr->ObWorld.vy - Global_VDB_Ptr->VDB_World.vy;
				disp.vz = objectPtr->ObWorld.vz - Global_VDB_Ptr->VDB_World.vz;

				dist = Approximate3dMagnitude(&disp);
				if (dist<leastDistanceRecorded && dist<ONE_FIXED)
				{
					leastDistanceRecorded = dist;
					VolumeOfNearestVideoScreen = ONE_FIXED + 1024 - dist/2;
					if (VolumeOfNearestVideoScreen>ONE_FIXED) VolumeOfNearestVideoScreen = ONE_FIXED;
					{
						VECTORCH rightEarDirection;

						rightEarDirection.vx = Global_VDB_Ptr->VDB_Mat.mat11;
						rightEarDirection.vy = 0;
						rightEarDirection.vz = Global_VDB_Ptr->VDB_Mat.mat31;
						disp.vy=0;
						Normalise(&disp);
						Normalise(&rightEarDirection);

						PanningOfNearestVideoScreen = 32768 + DotProduct(&disp,&rightEarDirection)/2;
					}
				}
			}
		}
	}
//	PrintDebuggingText("Volume: %d, Pan %d\n",VolumeOfNearestVideoScreen,PanningOfNearestVideoScreen);
	return VolumeOfNearestVideoScreen;
}

}; // extern C

#endif

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