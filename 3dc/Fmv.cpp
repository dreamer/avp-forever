
#ifdef USE_FMV

#include "d3_func.h"
#include <assert.h>
#include <map>
#include <iostream>
#include <fstream>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>
#include <process.h>

#ifdef WIN32
	#include <shlobj.h>
	#include <shlwapi.h>
#endif


#include "d3dx9.h"

extern "C" {
extern D3DINFO d3d;
};

D3DTEXTURE BinkTexture;
bool binkTextureCreated = false;

bool isPlaying = false;
bool frameReady = false;
bool playing = false;

bool mAudio = false; //fix

enum StreamType {
	TYPE_VORBIS,
	TYPE_THEORA,
	TYPE_UNKNOWN
};

class OggStream;

class TheoraDecode {
public:
  th_info mInfo;
  th_comment mComment;
  th_setup_info *mSetup;
  th_dec_ctx* mCtx;

public:
  TheoraDecode() :
    mSetup(0),
    mCtx(0)
  {
    th_info_init(&mInfo);
    th_comment_init(&mComment);
  }

  void initForData(OggStream* stream);

  ~TheoraDecode() {
    th_setup_free(mSetup);
    th_decode_free(mCtx);
  }   
};

class VorbisDecode {
public:
  vorbis_info mInfo;
  vorbis_comment mComment;
  vorbis_dsp_state mDsp;
  vorbis_block mBlock;

public:
  VorbisDecode()
  {
    vorbis_info_init(&mInfo);
    vorbis_comment_init(&mComment);    
  }

  void initForData(OggStream* stream);
};

class OggStream
{
public:
  int mSerial;
  ogg_stream_state mState;
  StreamType mType;
  bool mActive;
  TheoraDecode mTheora;
  VorbisDecode mVorbis;

public:
  OggStream(int serial = -1) : 
    mSerial(serial),
    mType(TYPE_UNKNOWN),
    mActive(true)
  { 
  }

  ~OggStream() {
    int ret = ogg_stream_clear(&mState);
    assert(ret == 0);
  }
};

