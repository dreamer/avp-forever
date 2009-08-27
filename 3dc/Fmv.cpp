
#ifdef USE_FMV

#include "d3_func.h"
#include <fstream>
#include <map>
#include "ogg/ogg.h"
#include "theora/theora.h"
#include "theora/theoradec.h"
#include <process.h>

#include <d3dx9.h>

D3DTEXTURE BinkTexture;
bool binkTextureCreated = false;

enum StreamType
{
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

struct OggStream
{
	int mSerial;
	ogg_stream_state mState;
	StreamType mType;
	bool mHeadersRead;
	int mPacketCount;
	TheoraDecode mTheora;
};

typedef std::map<int, OggStream*> StreamMap;

StreamMap mStreams;

extern "C" {

#include "fmv.h"
#include "avp_userprofile.h"
#include "inline.h"
#include <math.h>
#include <assert.h>

extern	D3DINFO d3d;

extern int GotAnyKey;

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

extern unsigned char DebouncedGotAnyKey;

bool FmvWait();
int NextFMVFrame();
int FmvOpen(char *filenamePtr);
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);

int CloseTheoraVideo();
void HandleTheoraHeader(OggStream* stream, ogg_packet* packet);

std::ifstream oggFile;
ogg_sync_state state;
char buf[100];
bool frameReady = false;
bool fmvPlaying = false;
int playing = false;

#ifdef WIN32
	#include <shlobj.h>
	#include <shlwapi.h>
#endif

th_ycbcr_buffer buffer;

LPDIRECT3DSURFACE9 mSurface;
LPDIRECT3DSURFACE9 mDestSurface;
LPDIRECT3DTEXTURE9 mDisplayTexture;

CRITICAL_SECTION CriticalSection;

int imageNum2 = 0;


void HandleTheoraData(OggStream* stream, ogg_packet* packet)
{
	ogg_int64_t granulepos = -1;

	int ret = th_decode_packetin(stream->mTheora.mCtx,
			packet,
			&granulepos);

	if (ret == TH_DUPFRAME) // same as previous frame, don't bother decoding it
		return;

	assert (ret == 0);

	// We have a frame. Get the YUV data
//	th_ycbcr_buffer buffer;
	ret = th_decode_ycbcr_out(stream->mTheora.mCtx, buffer);
	assert(ret == 0);

	frameReady = true;

//	OutputDebugString("handling some theora data...\n");

	// Sleep for the time period of 1 frame
	float framerate = float(stream->mTheora.mInfo.fps_numerator) /
		float(stream->mTheora.mInfo.fps_denominator);

	Sleep((1.0/framerate)*1000);
}

void HandleOggPacket(OggStream* stream, ogg_packet* packet)
{
	if (stream->mHeadersRead && stream->mType == TYPE_THEORA)
		HandleTheoraData(stream, packet);

	/* if we haven't already read the theora headers, do so now */
	if (!stream->mHeadersRead && (stream->mType == TYPE_THEORA || stream->mType == TYPE_UNKNOWN))
		HandleTheoraHeader(stream, packet);
}

void HandleTheoraHeader(OggStream* stream, ogg_packet* packet)
{
	/* try to decode a header packet */
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

	OutputDebugString("got theora header\n");

	assert(ret == 0);

	// This is not a header packet. It is the first
	// video data packet.
	stream->mTheora.mCtx = th_decode_alloc(&stream->mTheora.mInfo, stream->mTheora.mSetup);
	assert(stream->mTheora.mCtx != NULL);
	stream->mHeadersRead = true;
	HandleTheoraData(stream, packet);
}

bool ReadOggPage(ogg_sync_state* state, ogg_page* page)
{
	if (!oggFile.good())
//		return false;
		return ogg_sync_pageout(state, page) == 1;

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
			// End of file
			return false;
		}
		// Update the synchronisation layer with the number
		// of bytes written to the buffer
		int ret = ogg_sync_wrote(state, bytes);
		assert(ret == 0);
	}
	return true;
}

