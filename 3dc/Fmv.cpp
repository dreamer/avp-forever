
#ifdef USE_FMV

#include "d3_func.h"
#include <assert.h>
#include <map>
#include <iostream>
#include <fstream>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
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

enum StreamType {
	TYPE_THEORA,
	TYPE_UNKNOWN
};

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

  ~TheoraDecode() {
    th_setup_free(mSetup);
    th_decode_free(mCtx);
  }   
};

class OggStream
{
public:
	int mSerial;
	ogg_stream_state mState;
	StreamType mType;
	bool mHeadersRead;
	TheoraDecode mTheora;

public:
	OggStream(int serial = -1) : 
		mSerial(serial),
		mType(TYPE_UNKNOWN),
		mHeadersRead(false)
	{
	}

  ~OggStream() {
    int ret = ogg_stream_clear(&mState);
    assert(ret == 0);
  }
};

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

private:
	bool read_page(std::istream& stream, ogg_sync_state* state, ogg_page* page);
	void handle_packet(OggStream* stream, ogg_packet* packet);
	void handle_theora_data(OggStream* stream, ogg_packet* packet);
	void handle_theora_header(OggStream* stream, ogg_packet* packet);
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
			*destPtr = *srcPtr;
			destPtr++;
			srcPtr++;
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
	if (FAILED(D3DXSaveSurfaceToFile(strPath, D3DXIFF_PNG, mDestSurface, NULL, NULL))) 
	{
		//LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("Save texture to file failed!!!\n");
	}

	imageNum++;

	frameReady = false;

	return 1;
}

bool OggDecoder::read_page(std::istream& stream, ogg_sync_state* state, ogg_page* page) 
{
	int ret = 0;
	if (!stream.good())
		return false;

	while (ogg_sync_pageout(state, page) != 1) 
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
			// End of file
			return false;
		}

		// Update the synchronisation layer with the number
		// of bytes written to the buffer
		ret = ogg_sync_wrote(state, bytes);
		assert(ret == 0);
	}
	return true;
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

unsigned int __stdcall OggDecoder::decode_thread(void *arg)
{
	ogg_sync_state state;
	ogg_page page;
	int packets = 0;

	threadParamsStruct *tempParams = static_cast<threadParamsStruct*>(arg);

	int ret = ogg_sync_init(&state);
	assert(ret == 0);
  
	while ((tempParams->decoder->read_page(*tempParams->stream, &state, &page)) && tempParams->decoder->fmvPlaying)
	{
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
			tempParams->decoder->mStreams[serial] = stream;
		}

		assert(tempParams->decoder->mStreams.find(serial) != tempParams->decoder->mStreams.end());
		stream = tempParams->decoder->mStreams[serial];

		// Add a complete page to the bitstream
		ret = ogg_stream_pagein(&stream->mState, &page);
		assert(ret == 0);

		// Return a complete packet of data from the stream
		ogg_packet packet;
		ret = ogg_stream_packetout(&stream->mState, &packet);

		if (ret == 0) 
		{
			// Need more data to be able to complete the packet
			continue;
		}
		else if (ret == -1) 
		{
			// We are out of sync and there is a gap in the data.
			// Exit
			std::cout << "There is a gap in the data - we are out of sync" << std::endl;
			break;
		}

		// A packet is available, this is what we pass to the vorbis or
		// theora libraries to decode.
		tempParams->decoder->handle_packet(stream, &packet);
	}	

	// Cleanup
	ret = ogg_sync_clear(&state);
	assert(ret == 0);

	return 0;
}

void OggDecoder::handle_packet(OggStream* stream, ogg_packet* packet) 
{
	if (stream->mHeadersRead && stream->mType == TYPE_THEORA)
		handle_theora_data(stream, packet);

	if (!stream->mHeadersRead && (stream->mType == TYPE_THEORA || stream->mType == TYPE_UNKNOWN))
		handle_theora_header(stream, packet);
}

void OggDecoder::handle_theora_header(OggStream* stream, ogg_packet* packet) 
{
	int ret = th_decode_headerin(&stream->mTheora.mInfo,
		&stream->mTheora.mComment,
		&stream->mTheora.mSetup,
		packet);

	if (ret == OC_NOTFORMAT)
		return; // Not a theora header

	if (ret > 0) 
	{
		// This is a theora header packet
		stream->mType = TYPE_THEORA;
		return;
	}

	assert(ret == 0);
	// This is not a header packet. It is the first 
	// video data packet.
	stream->mTheora.mCtx = th_decode_alloc(&stream->mTheora.mInfo, stream->mTheora.mSetup);
	assert(stream->mTheora.mCtx != NULL);
	stream->mHeadersRead = true;
	handle_theora_data(stream, packet);
}

void OggDecoder::handle_theora_data(OggStream* stream, ogg_packet* packet) 
{
	ogg_int64_t granulepos = -1;
	int ret = th_decode_packetin(stream->mTheora.mCtx,
				   packet,
				   &granulepos);

	assert(ret == 0);

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
//	th_ycbcr_buffer buffer;
	ret = th_decode_ycbcr_out(stream->mTheora.mCtx, YUVbuffer);
	assert(ret == 0);

	frameHeight = YUVbuffer[0].height;
	frameWidth = YUVbuffer[0].width;


	frameReady = true;

	LeaveCriticalSection(&CriticalSection);

  // Sleep for the time period of 1 frame
	float framerate = 
	float(stream->mTheora.mInfo.fps_numerator) / 
	float(stream->mTheora.mInfo.fps_denominator);
	Sleep((1.0/framerate)*1000);
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

void drive_decoding(void *arg);
void display_frame(void *arg);
bool FmvWait();
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);
void writeFmvData(unsigned char *destData, unsigned char* srcData, int width, int height, int pitch);
int CreateFMVAudioBuffer(int channels, int rate);
int WriteToDsound(int dataSize, int offset);
int GetWritableBufferSize();
int updateAudioBuffer(int numBytes, short *data);

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