void TheoraDecode::initForData(OggStream* stream) {
	stream->mTheora.mCtx = 
	th_decode_alloc(&stream->mTheora.mInfo, 
			stream->mTheora.mSetup);
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

void VorbisDecode::initForData(OggStream* stream) 
{
	int ret = vorbis_synthesis_init(&stream->mVorbis.mDsp, &stream->mVorbis.mInfo);
	assert(ret == 0);
	ret = vorbis_block_init(&stream->mVorbis.mDsp, &stream->mVorbis.mBlock);
	assert(ret == 0);
}

typedef std::map<int, OggStream*> StreamMap;

std::istream *testStream;

class OggDecoder
{
public:
	StreamMap mStreams;  
	LPDIRECT3DSURFACE9 mSurface;
	LPDIRECT3DSURFACE9 mDestSurface; // needed?
	LPDIRECT3DTEXTURE9 mDisplayTexture;
	th_ycbcr_buffer YUVbuffer;
	bool fmvPlaying;
	int frameHeight;
	int frameWidth;
	CRITICAL_SECTION CriticalSection;
	struct threadParamsStruct
	{
		std::istream *stream;
		OggDecoder *decoder;
	};
	threadParamsStruct threadParams;
	ogg_int64_t  mGranulepos;

private:
	bool handle_theora_header(OggStream* stream, ogg_packet* packet);
	bool handle_vorbis_header(OggStream* stream, ogg_packet* packet);
	void read_headers(std::istream& stream, ogg_sync_state* state);

	bool read_page(std::istream& stream, ogg_sync_state* state, ogg_page* page);
	bool read_packet(std::istream& is, ogg_sync_state* state, OggStream* stream, ogg_packet* packet);
	void handle_theora_data(OggStream* stream, ogg_packet* packet);
	static unsigned int __stdcall decode_thread(void *arg);

public:
  OggDecoder() :
	mSurface(0),
	mDestSurface(0),
	mDisplayTexture(0)
  {
  }

	~OggDecoder() {
		if (mSurface)
		{
			mSurface->Release();
			mSurface = NULL;
		}
		if (mDestSurface)
		{
			mDestSurface->Release();
			mDestSurface = NULL;
		}
		if (mDisplayTexture)
		{
			mDisplayTexture->Release();
			mDisplayTexture = NULL;
		}
		OutputDebugString("destroying OggDecoder\n");
	}
	void play(std::istream& stream);
	int NextFMVFrame();
	int FmvOpen(char *filenamePtr);
};

std::ifstream file;
OggDecoder decoder;
int imageNum = 0;

int OggDecoder::NextFMVFrame()
{
	OutputDebugString("NextFMVFrame\n");
	if (!mSurface)
	{
		if (FAILED(d3d.lpD3DDevice->CreateOffscreenPlainSurface(frameWidth, frameHeight, 
				D3DFMT_YUY2,
				D3DPOOL_DEFAULT,
				&mSurface,
				NULL)))
		{
			OutputDebugString("can't create FMV surface\n");
		}
	}

	if (!mDestSurface)
	{
		if (FAILED(d3d.lpD3DDevice->CreateOffscreenPlainSurface(frameWidth, frameHeight, 
				D3DFMT_X8R8G8B8,
				D3DPOOL_DEFAULT,
				&mDestSurface,
				NULL)))
		{
			OutputDebugString("can't create FMV surface\n");
		}
	}

	/* TODO! create as power of 2 texture? */
	if (!mDisplayTexture)
	{
		if (FAILED(d3d.lpD3DDevice->CreateTexture(frameWidth, frameHeight, 
				1, 
				D3DUSAGE_DYNAMIC, 
				D3DFMT_X8R8G8B8,
				D3DPOOL_DEFAULT, 
				&mDisplayTexture, 
				NULL)))
		{
			OutputDebugString("can't create FMV texture\n");
		}
		BinkTexture = mDisplayTexture;
		mDisplayTexture->AddRef();
	}

	D3DLOCKED_RECT surfaceLock;
	if (FAILED(mSurface->LockRect(&surfaceLock, NULL, D3DLOCK_DISCARD)))
	{
		OutputDebugString("can't lock FMV surface\n");
	}

	EnterCriticalSection(&CriticalSection);

	unsigned char *destPtr, *srcPtr;

	unsigned char* uPtr = YUVbuffer[1].data;
	unsigned char* vPtr = YUVbuffer[2].data;

	bool bMustIncrementUVPlanes = false;

	for (int y = 0; y < frameHeight; y++)
	{
		destPtr = (((unsigned char *)surfaceLock.pBits) + y * surfaceLock.Pitch);
		srcPtr = (((unsigned char*)YUVbuffer[0].data) + y * YUVbuffer[0].stride);

		// Y U Y V
		for (int x = 0; x < (frameWidth / 2); x++)
		{
			// Y0
			*destPtr = *srcPtr;
			destPtr++;
			srcPtr++;

			// U
			*destPtr = *uPtr;
			destPtr++;
			uPtr++;

			// Y1
			*destPtr = *srcPtr;
			destPtr++;
			srcPtr++;

			// V
			*destPtr = *vPtr;
			destPtr++;
			vPtr++;
		}

        // Since YV12 has half the UV data that YUY2 has, we reuse these  
        // values--so we only increment these planes every other pass  
        // through.  
        if( bMustIncrementUVPlanes )  
        {  
			/* increment pointers down onto the next line, skipping any pitch/stride data */
			uPtr += YUVbuffer[1].stride - YUVbuffer[1].width;
			vPtr += YUVbuffer[2].stride - YUVbuffer[2].width;
            bMustIncrementUVPlanes = false;  
        }  
        else  
        {  
			/* go back to the start of the row */
			uPtr -= YUVbuffer[1].width;
			vPtr -= YUVbuffer[2].width;
            bMustIncrementUVPlanes = true;  
        }
	}

	LeaveCriticalSection(&CriticalSection);

	if (FAILED(mSurface->UnlockRect()))
	{
		OutputDebugString("can't unlock FMV surface\n");
	}


	if (FAILED(d3d.lpD3DDevice->StretchRect(mSurface, NULL, mDestSurface, NULL, D3DTEXF_NONE)))
	{
		OutputDebugString("stretchrect failed\n");
	}
	
	D3DLOCKED_RECT textureLock;
	if (FAILED(mDisplayTexture->LockRect(0, &textureLock, NULL, D3DLOCK_DISCARD)))
	{
		OutputDebugString("can't lock FMV texture\n");
	}
	
	D3DLOCKED_RECT surfaceLock2;
	if (FAILED(mDestSurface->LockRect(&surfaceLock2, NULL, D3DLOCK_READONLY)))
	{
		OutputDebugString("can't lock FMV surface\n");
	}

	/* FIXME - optimise.. */
	for (int y = 0; y < frameHeight; y++)
	{
		srcPtr = (((unsigned char *)surfaceLock2.pBits) + y * surfaceLock2.Pitch);
		destPtr = (((unsigned char *)textureLock.pBits) + y * textureLock.Pitch);

		for (int x = 0; x < frameWidth; x++)
		{
			memcpy(destPtr, srcPtr, frameWidth * 4);
		}
	}
	
	if (FAILED(mDisplayTexture->UnlockRect(0)))
	{
		OutputDebugString("can't unlock FMV texture\n");
	}

	if (FAILED(mDestSurface->UnlockRect()))
	{
		OutputDebugString("can't unlock FMV surface\n");
	}
#if 1 
	char strPath[MAX_PATH];
	char buf[100];

	/* finds the path to the folder. On Win7, this would be "C:\Users\<username>\AppData\Local\ as an example */
	if( FAILED(SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath ) ) )
	{
		return 0;
	}

	PathAppend( strPath, TEXT( "Fox\\Aliens versus Predator\\" ) );
	sprintf(buf, "image_%d.png", imageNum);
	PathAppend( strPath, buf);

	/* save surface to image file */
	if (FAILED(D3DXSaveTextureToFile(strPath, D3DXIFF_PNG, mDisplayTexture, 
		NULL))) 
	{
		//LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("Save texture to file failed!!!\n");
	}
#endif
	imageNum++;

	frameReady = false;

	return 1;
}