void TheoraDecodeThread(void *args)
{
	int ret = 0;
	ogg_page page; // store a complete page of ogg data in page

	fmvPlaying = true;

	while (ReadOggPage(&state, &page))
	{
		/* grab the serial number of the page */
		int serial = ogg_page_serialno(&page);

		/* new pointer to OggStream struct */
		OggStream* stream = 0;

		/* check if this is the first page */
		if (ogg_page_bos(&page))
		{
			// At the beginning of the stream, read headers
			// Initialize the stream, giving it the serial
			// number of the stream for this page.
			stream = new OggStream;
			stream->mSerial = serial;
			stream->mPacketCount = 0;
			stream->mHeadersRead = false;
			stream->mType = TYPE_UNKNOWN;

			/* init the theora decode stuff */
			th_info_init(&stream->mTheora.mInfo);
			th_comment_init(&stream->mTheora.mComment);
			stream->mTheora.mCtx = 0;
			stream->mTheora.mSetup = 0;

			/* init ogg_stream_state struct and allocates appropriate memory in preparation for encoding or decoding.*/
			ret = ogg_stream_init(&stream->mState, serial);
			assert(ret == 0);
			mStreams[serial] = stream; // store our inited stream struct in the std::map
		}
		assert(mStreams.find(serial) != mStreams.end());
		stream = mStreams[serial]; // grab an existing stream object?

		/* Add a complete page to the bitstream */
		ret = ogg_stream_pagein(&stream->mState, &page);
		assert(ret == 0);

		// Return a complete packet of data from the stream, if we have one
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
			OutputDebugString("There is a gap in the data - we are out of sync\n");
			break;
		}

		// A packet is available, this is what we pass to the vorbis or theora libraries to decode.
		stream->mPacketCount++;
		HandleOggPacket(stream, &packet);
//		sprintf(buf, "mPacketCount: %d\n", stream->mPacketCount);
//		OutputDebugString(buf);

//		assert (stream->mPacketCount > 0);
	}
	fmvPlaying = false;
}

int OpenTheoraVideo(char *fileName)
{
	oggFile.open(fileName, std::ios::in | std::ios::binary);

	/* find the first page of ogg data */
//	ogg_sync_state state;

	/* initialise our state struct in preparation for getting some data */
	int ret = ogg_sync_init(&state);

	assert(ret == 0);

	if (!oggFile.is_open())
		return 0;

	if (!InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x80000400) )
		return 0;

	 _beginthread(TheoraDecodeThread, 0, 0);

#if 0
	ogg_page page; // store a complete page of ogg data in page
	while (ReadOggPage(&state, &page))
	{
		/* grab the serial number of the page */
		int serial = ogg_page_serialno(&page);

		/* new pointer to OggStream struct */
		OggStream* stream = 0;

		/* check if this is the first page */
		if (ogg_page_bos(&page))
		{
			// At the beginning of the stream, read headers
			// Initialize the stream, giving it the serial
			// number of the stream for this page.
			stream = new OggStream;//(serial);
			stream->mSerial = serial;
			stream->mPacketCount = 0;
			stream->mHeadersRead = false;
			stream->mType = TYPE_UNKNOWN;

			/* init the theora decode stuff */
			th_info_init(&stream->mTheora.mInfo);
			th_comment_init(&stream->mTheora.mComment);
			stream->mTheora.mCtx = 0;
			stream->mTheora.mSetup = 0;

			/* init ogg_stream_state struct and allocates appropriate memory in preparation for encoding or decoding.*/
			ret = ogg_stream_init(&stream->mState, serial);
			assert(ret == 0);
			mStreams[serial] = stream; // store our inited stream struct in the std::map
		}
		assert(mStreams.find(serial) != mStreams.end());
		stream = mStreams[serial]; // grab an existing stream object?

		/* Add a complete page to the bitstream */
		ret = ogg_stream_pagein(&stream->mState, &page);
		assert(ret == 0);

		// Return a complete packet of data from the stream, if we have one
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
			OutputDebugString("There is a gap in the data - we are out of sync\n");
			break;
		}

		// A packet is available, this is what we pass to the vorbis or theora libraries to decode.
		stream->mPacketCount++;
		HandleOggPacket(stream, &packet);
