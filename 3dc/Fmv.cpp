//#ifndef WIN32

extern "C" {

#include "d3_func.h"
#include "fmv.h"
#include "avp_userprofile.h"
#include "inline.h"
#include <math.h>

#include <assert.h>
#include "sndfile.h"

extern int GotAnyKey;

int SmackerSoundVolume = 65536/512; // 128
int MoviesAreActive;
int IntroOutroMoviesAreActive = 1;
int VolumeOfNearestVideoScreen = 0;
int PanningOfNearestVideoScreen = 0;

#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES] = {0};
int NumberOfFMVTextures;

extern HWND hWndMain;

extern IMAGEHEADER ImageHeaderArray[];
#if MaxImageGroups>1
	extern int NumImagesArray[];
#else
	extern int NumImages;
#endif

int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;

#include <oggplay/oggplay.h>
#pragma comment(lib, "liboggplay.lib")
#pragma comment(lib, "liboggz.lib")

/* for threads */
#include <process.h>

int decodingThreadHandle	= 0;
int frameThreadHandle		= 0;
int updateAudioThreadHandle = 0;

/* liboggplay */
static OggPlay			*player = NULL;
static OggPlayReader	*reader = NULL;

static int				video_track = 0;
static int				audio_track = 0;

static int				channels = 0;
static int				rate = 0;
static int				fps_num = 0;
static int				fps_denom = 0;
static int				n_frames = 0;

bool					fmvPlaying = false;
bool					frameReady = false;

extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);

void drive_decoding(void *arg);
void display_frame(void *arg);
void handle_video_data (OggPlay * player, int track_num, OggPlayVideoData * video_data, int frame);
bool FmvWait();
int NextFMVFrame();
int FmvOpen(char *filenamePtr);
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);
void writeFmvData(unsigned char *destData, unsigned char* srcData, int width, int height, int pitch);
int CreateFMVAudioBuffer(int channels, int rate);
void handle_audio_data (OggPlay * player, int track, OggPlayAudioData * data, int samples);
int WriteToDsound(int dataSize, int offset);
int GetWritableBufferSize();
int updateAudioBuffer(int numBytes, short *data);

unsigned char	*textureData = NULL;
int				imageWidth = 0;
int				imageHeight = 0;
int				textureWidth = 0;
int				textureHeight = 0;

int				playing = 0;

extern			D3DINFO d3d;
D3DTEXTURE		fmvTexture = NULL;
D3DTEXTURE		fmvDynamicTexture = NULL;

/* dsound stuff */
extern LPDIRECTSOUND	DSObject;
LPDIRECTSOUNDBUFFER		fmvAudioBuffer = NULL;
LPDIRECTSOUNDNOTIFY		fmvpDSNotify = NULL;
HANDLE					fmvhHandles[2];
LPVOID					fmvaudioPtr1;
DWORD					fmvaudioBytes1;
LPVOID					fmvaudioPtr2;   
DWORD					fmvaudioBytes2;
int						fullBufferSize = 0;
int						halfBufferSize = 0;

DWORD					bufferOffset = 0;
long					lastPlayCursor = 0;
long					totalBytesPlayed = 0;
long					totalBytesWritten = 0;


/* audio buffer for liboggplay */
static unsigned char	*audioDataBuffer = NULL;
static int				audioDataBufferSize = 0;

/* ring buffer stuff */
struct ringBuffer 
{
	int				bufferReadPos;
	int				bufferWritePos;
	long			bufferBytesPlayed;
	long			bufferBytesWritten;
	int				bufferFillSize;
	bool			bufferInitialFill;

	unsigned char	*buffer;
	int				bufferSize;
};
ringBuffer fmvRingBuffer = {0};

#define	USE_AUDIO		1
#define	WRITE_WAV		0
#define USE_ARGB		0

SNDFILE					*sndFile;

extern void EndMenuBackgroundBink()
{
}

extern void StartMenuBackgroundBink()
{
}

void UpdateFMVAudioBuffer(void *arg) 
{
	int lockOffset = 0;
	updateAudioThreadHandle = 1;

	while(fmvPlaying)
	{
#if 0
		int wait_value = WaitForMultipleObjects(2, fmvhHandles, FALSE, 1);

		if((wait_value != WAIT_TIMEOUT) && (wait_value != WAIT_FAILED))
		{
			if(wait_value == 0) 
			{
				OutputDebugString("lock first half\n");
				lockOffset = 0;
			}
			else 
			{
				OutputDebugString("lock second half\n");
				lockOffset = halfBufferSize;
			}

			WriteToDsound( fmvRingBuffer.bufferSize / 2, lockOffset );
		}
#endif
	}

	updateAudioThreadHandle = 0;
}