bool OggDecoder::read_page(std::istream& stream, ogg_sync_state* state, ogg_page* page) 
{
	int ret = 0;

	// If we've hit end of file we still need to continue processing
	// any remaining pages that we've got buffered.
	if (!stream.good())
		return ogg_sync_pageout(state, page) == 1;

	while ((ret = ogg_sync_pageout(state, page)) != 1) 
	{
		// Returns a buffer that can be written too
		// with the given size. This buffer is stored
		// in the ogg synchronisation structure.
		char* buffer = ogg_sync_buffer(state, 4096);
		assert(buffer);

		// Read from the file into the buffer
		stream.read(buffer, 4096);
		int bytes = stream.gcount();
		if (bytes == 0) 
		{
			// End of file. 
			continue;
		}

		// Update the synchronisation layer with the number
		// of bytes written to the buffer
		ret = ogg_sync_wrote(state, bytes);
		assert(ret == 0);
	}
	return true;
}

bool OggDecoder::read_packet(std::istream& is, ogg_sync_state* state, OggStream* stream, ogg_packet* packet) 
{
	int ret = 0;

	while ((ret = ogg_stream_packetout(&stream->mState, packet)) != 1) 
	{
		ogg_page page;
		if (!read_page(is, state, &page))
			return false;

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

void OggDecoder::read_headers(std::istream& stream, ogg_sync_state* state) 
{
	ogg_page page;

	bool headersDone = false;
	while (!headersDone && read_page(stream, state, &page)) 
	{
		int ret = 0;
		int serial = ogg_page_serialno(&page);
		OggStream* stream = 0;

		if (ogg_page_bos(&page)) 
		{
			// At the beginning of the stream, read headers
			// Initialize the stream, giving it the serial
			// number of the stream for this page.
			stream = new OggStream(serial);
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
			headersDone = headersDone || handle_theora_header(stream, &packet);
			headersDone = headersDone || handle_vorbis_header(stream, &packet);
			if (!headersDone) 
			{
				// Consume the packet
				ret = ogg_stream_packetout(&stream->mState, &packet);
				assert(ret == 1);
			}
		}
	} 
}

void OggDecoder::play(std::istream& stream) 
{
	threadParams.stream = &stream;
	threadParams.decoder = this;

	testStream = &stream;

	if (!InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x80000400) )
	{
		return;
	}

	/* create decoding thread */
	_beginthreadex(NULL, 0, &OggDecoder::decode_thread, (void*)&threadParams, 0, NULL);

	fmvPlaying = true;
	isPlaying = true;
}

OggStream* video = 0;
OggStream* audio = 0;

unsigned int __stdcall OggDecoder::decode_thread(void *arg)
{
	ogg_sync_state state;

	threadParamsStruct *tempParams = static_cast<threadParamsStruct*>(arg);

	int ret = ogg_sync_init(&state);
	assert(ret == 0);

	// Read headers for all streams
	tempParams->decoder->read_headers(*tempParams->stream, &state);

	// Find and initialize the first theora and vorbis
	// streams. According to the Theora spec these can be considered the
	// 'primary' streams for playback.
	video = 0;
	audio = 0;

	double audioTime = timeGetTime();
	char buf[100];
	sprintf(buf, "start time: %f\n", audioTime);
	OutputDebugString(buf);

	for (StreamMap::iterator it = tempParams->decoder->mStreams.begin(); it != tempParams->decoder->mStreams.end(); ++it) 
	{
		OggStream* stream = (*it).second;
		if (!video && stream->mType == TYPE_THEORA) {
			video = stream;
			video->mTheora.initForData(video);
		}
		else if (!audio && stream->mType == TYPE_VORBIS) {
			audio = stream;
			audio->mVorbis.initForData(audio);
		}
		else stream->mActive = false;
	}

	assert(audio);

	if (video) {
		std::cout << "Video stream is " 
		<< video->mSerial << " "
		<< video->mTheora.mInfo.frame_width << "x" << video->mTheora.mInfo.frame_height
		<< std::endl;
	}

	std::cout << "Audio stream is " 
		<< audio->mSerial << " "
		<< audio->mVorbis.mInfo.channels << " channels "
		<< audio->mVorbis.mInfo.rate << "KHz"
		<< std::endl;
/*
  ret = sa_stream_create_pcm(&mAudio,
			     NULL,
			     SA_MODE_WRONLY,
			     SA_PCM_FORMAT_S16_NE,
			     audio->mVorbis.mInfo.rate,
			     audio->mVorbis.mInfo.channels);
  assert(ret == SA_SUCCESS);
	
  ret = sa_stream_open(mAudio);
  assert(ret == SA_SUCCESS);
*/
  // Read audio packets, sending audio data to the sound hardware.
  // When it's time to display a frame, decode the frame and display it.
	ogg_packet packet;
	while ((tempParams->decoder->read_packet(*tempParams->stream, &state, audio, &packet)) && tempParams->decoder->fmvPlaying)
	{
		if (vorbis_synthesis(&audio->mVorbis.mBlock, &packet) == 0) 
		{
			ret = vorbis_synthesis_blockin(&audio->mVorbis.mDsp, &audio->mVorbis.mBlock);
			assert(ret == 0);
		}

		float** pcm = 0;
		int samples = 0;

		while ((samples = vorbis_synthesis_pcmout(&audio->mVorbis.mDsp, &pcm)) > 0) 
		{
			if (mAudio) 
			{
				if (samples > 0) 
				{
					short *buffer = new short[samples * audio->mVorbis.mInfo.channels];
					//short buffer[samples * audio->mVorbis.mInfo.channels];
					short* p = buffer;
					for (int i = 0; i < samples; ++i) 
					{
						for (int j = 0; j < audio->mVorbis.mInfo.channels; ++j) 
						{
							int v = static_cast<int>(floorf(0.5 + pcm[j][i]*32767.0));
							if (v > 32767) v = 32767;
							if (v <-32768) v = -32768;
							*p++ = v;
						}
					}
	  
					//ret = sa_stream_write(mAudio, buffer, sizeof(buffer));
					//assert(ret == SA_SUCCESS);

					delete []buffer;
					buffer = NULL;
				}
	
				ret = vorbis_synthesis_read(&audio->mVorbis.mDsp, samples);
				assert(ret == 0);
			}

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
				ogg_int64_t position = 0;
				//int ret = sa_stream_get_position(mAudio, SA_POSITION_WRITE_SOFTWARE, &position);
				//assert(ret == SA_SUCCESS);

				/*
					double audio_time = 
						double(position) /
						double(audio->mVorbis.mInfo.rate) /
						double(audio->mVorbis.mInfo.channels) /
						sizeof(short);
				*/
				double audio_time = timeGetTime() - audioTime;
				double video_time = th_granule_time(video->mTheora.mCtx, tempParams->decoder->mGranulepos);
				audio_time = audio_time / 1000;
/*
				char buf[100];
				sprintf(buf, "audio_time: %f, video_time: %f\n", audio_time, video_time );
				OutputDebugString(buf);
*/
				if (audio_time > video_time) 
				{
					// Decode one frame and display it. If no frame is available we
					// don't do anything.
					ogg_packet packet;
					if (tempParams->decoder->read_packet(*tempParams->stream, &state, video, &packet)) 
					{
						tempParams->decoder->handle_theora_data(video, &packet);
						video_time = th_granule_time(video->mTheora.mCtx, tempParams->decoder->mGranulepos);
					}
				}
				else
				{
					Sleep(video_time - audio_time);
				}
			}
		}
	}

	// Cleanup
	ret = ogg_sync_clear(&state);
	assert(ret == 0);

	return 0;
}