//		sprintf(buf, "mPacketCount: %d\n", stream->mPacketCount);
//		OutputDebugString(buf);

//		assert (stream->mPacketCount > 0);
	}
#endif

	// Cleanup
/*
	ret = ogg_sync_clear(&state);
	assert(ret == 0);

	CloseTheoraVideo();
*/
	return 0;
}

int CloseTheoraVideo()
{
	int ret = ogg_sync_clear(&state);
	assert(ret == 0);

	oggFile.close();

	for (StreamMap::iterator it = mStreams.begin(); it != mStreams.end(); ++it)
	{
		OggStream* stream = (*it).second;
//		char buf[100];
		sprintf(buf, "%d has %d packets\n", stream->mSerial, stream->mPacketCount);
		OutputDebugString(buf);
		//cout << stream->mSerial << " has " << stream->mPacketCount << " packets" << endl;
		//delete stream; we have no class so no destructor. do it manually
		int ret = ogg_stream_clear(&stream->mState);
		assert(ret == 0);
		th_setup_free(stream->mTheora.mSetup);
		th_decode_free(stream->mTheora.mCtx);
		delete stream;
    }

	return 0;
}

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
	if (!IntroOutroMoviesAreActive)
		return;

	if (OpenTheoraVideo(filenamePtr) != 0)
		return;

	playing = 1;

	while (playing)
	{
		CheckForWindowsMessages();

		if (frameReady)
			playing = NextFMVFrame();

		ThisFramesRenderingHasBegun();
		ClearScreenToBlack();

			int imageWidth = 640;
			int imageHeight = 360;
			DrawBinkFmv(0, 0, imageWidth, imageHeight, mDisplayTexture);

		ThisFramesRenderingHasFinished();
		FlipBuffers();

		DirectReadKeyboard();

		/* added DebouncedGotAnyKey to ensure previous frame's key press for starting level doesn't count */
		if (GotAnyKey && DebouncedGotAnyKey)
			playing = 0;
	}
}