extern void PlayBinkedFMV(char *filenamePtr)
{
	if (!IntroOutroMoviesAreActive) return;

	/* try to open file - quit this function if we can't */
	if (FmvOpen(filenamePtr) != 0) return;

	/* create the d3d textures used to display the video */
	if(FAILED(d3d.lpD3DDevice->CreateTexture(1024, 1024, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &fmvTexture, NULL)))
	{
		OutputDebugString("problem creating fmv texture from d3d\n");
		FmvClose();
		return;
	}

	if(FAILED(d3d.lpD3DDevice->CreateTexture(1024, 1024, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, /*D3DPOOL_DEFAULT*/D3DPOOL_SYSTEMMEM, &fmvDynamicTexture, NULL)))
	{
		OutputDebugString("problem creating dynamic fmv texture from d3d\n");
		FmvClose();
		return;
	}

	playing = 1;

	/* start the playback loop. */
	while(playing == 1)
	{
		CheckForWindowsMessages();
//	    if (!BinkWait(binkHandle)) 
		if (!FmvWait())
			playing = NextFMVFrame();//NextBinkFrame(binkHandle);

		ThisFramesRenderingHasBegun();
		ClearScreenToBlack();

			DrawBinkFmv((640-imageWidth)/2, (480-imageHeight)/2, imageHeight, imageWidth, fmvTexture);

		ThisFramesRenderingHasFinished();
		FlipBuffers();

		DirectReadKeyboard();
		if (GotAnyKey) 
			playing = 0;
	}

	OutputDebugString("closing pre-game fmv..\n");
	FmvClose();
}