bool OggDecoder::handle_theora_header(OggStream* stream, ogg_packet* packet) {
  int ret = th_decode_headerin(&stream->mTheora.mInfo,
			       &stream->mTheora.mComment,
			       &stream->mTheora.mSetup,
			       packet);

	if (ret == TH_ENOTFORMAT)
		return false; // Not a theora header

	if (ret > 0) {
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

void OggDecoder::handle_theora_data(OggStream* stream, ogg_packet* packet) 
{
	int ret = th_decode_packetin(stream->mTheora.mCtx, packet, &mGranulepos);

	assert(ret == 0 || ret == TH_DUPFRAME);

	OutputDebugString("handle_theora_data\n");
/*
	switch (stream->mTheora.mInfo.pixel_fmt)
	{
		case TH_PF_420:
			OutputDebugString("Format: TH_PF_420\n");
			break;
		case TH_PF_RSVD:
			OutputDebugString("Format: TH_PF_RSVD\n");
			break;
		case TH_PF_422:
			OutputDebugString("Format: TH_PF_422\n");
			break;
		case TH_PF_444:
			OutputDebugString("Format: TH_PF_444\n");
			break;
		default:
			OutputDebugString("Format: Unknown\n");
			break;
	}
*/

//	OutputDebugString("handle_theora_data\n");

	EnterCriticalSection(&CriticalSection); 

	// We have a frame. Get the YUV data
	ret = th_decode_ycbcr_out(stream->mTheora.mCtx, YUVbuffer);
	assert(ret == 0);

	frameHeight = YUVbuffer[0].height;
	frameWidth = YUVbuffer[0].width;

	frameReady = true;

	LeaveCriticalSection(&CriticalSection);
}

bool OggDecoder::handle_vorbis_header(OggStream* stream, ogg_packet* packet) 
{
	int ret = vorbis_synthesis_headerin(&stream->mVorbis.mInfo, &stream->mVorbis.mComment, packet);
	// Unlike libtheora, libvorbis does not provide a return value to
	// indicate that we've finished loading the headers and got the
	// first data packet. To detect this I check if I already know the
	// stream type and if the vorbis_synthesis_headerin call failed.
	if (stream->mType == TYPE_VORBIS && ret == OV_ENOTVORBIS) {
		// First data packet
		return true;
	}
	else if (ret == 0) {
		stream->mType = TYPE_VORBIS;
	}
	return false;
}

extern "C" {

#include "fmv.h"
#include "avp_userprofile.h"
#include "inline.h"
#include <math.h>

extern int GotAnyKey;
extern unsigned char DebouncedGotAnyKey;

int SmackerSoundVolume = 65536/512;
int MoviesAreActive;
int IntroOutroMoviesAreActive = 1;
int VolumeOfNearestVideoScreen = 0;
int PanningOfNearestVideoScreen = 0;

#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES];
int NumberOfFMVTextures;

extern IMAGEHEADER ImageHeaderArray[];
#if MaxImageGroups>1
	extern int NumImagesArray[];
#else
	extern int NumImages;
#endif

int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;

extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);

bool FmvWait();
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);

extern void EndMenuBackgroundBink()
{
}

extern void StartMenuBackgroundBink()
{
}

void UpdateFMVAudioBuffer(void *arg) 
{

}

void RecreateAllFMVTexturesAfterDeviceReset()
{

}

extern void PlayBinkedFMV(char *filenamePtr)
{
	decoder.FmvOpen(filenamePtr);

	return;
}

void FmvClose()
{
}

extern int PlayMenuBackgroundBink()
{
	return 0;
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
	return;
}

void StartMenuMusic()
{
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

	OutputDebugString("scan images for fmvs\n");

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
					FILE* file = fopen(filename, "rb");
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
					}
				}
			}
		}
	}
}