int NextFMVFrame()
{
	if (!mSurface)
	{
		if (FAILED(d3d.lpD3DDevice->CreateOffscreenPlainSurface(buffer[0].width, buffer[0].height,
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
		if (FAILED(d3d.lpD3DDevice->CreateOffscreenPlainSurface(buffer[0].width, buffer[0].height,
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
		if (FAILED(d3d.lpD3DDevice->CreateTexture(buffer[0].width, buffer[0].height,
				1,
				D3DUSAGE_DYNAMIC,
				D3DFMT_X8R8G8B8,
				D3DPOOL_DEFAULT,
				&mDisplayTexture,
				NULL)))
		{
			OutputDebugString("can't create FMV texture\n");
		}
//		BinkTexture = mDisplayTexture;
//		frameHeight = buffer[0].height;
//		frameWidth = buffer[0].width;
	}

	EnterCriticalSection(&CriticalSection);

	D3DLOCKED_RECT surfaceLock;
	if (FAILED(mSurface->LockRect(&surfaceLock, NULL, D3DLOCK_DISCARD)))
	{
		OutputDebugString("can't lock FMV surface\n");
	}

#if 0
	sprintf(buf2, "dest pitch: %d\n", surfaceLock.Pitch);
	OutputDebugString(buf2);

	sprintf(buf2, "src u pitch: %d\n", buffer[1].stride);
	OutputDebugString(buf2);

	sprintf(buf2, "src v pitch: %d\n", buffer[2].stride);
	OutputDebugString(buf2);
#endif

	unsigned char *destPtr, *srcPtr;

	unsigned char* uPtr = buffer[1].data;
	unsigned char* vPtr = buffer[2].data;

	/*
	D3DFMT_YUY2
	YUY2 format (PC98 compliance). Two pixels are stored in each YUY2 word. Each data value (Y0, U, Y1, V) is 8 bits.
	This format is identical to the UYVY format described except that the ordering of the bits is changed. Before
	this format can be used, it must be enabled using the render state D3DRS_YUVENABLE.

	YUY2 = MEDIASUBTYPE_YUY2	YUY2	4:2:2	Packed	8bits per channel

	th_info

	4:2:0 planar to 4:2:2 packed

*/

	/* bjd - convert from 4:2:0 planar to 4:2:2 packed. had help from http://goldfishforthought.blogspot.com/2009/05/dealing-with-image-formats.html :) */
	bool bMustIncrementUVPlanes = false;

	for (int y = 0; y < buffer[0].height; y++)
	{
		destPtr = (((unsigned char *)surfaceLock.pBits) + y * surfaceLock.Pitch);
		srcPtr = (((unsigned char*)buffer[0].data) + y * buffer[0].stride);

		// Y U Y V
		for (int x = 0; x < (buffer[0].width / 2); x++)
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
			uPtr += buffer[1].stride - buffer[1].width;
			vPtr += buffer[2].stride - buffer[2].width;
            bMustIncrementUVPlanes = false;
        }
        else
        {
			/* go back to the start of the row */
			uPtr -= buffer[1].width;
			vPtr -= buffer[2].width;
            bMustIncrementUVPlanes = true;
        }
	}

	if (FAILED(mSurface->UnlockRect()))
	{
		OutputDebugString("can't unlock FMV surface\n");
	}

	LeaveCriticalSection(&CriticalSection);

	if (FAILED(d3d.lpD3DDevice->StretchRect(mSurface, NULL, mDestSurface, NULL, D3DTEXF_NONE)))
	{
		OutputDebugString("stretchRect failed\n");
	}

//	D3DLOCKED_RECT surfaceLock;
	if (FAILED(mDestSurface->LockRect(&surfaceLock, NULL, 0)))
	{
		OutputDebugString("can't lockrect mDestSurface\n");
		return 0;
	}

	D3DLOCKED_RECT textureLock;
	if (FAILED(mDisplayTexture->LockRect(0, &textureLock, NULL, NULL )))
	{
		OutputDebugString("can't lockrect mDisplayTexture\n");
		return 0;
	}

	srcPtr = static_cast<unsigned char*>(surfaceLock.pBits);
	destPtr = static_cast<unsigned char*>(textureLock.pBits);

	for (int y = 0; y < buffer[0].height; y++)
	{
		destPtr = (((unsigned char *)textureLock.pBits) + y*textureLock.Pitch);
		srcPtr = (((unsigned char *)surfaceLock.pBits) + y*surfaceLock.Pitch);

		for (int x = 0; x < buffer[0].width; x++)
		{
			memcpy(destPtr, srcPtr, 4);
			destPtr += 4;
			srcPtr += 4;
		}
	}

	mDisplayTexture->UnlockRect(0);
	mDestSurface->UnlockRect();


#if 0 // save frames to png files
	char strPath[MAX_PATH];
	char buf[100];

	/* finds the path to the folder. On Win7, this would be "C:\Users\<username>\AppData\Local\ as an example */
	if( FAILED(SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath ) ) )
	{
		return 0;
	}

	PathAppend( strPath, TEXT( "Fox\\Aliens versus Predator\\" ) );
	sprintf(buf, "image_%d.png", imageNum2);
	PathAppend( strPath, buf);

	/* save surface to image file */
	if (FAILED(D3DXSaveTextureToFile(strPath, D3DXIFF_PNG, mDisplayTexture, NULL)))
	{
		//LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("Save Surface to file failed!!!\n");
	}

	imageNum2++;
#endif

//	OutputDebugString("NextFMVFrame processed a frame\n");

	frameReady = false;

	if (fmvPlaying == false)
		return 0;

	return 1;
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

/* not needed */
void CloseFMV()
{
}

/* call this for res change, alt tabbing and whatnot */
extern void ReleaseBinkTextures()
{
	//ReleaseD3DTexture(&binkTexture);
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

}; // extern C

#endif

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