/* TODO: This needs to be actually fixed so it's not called if we aren't playing anything! */
void FmvClose()
{
	fmvPlaying = false;

	/* ......................... ahem */
	while ((frameThreadHandle != 0) || (decodingThreadHandle != 0) || (updateAudioThreadHandle != 0))
	{
		Sleep(2);
	}

	OutputDebugString("releasing..\n");

	/* release d3d textures */
	SAFE_RELEASE(fmvTexture);
	SAFE_RELEASE(fmvDynamicTexture);

	/* delete texture data buffer */
	if (textureData != NULL)
	{
		delete []textureData;
		textureData = NULL;
	}

	/* delete temp audio buffer */
	delete []audioDataBuffer;
	audioDataBuffer = NULL;
	audioDataBufferSize = 0;
	OutputDebugString("deleted audioDataBuffer\n");

	/* clear ringbuffer */
	delete []fmvRingBuffer.buffer;
	fmvRingBuffer.buffer = NULL;
	fmvRingBuffer.bufferBytesPlayed = 0;
	fmvRingBuffer.bufferBytesWritten = 0;
	fmvRingBuffer.bufferFillSize = 0;
	fmvRingBuffer.bufferInitialFill = false;
	fmvRingBuffer.bufferReadPos = 0;
	fmvRingBuffer.bufferSize = 0;
	fmvRingBuffer.bufferWritePos = 0;

	bufferOffset = 0;
	lastPlayCursor = 0;
	totalBytesPlayed = 0;
	totalBytesWritten = 0;

	/* stop and release dsound buffer */
	if(fmvAudioBuffer != NULL)
	{
		fmvAudioBuffer->Stop();
		OutputDebugString("releasing fmv dsound buffer...\n");
		SAFE_RELEASE(fmvAudioBuffer);
	}

//	oggplay_close(player);

	OutputDebugString("our reader is gone\n");
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
	OutputDebugString("StartTriggerPlotFMV\n");

	int i = NumberOfFMVTextures;
	char buffer[25];

	if (CheatMode_Active != CHEATMODE_NONACTIVE) return;

//	sprintf(buffer,"FMVs//message%d.smk",number);
	sprintf(buffer,"FMVs//message%d.ogv",number);
	{
		FILE* file=fopen(buffer,"rb");
		if(!file)
		{
			return;
		}
		fclose(file);
	}
	while(i--)
	{
		if (FMVTexture[i].IsTriggeredPlotFMV)
		{
			if(FMVTexture[i].SmackHandle)
			{
				delete FMVTexture[i].SmackHandle;
//				SmackClose(FMVTexture[i].SmackHandle);
			}

			Smack *smackHandle = new Smack;

			OutputDebugString("opening..");
			OutputDebugString(buffer);
			FmvOpen(buffer);

			playing = 1;

			FMVTexture[i].SmackHandle = smackHandle;

//			FMVTexture[i].SmackHandle = SmackOpen(&buffer,SMACKTRACKS|SMACKNEEDVOLUME|SMACKNEEDPAN,SMACKAUTOEXTRA);
			FMVTexture[i].MessageNumber = number;
		}
	}
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
	NumberOfFMVTextures=0;

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

//						*filenamePtr++='s';
//						*filenamePtr++='m';
//						*filenamePtr++='k';
						*filenamePtr++='o';
						*filenamePtr++='g';
						*filenamePtr++='v';
						*filenamePtr=0;
					}

					/* do check here to see if it's a theora file rather than just any old file with the right name? */
					FILE* file=fopen(filename,"rb");
					if(!file)
					{
						OutputDebugString("cant find smacker file!\n");
						OutputDebugString(filename);
					}
					else 
					{	
						smackHandle = new Smack;
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

void ReleaseAllFMVTextures()
{
	OutputDebugString("releasing fmv textures and also closing ingame fmv playback\n");
	FmvClose();

	for(int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;
//		ReleaseD3DTexture8(FMVTexture[i].SrcTexture);
//		ReleaseD3DTexture8(FMVTexture[i].SrcSurface);
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
		SAFE_RELEASE(FMVTexture[i].DestTexture);
	}
}

void ReleaseAllFMVTexturesForDeviceReset()
{
	for(int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;
//		ReleaseD3DTexture8(FMVTexture[i].SrcTexture);
//		ReleaseD3DTexture8(FMVTexture[i].SrcSurface);
		ReleaseD3DTexture8(FMVTexture[i].DestTexture);

		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);

//		ReleaseD3DTexture8(FMVTexture[i].ImagePtr->Direct3DTexture);
/*
		if(FMVTexture[i].ImagePtr->Direct3DTexture != NULL)
		{
			FMVTexture[i].ImagePtr->Direct3DTexture->Release();
			FMVTexture[i].ImagePtr->Direct3DTexture = NULL;
		}
*/
	}
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
			if(FMVTexture[i].SmackHandle)
			{
				delete FMVTexture[i].SmackHandle;
//				SmackClose(FMVTexture[i].SmackHandle);
				FMVTexture[i].MessageNumber = 0;
			}

			FMVTexture[i].SmackHandle = 0;
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
	//ReleaseD3DTexture8(&binkTexture);
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

int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr, int pitch)
{
//	OutputDebugString("NextFMVTextureFrame\n");
	int smackerFormat = 1;
	int w = 128;
	int h = 96;
	
	{
//		extern D3DINFO d3d;
//		smackerFormat = GetSmackerPixelFormat(&(d3d.TextureFormat[d3d.CurrentTextureFormat].ddsd.ddpfPixelFormat));
	}
//	if (smackerFormat) w*=2;

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

	else if(!ftPtr->StaticImageDrawn || smackerFormat)
	{
		int i = w*h;///2;
		unsigned int seed = FastRandom();
		int *ptr = (int*)bufferPtr;
		do
		{
			seed = ((seed*1664525)+1013904223);
			*ptr++ = seed;
		}
		while(--i);
		ftPtr->StaticImageDrawn=1;
	}
	FindLightingValuesFromTriggeredFMV((unsigned char*)bufferPtr,ftPtr);
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

int FmvOpen(char *filenamePtr)
{
	/* try to open our video file by filename*/
	reader = oggplay_file_reader_new(filenamePtr);

	player = oggplay_open_with_reader(reader);

	if (player == NULL) 
	{
		char message[100];
		sprintf(message,"Unable to access file: %s\n",filenamePtr);
#ifdef WIN32
		MessageBox(hWndMain, message, "AvP Error", MB_OK+MB_SYSTEMMODAL);
#endif
//		exit(0x111);
		return 1;
//		printf ("could not initialise oggplay with this file\n");
//		exit (1);
	}

	video_track = -1; 
	for (int i = 0; i < oggplay_get_num_tracks (player); i++) {
		printf("Track %d is of type %s\n", i, 
		oggplay_get_track_typename (player, i));
	
		if (oggplay_get_track_type (player, i) == OGGZ_CONTENT_THEORA) {
			int ret;
			oggplay_set_callback_num_frames (player, i, 1);
			video_track = i;
			ret = oggplay_get_video_fps(player, i , &fps_denom, &fps_num);
			printf("fps: %d\n", fps_num);
			printf("fps denom: %d\n", fps_denom);
		}
		else if (oggplay_get_track_type (player, i) == OGGZ_CONTENT_VORBIS
				||
				oggplay_get_track_type (player, i) == OGGZ_CONTENT_SPEEX) 
		{
			int ret;
			audio_track = i;      
			oggplay_set_offset(player, i, 300);
			ret = oggplay_get_audio_samplerate(player, i , &rate);
			ret = oggplay_get_audio_channels(player, i, &channels);
			printf("samplerate: %d channels: %d\n", rate, channels);

			/* fix this! */
//			if(channels == 1) audioFormat = AUDIO_S16SYS;
//			else			  audioFormat = AUDIO_S16SYS;
		}
		else if (oggplay_get_track_type (player, i) == OGGZ_CONTENT_KATE) {
			const char *category = "<unknown>", *language = "<unknown>";
			int ret = oggplay_get_kate_category(player, i, &category);
			ret = oggplay_get_kate_language(player, i, &language);
			printf("category %s, language %s\n", category, language);
		}

		if (oggplay_set_track_active(player, i) < 0) {
			//printf("\tNote: Could not set this track active!\n");
			OutputDebugString("\tNote: Could not set this track active!\n");
		}
	}

	if (video_track == -1) {
		if (audio_track >= 0) {
			oggplay_set_callback_num_frames(player, audio_track, 8192);
		}
	}

	oggplay_use_buffer(player, 20);

	fullBufferSize = CreateFMVAudioBuffer(channels, rate);
	halfBufferSize = fullBufferSize / 2;

//	OutputDebugString("should have created textures\n");

	fmvPlaying = true;

	/* create decoding threads? */
	_beginthread(drive_decoding, 0, 0);
	_beginthread(display_frame, 0, 0);
//	_beginthread(UpdateFMVAudioBuffer, 0, 0);

	return 0;
}

void drive_decoding(void *arg) 
{
	OggPlayErrorCode r;
	static int saved_avail = 0;

	decodingThreadHandle = 1;

	while (fmvPlaying) 
	{
//		int avail;
	
		/* don't care about seeking yet.. */

		r = E_OGGPLAY_TIMEOUT;
		while (r == E_OGGPLAY_TIMEOUT) 
		{
//			OutputDebugString("while (r == E_OGGPLAY_TIMEOUT) \n");
			r = oggplay_step_decoding(player);
		}

#if 0
		avail = oggplay_get_available(player);
		
		if (avail != saved_avail) 
		{
			saved_avail = avail;
			sprintf(buf, "available: %d\n", avail);
			OutputDebugString(buf);
			//printf("available: %d\n", avail);
		}
#endif
		if (r == E_OGGPLAY_END_OF_FILE)
		{
			OutputDebugString("end of file!\n");
			playing = 0;
			break;
		}

		if (r != E_OGGPLAY_CONTINUE && r != E_OGGPLAY_USER_INTERRUPT) 
		{
			OutputDebugString("end of file?\n");
			printf("hmm, totally bogus, dude.  r is %d\n", r);

			playing = 0;
			// wait for display thread to finish
//			sem_wait(&stop_sem);
	
			/* destroy window/cleanup here */
			break;
		}
	}

	decodingThreadHandle = 0;
}

void display_frame(void *arg)  
{	
	int                     i;
	int                     j;
	OggPlayDataHeader		**headers;
	OggPlayVideoData		*video_data;
	#if USE_AUDIO
	OggPlayAudioData		*audio_data;
	int						samples;
	#endif
	int                     required;
	OggPlayDataType         type;
	int                     num_tracks;
	OggPlayCallbackInfo		**track_info;  
	int						offset = 0;  
	int						delay;

	/* work out how many milliseconds per frame */
	delay = 1000 / fps_num;

	num_tracks = oggplay_get_num_tracks(player);

	frameThreadHandle = 1;

	while(fmvPlaying)
	{
		track_info = oggplay_buffer_retrieve_next(player);
		if (track_info == NULL) {
			/* sleep for 40ms - one frame at 25fps, if no new data available */
//			SDL_Delay(delay);
			Sleep(delay);
			continue;
		}

		Sleep(delay - 5);
/*
		if (target > bufferBytesPlayed - position) 
		{
			printf("bytes played: %d\n", bufferBytesPlayed);
			offset = (target - bufferBytesPlayed + position) * delay / target;
			printf("offset: %d\n", offset);
			SDL_Delay(offset);
		}
		position = bufferBytesPlayed;
*/

		/* avg bytes per sec */
//		bps = (rate * channels * 16) / 8;
//		printf("time taken: %f\n", (bufferBytesPlayed / bps) * 1.0f); // ??
#if 0
#if USE_AUDIO
		if (rate > 0) {
			/* get number of bytes played */
			int samples = 0;
			int error = 0;
			Uint32 bytes = bufferBytesPlayed;
			Uint32 offset = 0;

//			bytes = samples * channels * sizeof(short);
			offset = (target - (bytes * 100 / rate / 4)) * 100;
#else
		{
			Uint32 offset = 30;//400 * 100;
#endif
			target += 400;
/*
		track_info = oggplay_buffer_retrieve_next(player);
		if (track_info == NULL) {
			SDL_Delay(delay);
			continue;
		}
*/
			if (offset > 0) {
				SDL_Delay(offset);
			}
		}
#endif
		for (i = 0; i < num_tracks; i++) 
		{
			type = oggplay_callback_info_get_type(track_info[i]);
			headers = oggplay_callback_info_get_headers(track_info[i]);

			switch (type) 
			{
				case OGGPLAY_INACTIVE:
					break;
				case OGGPLAY_YUV_VIDEO:
					/*
					* there should only be one record
					*/

					required = oggplay_callback_info_get_required(track_info[i]);
					if (required == 0) 
					{
						oggplay_buffer_release(player, track_info);
						goto next_frame;
					}

					/* grab frame start time */
//					frameProcessTime = SDL_GetTicks();
						
					video_data = oggplay_callback_info_get_video_data(headers[0]);
//					printf("video presentation time: %ld\n",
//					        oggplay_callback_info_get_presentation_time(headers[0]));
					
//					printf("video fst %ld lst %ld\n",
//						oggplay_callback_info_get_presentation_time(headers[0]),
//						oggplay_callback_info_get_presentation_time(headers[required - 1]));


					handle_video_data(player, i, video_data, n_frames);
					break;
				case OGGPLAY_FLOATS_AUDIO:
					#if USE_AUDIO
					required = oggplay_callback_info_get_required(track_info[i]);          
       
					for (j = 0; j < required; j++) {      
						 /* Returns number of samples in the record. Note: the resulting data include samples for all audio channels */
						samples = oggplay_callback_info_get_record_size(headers[j]);
						audio_data = oggplay_callback_info_get_audio_data(headers[j]);  
						handle_audio_data(player, i, audio_data, samples);            
					}          
//					printf("audio presentation time: %ld\n",
//					        oggplay_callback_info_get_presentation_time(headers[j]));                    
					#endif    
					break;
				case OGGPLAY_CMML:
					if (oggplay_callback_info_get_required(track_info[i]) > 0)
						printf("%s\n", oggplay_callback_info_get_text_data(headers[0]));
					break;
				case OGGPLAY_KATE:
					required = oggplay_callback_info_get_required(track_info[i]);
					for (j = 0; j < required; j++)
						printf("[%d] %s\n", j, oggplay_callback_info_get_text_data(headers[j]));
					break;
				default:
					break;
			}
		}    
		n_frames++;

		oggplay_buffer_release(player, track_info);

next_frame: ;
//    ReleaseSemaphore(sem, 1, NULL);
	}

	frameThreadHandle = 0;
}

void float_to_short_array(const float* in, short* out, int len) 
{
	int i = 0;
	float scaled_value = 0;		
	for(i = 0; i < len; i++) {				
		scaled_value = floorf(0.5 + 32768 * in[i]);
		if (in[i] < 0) {
			out[i] = (scaled_value < -32768.0) ? -32768 : (short)scaled_value;
		} else {
			out[i] = (scaled_value > 32767.0) ? 32767 : (short)scaled_value;
		}
	}
}

void handle_audio_data(OggPlay * player, int track, OggPlayAudioData * data, int samples) 
{
	int dataSize = (samples * sizeof (short) * channels);

	if (audioDataBufferSize < dataSize) {
		if (audioDataBuffer != NULL)
		{
			delete []audioDataBuffer;
			OutputDebugString("deleted audioDataBuffer to resize\n");
		}
		
		audioDataBuffer = new unsigned char[dataSize];
		OutputDebugString("creating audioDataBuffer\n");

		if (!audioDataBuffer) {
			OutputDebugString("Out of memory\n");
			return;
		}
		audioDataBufferSize = dataSize;
	}

	float_to_short_array((float *)data, (short*)audioDataBuffer, samples * channels);

//	char buf[100];
//	sprintf(buf, "writable dsound space: %d\n", GetWritableBufferSize());
//	OutputDebugString(buf);

	GetWritableBufferSize();
	updateAudioBuffer(dataSize, (short*)audioDataBuffer);
#if 1
//	if (audio_opened) 
	{
		int freeSpace = 0;
		int firstSize = 0;
		int secondSize = 0;

//		sf_write_raw( sndFile, &audioDataBuffer[0], dataSize);

		/* if our read and write positions are at the same location, does that mean the buffer is empty or full? */
		if (fmvRingBuffer.bufferReadPos == fmvRingBuffer.bufferWritePos)
		{
			/* if fill size is greater than 0, we must be full? .. */
			if (fmvRingBuffer.bufferFillSize == fmvRingBuffer.bufferSize)
			{
				/* this should really just be zero store size free logically? */
				freeSpace = 0;// - bufferFillSize;
			}
			else if(fmvRingBuffer.bufferFillSize == 0)
			{
				/* buffer fill size must be zero, we can assume there's no data in buffer, ie freespace is entire buffer */
				freeSpace = fmvRingBuffer.bufferSize;
			}
		}
		else 
		{
			/* standard logic - buffers have a gap, work out writable space between them */
			freeSpace = fmvRingBuffer.bufferReadPos - fmvRingBuffer.bufferWritePos;
			if (freeSpace < 0) freeSpace += fmvRingBuffer.bufferSize;
		}

//		printf("free space is: %d\n", freeSpace);

		/* if we can't fit all our data.. */
		if (dataSize > freeSpace) 
		{
			char buf[100];
			sprintf(buf, "not enough room for all data. need %d, have free %d fillCount: %d\n", dataSize, freeSpace, fmvRingBuffer.bufferFillSize);
//			OutputDebugString(buf);
			dataSize = freeSpace;
		}

		/* space free from write cursor to end of buffer */
		firstSize = fmvRingBuffer.bufferSize - fmvRingBuffer.bufferWritePos;

		/* is first size big enough to hold all our data? */
		if (firstSize > dataSize) firstSize = dataSize;

//		SDL_LockAudio();

		/* first part. from write cursor to end of buffer */
		memcpy( &fmvRingBuffer.buffer[fmvRingBuffer.bufferWritePos], &audioDataBuffer[0], firstSize);

		secondSize = dataSize - firstSize;

#if WRITE_WAV
		sf_write_raw( sndFile, &dataStore[bufferWritePos], firstSize);
#endif

		/* need to do second copy due to wrap */
		if (secondSize > 0)
		{
//			printf("wrapped. firstSize: %d secondSize: %d totalWrite: %d dataSize: %d\n", firstSize, secondSize, (firstSize + secondSize), dataSize);
//			printf("write pos: %d read pos: %d free space: %d\n", bufferWritePos, bufferReadPos, freeSpace);
			/* copy second part. start of buffer to play cursor */
			memcpy( &fmvRingBuffer.buffer[0], &audioDataBuffer[firstSize], secondSize);

#if WRITE_WAV
			sf_write_raw( sndFile, &dataStore[0], secondSize);
#endif
		}

//		printf("data: %d firstSize: %d secondSize: %d total: %d\n", dataSize, firstSize, secondSize, firstSize + secondSize);

		/* update the write cusor position */
//		printf("buffer write was: %d now: %d\n", bufferWritePos, ((bufferWritePos + firstSize + secondSize) % MAX_STORE_SIZE));
		fmvRingBuffer.bufferWritePos = (fmvRingBuffer.bufferWritePos + firstSize + secondSize) % fmvRingBuffer.bufferSize;
		fmvRingBuffer.bufferFillSize += firstSize + secondSize;

		/* update read cursor */
//		bufferReadPos = (bufferReadPos + firstSize + secondSize) % MAX_STORE_SIZE;
//		bufferFillSize -= firstSize + secondSize;

		if (fmvRingBuffer.bufferFillSize > fmvRingBuffer.bufferSize)
		{
			OutputDebugString("bufferFillSize greater than store size!\n");
		}

		fmvRingBuffer.bufferBytesWritten += firstSize + secondSize;

#if 1
		if (fmvRingBuffer.bufferInitialFill == false)
		{
//			OutputDebugString("bufferInitialFill = false\n");
			if(totalBytesWritten >= fullBufferSize / 2)
			{
//				WriteToDsound(fullBufferSize, 0);

				char buf[100];
				sprintf(buf, "starting audio. buffer should have: %d bytes\n", fmvRingBuffer.bufferBytesWritten);
				OutputDebugString(buf);
				fmvRingBuffer.bufferInitialFill = true;

				if(FAILED(fmvAudioBuffer->Play(0, 0, DSBPLAY_LOOPING)))
				{
					OutputDebugString("can't play dsound fmv buffer\n");
				}
			}
		}
#endif
	}
#endif
}

int updateAudioBuffer(int numBytes, short *data)
{
	if(numBytes == 0) return 0;

	int bytesWritten = 0;
	DWORD playPosition = 0;
	DWORD writePosition = 0;
	DWORD writeLen = 0;

	unsigned char *audioData = (unsigned char *)data;

	/* check where play cursor is... */
	fmvAudioBuffer->GetCurrentPosition(&playPosition, &writePosition);

	char buf[100];
//	sprintf(buf, "play cursor: %i writing data at: %i\n", playPosition, bufferOffset);
//	OutputDebugString(buf);

	/* lock buffer */
	if(FAILED(fmvAudioBuffer->Lock(bufferOffset/*offset*/, numBytes/*size of lock*/, &fmvaudioPtr1, &fmvaudioBytes1, &fmvaudioPtr2, &fmvaudioBytes2, NULL))) 
	{
		OutputDebugString("\n couldn't lock buffer");
		return 0;
	}

	/* write data to buffer */
	if(!fmvaudioPtr2) // buffer didn't wrap
	{
		memcpy(fmvaudioPtr1, audioData, fmvaudioBytes1);
		bytesWritten += fmvaudioBytes1;
	}
	else // need to split memcpy
	{
		memcpy(fmvaudioPtr1, audioData, fmvaudioBytes1);
		memcpy(fmvaudioPtr2, &audioData[fmvaudioBytes1], fmvaudioBytes2);
		bytesWritten += fmvaudioBytes1 + fmvaudioBytes2;
	}

	if((fmvaudioBytes1 + fmvaudioBytes2) != numBytes)
	{
		OutputDebugString("didnt lock enough buffer space\n");
	}

	/* unlock buffer */
	if(FAILED(fmvAudioBuffer->Unlock(fmvaudioPtr1, fmvaudioBytes1, fmvaudioPtr2, fmvaudioBytes2))) 
	{
		OutputDebugString("\n couldn't unlock ds buffer");
		return 0;
	}

	/* add to previous offset, and get remainder from buffersize */
	bufferOffset = (bufferOffset + fmvaudioBytes1 + fmvaudioBytes2)
                     % fullBufferSize;


	totalBytesWritten += bytesWritten;//+= (audioBytes1 + audioBytes2);

	return bytesWritten;
}

int GetWritableBufferSize()
{
	DWORD playPosition = 0;
	DWORD writePosition = 0;
	DWORD writeLen = 0;
	long bytesPlayed = 0;

	fmvAudioBuffer->GetCurrentPosition(&playPosition, &writePosition);

	/* How many bytes does DirectSound say have been played. */
	bytesPlayed = playPosition - lastPlayCursor;
	if( bytesPlayed < 0 ) bytesPlayed += fullBufferSize; // unwrap
	lastPlayCursor = playPosition;

	char buf[100];
//	sprintf(buf, "getWrite - playPosition: %d writePosition: %d\n", playPosition, writePosition);
//	OutputDebugString(buf);

	if(bufferOffset <= playPosition)
	{
		writeLen = playPosition - bufferOffset;
	}

    else // (bufferOffset > playPosition)
    {
		// Play cursor has wrapped
		writeLen = (fullBufferSize - bufferOffset) + playPosition;
    }

//	char buf[100];
//	sprintf(buf, "bytesPlayed: %ld\n", bytesPlayed);
//	OutputDebugString(buf);
/*
	if(bytesPlayed > bufferSize) // FIX THIS PLZZ
	{	
		bytesPlayed = 0;
	}
*/
	totalBytesPlayed += bytesPlayed;

//	sprintf(buf, "total bytes played: %d\n", totalBytesPlayed);
//	OutputDebugString(buf);

	return writeLen;
}

int WriteToDsound(/*char *audioData,*/ int dataSize, int offset)
{
	/* find out how we need to read data. do we split into 2 memcpys or not? */
	int firstSize = 0;
	int secondSize = 0;
	int readableSpace = 0;

	/* if our read and write positions are at the same location, does that mean the buffer is empty or full? */
	if (fmvRingBuffer.bufferReadPos == fmvRingBuffer.bufferWritePos)
	{
		/* if fill size is greater than 0, we must be full? .. */
		if (fmvRingBuffer.bufferFillSize == fmvRingBuffer.bufferSize)
		{
			/* this should really just be entire buffer logically? */
			readableSpace = fmvRingBuffer.bufferSize;
		}
		else
		{
			/* buffer fill size must be zero, we can assume there's no data in buffer */
			readableSpace = 0;
		}
	}
	else 
	{
		/* standard logic - buffers have a gap, work out readable space between them */
		readableSpace = fmvRingBuffer.bufferWritePos - fmvRingBuffer.bufferReadPos;
		if (readableSpace < 0) readableSpace += fmvRingBuffer.bufferSize;
	}

	if (readableSpace == 0)
	{
		OutputDebugString("no readable audio data in ring buffer!\n");
	}

	if (dataSize > readableSpace)
	{
		OutputDebugString("not enough audio data to satisfy dsound\n");
	}

	char buf[100];
	sprintf(buf, "dsound wants: %d we have: %d\n", dataSize, readableSpace);
	OutputDebugString(buf);

	firstSize = fmvRingBuffer.bufferSize - fmvRingBuffer.bufferReadPos;

	if (dataSize > readableSpace) dataSize = readableSpace;
	if (firstSize > dataSize) firstSize = dataSize;

	secondSize = dataSize - firstSize;
	
	/* work out how to write this data based on the pointers DSOUND returns from the lock */
#if 1
	/* lock the fmv buffer */
	if(FAILED(fmvAudioBuffer->Lock(offset, dataSize, &fmvaudioPtr1, &fmvaudioBytes1, &fmvaudioPtr2, &fmvaudioBytes2, NULL))) 
	{
		OutputDebugString("couldn't lock fmv audio buffer for update\n");
	}

	int read = 0; 
	int write = 0;
	int dataWritten = 0;

	/* write data to buffer */
	if(!fmvaudioPtr2) // buffer didn't wrap
	{
		if (fmvaudioBytes1 <= firstSize)
		{
			memcpy(fmvaudioPtr1, &fmvRingBuffer.buffer[fmvRingBuffer.bufferReadPos], fmvaudioBytes1);
			dataWritten += fmvaudioBytes1;
		}
		else
		{
			memcpy(fmvaudioPtr1, &fmvRingBuffer.buffer[fmvRingBuffer.bufferReadPos], firstSize);
			dataWritten += firstSize;
		}
		if (secondSize)
		{
			/* how much left to fill */
			if ((fmvaudioBytes1 - dataWritten) <=  secondSize)
			{
				memcpy((unsigned char*)fmvaudioPtr1 + dataWritten, &fmvRingBuffer.buffer[0], fmvaudioBytes1 - dataWritten);
				dataWritten += (fmvaudioBytes1 - dataWritten);
			}
			else
			{
				memcpy((unsigned char*)fmvaudioPtr1 + dataWritten, &fmvRingBuffer.buffer[0], secondSize);
				dataWritten += secondSize;
			}
		}
	}
	else // need to split memcpy
	{
		OutputDebugString("dsound wrap\n");
	}

	if(FAILED(fmvAudioBuffer->Unlock(fmvaudioPtr1, fmvaudioBytes1, fmvaudioPtr2, fmvaudioBytes2))) 
	{
		OutputDebugString("couldn't unlock fmv audio buffer\n");
	}

	fmvRingBuffer.bufferReadPos = (fmvRingBuffer.bufferReadPos + dataWritten) % fmvRingBuffer.bufferSize;
	fmvRingBuffer.bufferFillSize -= dataWritten;

#endif
	return 0;
}

void handle_video_data (OggPlay * player, int track_num, OggPlayVideoData * video_data, int frame) 
{
	int					y_width;
	int					y_height;
	int					uv_width;
	int					uv_height;
	OggPlayYUVChannels	yuv;
	OggPlayRGBChannels	rgb;
	long frameTime = 0;
	int frameDelay = 0;

	oggplay_get_video_y_size(player, track_num, &y_width, &y_height);
	oggplay_get_video_uv_size(player, track_num, &uv_width, &uv_height);

	if (textureData == NULL) 
	{
		textureData = new unsigned char[y_width * y_height * 4];
		imageWidth = y_width;
		imageHeight = y_height;
	}

	/*
	* Convert the YUV data to RGB, using platform-specific optimisations
	* where possible.
	*/
	yuv.ptry = video_data->y;
	yuv.ptru = video_data->u;
	yuv.ptrv = video_data->v;
	yuv.uv_width = uv_width;
	yuv.uv_height = uv_height;  
	yuv.y_width = y_width;
	yuv.y_height = y_height;  

	rgb.ptro = textureData;
	rgb.rgb_width = y_width;
	rgb.rgb_height = y_height;  

#if USE_ARGB
	oggplay_yuv2argb(&yuv, &rgb);
#else
	oggplay_yuv2rgb(&yuv, &rgb);
#endif
	

	frameReady = true;
}

bool FmvWait()
{
	if (frameReady == true)
	{
		return false;
	}
	else
	{
		return true;
	}
}

void writeFmvData(unsigned char *destData, unsigned char* srcData, int width, int height, int pitch)
{
	unsigned char *srcPtr, *destPtr;

	if (srcData == NULL) return;

	srcPtr = (unsigned char *)srcData;

	// then actual image data
	for (int y = 0; y < height; y++)
	{
		destPtr = (((unsigned char *)destData) + y*pitch);

#if USE_ARGB
		memcpy(destPtr, srcPtr, width * 4);
		srcPtr+=width * 4;
#else
		for (int x = 0; x < width; x++)
		{
			*(D3DCOLOR*)destPtr = D3DCOLOR_RGBA(srcPtr[0], srcPtr[1], srcPtr[2], srcPtr[3]);

			destPtr+=4;
			srcPtr+=4;
		}
#endif
	}
}

int NextFMVFrame()
{
	D3DLOCKED_RECT texture_rect;

	/* lock the d3d texture */
	fmvDynamicTexture->LockRect( 0, &texture_rect, NULL, NULL );

	/* copy bink frame to texture */
//	BinkCopyToBuffer(binkHandle, texture_rect.pBits, texture_rect.Pitch, 1024, 0, 0, BinkSurfaceType);
//	unsigned char *srcPtr, *destPtr;

	/* if src pointer is null, return 'error' */
//	if (textureData == NULL) return -1;
//
//	srcPtr = (unsigned char *)textureData;

	writeFmvData((unsigned char*)texture_rect.pBits, textureData, imageWidth, imageHeight, texture_rect.Pitch);

	/* unlock d3d texture */
	fmvDynamicTexture->UnlockRect( 0 );

	/* update rendering texture with FMV image */
	d3d.lpD3DDevice->UpdateTexture( fmvDynamicTexture, fmvTexture );

	frameReady = false;

	return 1;
}

int CreateFMVAudioBuffer(int channels, int rate)
{
	WAVEFORMATEX waveFormat;
	DSBUFFERDESC bufferFormat;

	memset(&waveFormat, 0, sizeof(waveFormat));
	waveFormat.wFormatTag		= WAVE_FORMAT_PCM;
	waveFormat.nChannels		= channels;				//how many channels the OGG contains
	waveFormat.wBitsPerSample	= 16;					//always 16 in OGG
	waveFormat.nSamplesPerSec	= rate;	
	waveFormat.nBlockAlign		= waveFormat.nChannels * waveFormat.wBitsPerSample / 8;	//what block boundaries exist
	waveFormat.nAvgBytesPerSec	= waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;	//average bytes per second
	waveFormat.cbSize			= sizeof(waveFormat);	//how big this structure is

	memset(&bufferFormat, 0, sizeof(DSBUFFERDESC));
	bufferFormat.dwSize = sizeof(DSBUFFERDESC);
	bufferFormat.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE;
	bufferFormat.dwBufferBytes = waveFormat.nAvgBytesPerSec / 2;
	bufferFormat.lpwfxFormat = &waveFormat;

	if(FAILED(DSObject->CreateSoundBuffer(&bufferFormat, &fmvAudioBuffer, NULL))) 
	{
///		LogDxErrorString("couldn't create audio buffer for fmv\n");
		OutputDebugString("couldn't create audio buffer for fmv\n");
		return 0;
	}
	else { OutputDebugString("created dsound fmv buffer....\n"); }

#if 0
	if(FAILED(fmvAudioBuffer->QueryInterface( IID_IDirectSoundNotify, 
                                        (void**)&fmvpDSNotify ) ) )
	{
//		LogDxErrorString("couldn't query interface for ogg vorbis buffer notifications\n");
		OutputDebugString("couldn't query interface for ogg vorbis buffer notifications\n");
	}

	DSBPOSITIONNOTIFY notifyPosition[2];

	fmvhHandles[0] = CreateEvent(NULL,FALSE,FALSE,NULL);
	fmvhHandles[1] = CreateEvent(NULL,FALSE,FALSE,NULL);

	// halfway
	notifyPosition[0].dwOffset = bufferFormat.dwBufferBytes / 2;
	notifyPosition[0].hEventNotify = fmvhHandles[0];

	// end
	notifyPosition[1].dwOffset = bufferFormat.dwBufferBytes - 1;
	notifyPosition[1].hEventNotify = fmvhHandles[1];

	if(FAILED(fmvpDSNotify->SetNotificationPositions(2, notifyPosition)))
	{
		OutputDebugString("couldn't set notifications for ogg vorbis buffer\n");
		fmvpDSNotify->Release();
		return 1;
	}

	fmvhHandles[0] = notifyPosition[0].hEventNotify;
	fmvhHandles[1] = notifyPosition[1].hEventNotify;

	fmvpDSNotify->Release();
	fmvpDSNotify = NULL;

#endif

#if WRITE_WAV
	const int format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;  
	const char* outfilename = "c:/foo.wav";
	SF_INFO info;

	info.samplerate = rate;
	info.channels = channels;
	info.format = format;

//	sndFile = sf_open(outfilename, SFM_READ, &info);
	sndFile = sf_open(outfilename, SFM_WRITE, &info);

#endif

	char buf[100];
	sprintf(buf, "audio buffer size: %d\n", bufferFormat.dwBufferBytes);
	OutputDebugString(buf);

	/* initalise data buffer in ring buffer */
	fmvRingBuffer.buffer = new unsigned char[bufferFormat.dwBufferBytes * 2];
	fmvRingBuffer.bufferSize = bufferFormat.dwBufferBytes;

	return bufferFormat.dwBufferBytes;
}

} // extern "C"

//#endif