/* called when player quits level and returns to main menu */
void ReleaseAllFMVTextures()
{
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
		FMVTexture[i].MessageNumber = 0;

		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}

	SAFE_RELEASE(BinkTexture);
}

void PlayMenuMusic()
{
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

void EndMenuMusic()
{
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

void FindLightingValuesFromTriggeredFMV(unsigned char *bufferPtr, FMVTEXTURE *ftPtr)
{
	unsigned int totalRed=0;
	unsigned int totalBlue=0;
	unsigned int totalGreen=0;

	FmvColourRed = totalRed/48*16;
	FmvColourGreen = totalGreen/48*16;
	FmvColourBlue = totalBlue/48*16;
}

int NextFMVTextureFrame(FMVTEXTURE *ftPtr/*, void *bufferPtr, int pitch*/)
{
//	OutputDebugString("NextFMVTextureFrame\n");
	int smackerFormat = 1;
	int w = 128;
	int h = 96;
	unsigned char *bufferPtr = ftPtr->RGBBuffer;
	
	{
//		extern D3DINFO d3d;
//		smackerFormat = GetSmackerPixelFormat(&(d3d.TextureFormat[d3d.CurrentTextureFormat].ddsd.ddpfPixelFormat));
	}
//	if (smackerFormat) w*=2;

#if 0
	if (MoviesAreActive && ftPtr->SmackHandle)
	{
		int volume = MUL_FIXED(SmackerSoundVolume*256,GetVolumeOfNearestVideoScreen());
//		SmackVolumePan(ftPtr->SmackHandle,SMACKTRACKS,volume,PanningOfNearestVideoScreen);
		ftPtr->SoundVolume = SmackerSoundVolume;
	    
//	    if (SmackWait(ftPtr->SmackHandle)) return 0;
		if (FmvWait()) return 0;
		/* unpack frame */

		writeFmvData((unsigned char*)bufferPtr, textureData, w, h, pitch);

//		if (textureData == NULL) return -1;
//		bufferPtr = textureData;
//	  	SmackToBuffer(ftPtr->SmackHandle,0,0,w,96,bufferPtr,smackerFormat);

//		SmackDoFrame(ftPtr->SmackHandle);

		/* are we at the last frame yet? */
//		if (ftPtr->IsTriggeredPlotFMV && (ftPtr->SmackHandle->FrameNum==(ftPtr->SmackHandle->Frames-1)) )
		if (playing == 0)
		{
			if(ftPtr->SmackHandle)
			{
				OutputDebugString("closing ingame fmv..\n");
				FmvClose();

				delete ftPtr->SmackHandle;
				ftPtr->SmackHandle = NULL;
			}

//			SmackClose(ftPtr->SmackHandle);
			
			ftPtr->MessageNumber = 0;
		}
		else
		{
			/* next frame, please */
//			SmackNextFrame(ftPtr->SmackHandle);
		}
		ftPtr->StaticImageDrawn=0;
	}
#endif
	if(!ftPtr->StaticImageDrawn || smackerFormat)
	{
		int i = w * h;
		unsigned int seed = FastRandom();
		int *ptr = (int*)bufferPtr;
		do
		{
			seed = ((seed * 1664525) + 1013904223);
			*ptr++ = seed;
		}
		while(--i);
		ftPtr->StaticImageDrawn = 1;
	}
	FindLightingValuesFromTriggeredFMV((unsigned char*)bufferPtr, ftPtr);
	return 1;
}

extern int NumActiveBlocks;
extern DISPLAYBLOCK *ActiveBlockList[];
#include "showcmds.h"
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
						#if 0
						rightEarDirection.vx = Global_VDB_Ptr->VDB_Mat.mat11;
						rightEarDirection.vy = Global_VDB_Ptr->VDB_Mat.mat12;
						rightEarDirection.vz = Global_VDB_Ptr->VDB_Mat.mat13;
						Normalise(&disp);
						#else
						rightEarDirection.vx = Global_VDB_Ptr->VDB_Mat.mat11;
						rightEarDirection.vy = 0;
						rightEarDirection.vz = Global_VDB_Ptr->VDB_Mat.mat31;
						disp.vy=0;
						Normalise(&disp);
						Normalise(&rightEarDirection);
						#endif
						PanningOfNearestVideoScreen = 32768 + DotProduct(&disp,&rightEarDirection)/2;
					}
				}
			}
		}
	}
//	PrintDebuggingText("Volume: %d, Pan %d\n",VolumeOfNearestVideoScreen,PanningOfNearestVideoScreen);
	return VolumeOfNearestVideoScreen;
}

int OggDecoder::FmvOpen(char *filenamePtr)
{
	file.open(filenamePtr, std::ios::in | std::ios::binary);

	if (file) 
	{
		decoder.play(file);
/*
		file.close();
		for (StreamMap::iterator it = decoder.mStreams.begin(); it != decoder.mStreams.end(); ++it) 
		{
			OggStream* stream = (*it).second;
			delete stream;
		}
*/
	}

	while (isPlaying)
	{
		OutputDebugString("in while (isPlaying)\n");
		CheckForWindowsMessages();

		if (frameReady)
			/*playing = */NextFMVFrame();

		ThisFramesRenderingHasBegun();
		ClearScreenToBlack();

			DrawBinkFmv((640-frameWidth)/2, (480-frameHeight)/2, frameWidth, frameHeight, BinkTexture);

		ThisFramesRenderingHasFinished();
		FlipBuffers();

		DirectReadKeyboard();

		/* added DebouncedGotAnyKey to ensure previous frame's key press for starting level doesn't count */
		if (GotAnyKey && DebouncedGotAnyKey) 
			isPlaying = false;
	}

	return 1;
}

} // extern "C"

#endif