#define USE_FMV
#ifdef USE_FMV

#define	USE_AUDIO		1
#define USE_ARGB		1

extern "C" {

#include "d3_func.h"
#include "fmv.h"
#include "avp_userprofile.h"
#include "inline.h"
#include <math.h>

#include <assert.h>

int SmackerSoundVolume = 65536/512; // 128
int MoviesAreActive;
int IntroOutroMoviesAreActive = 1;
int VolumeOfNearestVideoScreen = 0;
int PanningOfNearestVideoScreen = 0;

#define MAX_NO_FMVTEXTURES 10
FMVTEXTURE FMVTexture[MAX_NO_FMVTEXTURES] = {0};
int NumberOfFMVTextures = 0;

extern unsigned char GotAnyKey;
extern unsigned char DebouncedGotAnyKey;
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
#pragma comment(lib, "liboggplay_static.lib")
#pragma comment(lib, "liboggz_static.lib")

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

static int				channels	= 0;
static int				rate		= 0;
static int				fps_num		= 0;
static int				fps_denom	= 0;
static int				n_frames	= 0;

bool					fmvPlaying	= false;
bool					frameReady	= false;

static long				presentationTime = 0;

extern void ThisFramesRenderingHasBegun(void);
extern void ThisFramesRenderingHasFinished(void);

void drive_decoding(void *arg);
void display_frame(void *arg);
void handle_video_data(OggPlay * player, int track_num, OggPlayVideoData * video_data, int frame);
int NextFMVFrame();
int FmvOpen(char *filenamePtr);
void FmvClose();
int GetVolumeOfNearestVideoScreen(void);
void WriteFmvData(unsigned char *destData, unsigned char* srcData, int width, int height, int pitch);
int CreateFMVAudioBuffer(int channels, int rate);
void handle_audio_data (OggPlay * player, int track, OggPlayAudioData * data, int samples);
int WriteToDsound(int dataSize);
int GetWritableBufferSize();
int UpdateAudioBuffer(int numBytes, short *data);

unsigned char	*textureData	= NULL;
int				imageWidth		= 0;
int				imageHeight		= 0;
int				textureWidth	= 0;
int				textureHeight	= 0;

int				playing = 0;

extern			D3DINFO d3d;
D3DTEXTURE		fmvTexture = NULL;
D3DTEXTURE		fmvDynamicTexture = NULL;

/* dsound stuff */
extern LPDIRECTSOUND	DSObject;
LPDIRECTSOUNDBUFFER		fmvAudioBuffer = NULL;
LPVOID					fmvaudioPtr1;
DWORD					fmvaudioBytes1;
LPVOID					fmvaudioPtr2;   
DWORD					fmvaudioBytes2;
int						fullBufferSize = 0;
int						halfBufferSize = 0;

long					writeOffset = 0;
long					lastPlayCursor = 0;
long					totalBytesPlayed = 0;
long					totalBytesWritten = 0;
long					totalAudioTimePlayed = 0;


/* audio buffer for liboggplay */
static unsigned char	*audioDataBuffer = NULL;
static int				audioDataBufferSize = 0;

static int				audioBytesPerSec = 0;

unsigned char			*testBuffer = NULL;

/* ring buffer stuff */
struct ringBuffer 
{
	int				readPos;
	int				writePos;
	long			bytesPlayed;
	long			bytesWritten;
	int				fillSize;
	bool			initialFill;

	unsigned char	*buffer;
	int				bufferSize;
};
ringBuffer ring = {0};

int count = 0;
char buf[100];

extern void EndMenuBackgroundBink()
{
}

extern void StartMenuBackgroundBink()
{
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

		if (frameReady)
			playing = NextFMVFrame();

		ThisFramesRenderingHasBegun();
		ClearScreenToBlack();

			DrawBinkFmv((640-imageWidth)/2, (480-imageHeight)/2, imageHeight, imageWidth, fmvTexture);

		ThisFramesRenderingHasFinished();
		FlipBuffers();

		DirectReadKeyboard();

		/* added DebouncedGotAnyKey to ensure previous frame's key press for starting level doesn't count */
		if (GotAnyKey && DebouncedGotAnyKey) playing = 0;
	}

	OutputDebugString("closing pre-game fmv..\n");
	FmvClose();
}

void FmvClose()
{
	/* bail out if we're not playing */
	if (fmvPlaying == false) return;

	fmvPlaying = false;

	/* ......................... ahem */
	while ((frameThreadHandle != 0) || (decodingThreadHandle != 0) || (updateAudioThreadHandle != 0))
	{
		Sleep(2);
	}

	OutputDebugString("releasing..\n");

	char buf[100];
	sprintf(buf, "had %d audio buffer locks\n", count);
	OutputDebugString(buf);

	count = 0;

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
	delete []ring.buffer;
	ring.buffer = NULL;
	ring.bytesPlayed = 0;
	ring.bytesWritten = 0;
	ring.fillSize = 0;
	ring.initialFill = false;
	ring.readPos = 0;
	ring.bufferSize = 0;
	ring.writePos = 0;

	writeOffset = 0;
	lastPlayCursor = 0;
	totalBytesPlayed = 0;
	totalBytesWritten = 0;

	delete []testBuffer;

	/* stop and release dsound buffer */
	if(fmvAudioBuffer != NULL)
	{
		fmvAudioBuffer->Stop();
		OutputDebugString("releasing fmv dsound buffer...\n");
		SAFE_RELEASE(fmvAudioBuffer);
	}

	/* clean up after liboggplay */
	oggplay_close(player);

	OutputDebugString("liboggplay is gone\n");
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
			OutputDebugString("couldn't open fmv file: ");
			OutputDebugString(buffer);
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

//						*filenamePtr++='s';
//						*filenamePtr++='m';
//						*filenamePtr++='k';
						*filenamePtr++='o';
						*filenamePtr++='g';
						*filenamePtr++='v';
						*filenamePtr=0;
					}

					/* do a check here to see if it's a theora file rather than just any old file with the right name? */
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

	for(int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;
		SAFE_RELEASE(FMVTexture[i].DestTexture);
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}
	NumberOfFMVTextures = 0;
}

void ReleaseAllFMVTexturesForDeviceReset()
{
	for(int i = 0; i < NumberOfFMVTextures; i++)
	{
		FMVTexture[i].MessageNumber = 0;

//		SAFE_RELEASE(FMVTexture[i].DestTexture);
		SAFE_RELEASE(FMVTexture[i].ImagePtr->Direct3DTexture);
	}
}

void RecreateAllFMVTexturesAfterDeviceReset()
{
	for(int i = 0; i < NumberOfFMVTextures; i++)
	{	
		//SetupFMVTexture(&FMVTexture[i]);
		d3d.lpD3DDevice->CreateTexture(FMVTexture[i].ImagePtr->ImageWidth, FMVTexture[i].ImagePtr->ImageHeight, 1, NULL, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &FMVTexture[i].ImagePtr->Direct3DTexture, NULL);
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

void FmvVolumePan(int volume, int pan)
{
	/* 
		have changed the code that calls this function to pass the pan value
		in the rage of -32768 for left, 0 middle and 32768 for right.

		DSound needs the pan in the range of -10000 to 10000. need to conver tour
		value to that format. the code below should do this. At least, it seems to pan and sound fine!
	*/

	int volume2 = (int)volume * .30517578125f;
	if (volume2 < 0) volume2 = 0;
	if (volume2 > 10000) volume2 = 10000;

//	fmvAudioBuffer->SetVolume(volume2);

	int pan2 = (int)pan * .30517578125f;

	if (pan2 < -10000) pan2 = -10000;
	if (pan2 > 10000) pan2 = 10000;

	fmvAudioBuffer->SetPan(pan2);
}

int NextFMVTextureFrame(FMVTEXTURE *ftPtr, void *bufferPtr, int pitch)
{
	int smackerFormat = 1;
	int w = 128;
	int h = 96;

	if (MoviesAreActive && ftPtr->SmackHandle)
	{
		int volume = MUL_FIXED(SmackerSoundVolume*256, GetVolumeOfNearestVideoScreen());

//		sprintf(buf, "vol of nearest screen: %d, volume: %d, pan: %d\n", GetVolumeOfNearestVideoScreen(), volume, PanningOfNearestVideoScreen);
//		OutputDebugString(buf);

		FmvVolumePan(volume, PanningOfNearestVideoScreen);
//		SmackVolumePan(ftPtr->SmackHandle,SMACKTRACKS,volume,PanningOfNearestVideoScreen);
		ftPtr->SoundVolume = SmackerSoundVolume;
	    
//	    if (SmackWait(ftPtr->SmackHandle)) return 0;
		//if (FmvWait()) return 0;

		if (!frameReady) return 0;
		/* unpack frame */

		WriteFmvData((unsigned char*)bufferPtr, textureData, w, h, pitch);

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
			
			ftPtr->MessageNumber = 0;
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
						int temp = /*32768 +*/ DotProduct(&disp,&rightEarDirection)/2;
						//PanningOfNearestVideoScreen = 32768 + DotProduct(&disp,&rightEarDirection)/2;
						PanningOfNearestVideoScreen = temp;
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
	FILE *test;
	test = fopen(filenamePtr, "r");
	if (test == NULL)
	{
		sprintf(buf, "couldn't find fmv file: %s\n", filenamePtr);
		OutputDebugString(buf);
		return 1;
	}
	else fclose(test);

	/* try to open our video file by filename*/
	reader = oggplay_file_reader_new(filenamePtr);
	if (reader == NULL)
	{
		OutputDebugString("couldn't create oggplay reader\n");
		return 1;
	}

	player = oggplay_open_with_reader(reader);
	if (player == NULL) 
	{
		char message[100];
		sprintf(message,"Unable to access file: %s\n",filenamePtr);
#ifdef WIN32
		MessageBox(hWndMain, message, "AvP Error", MB_OK+MB_SYSTEMMODAL);
#endif
		return 1;
	}

	video_track = -1; 
	for (int i = 0; i < oggplay_get_num_tracks (player); i++) 
	{
		printf("Track %d is of type %s\n", i, oggplay_get_track_typename (player, i));
	
		if (oggplay_get_track_type (player, i) == OGGZ_CONTENT_THEORA) 
		{
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
			oggplay_set_offset(player, i, 250);
			ret = oggplay_get_audio_samplerate(player, i , &rate);
			ret = oggplay_get_audio_channels(player, i, &channels);
			printf("samplerate: %d channels: %d\n", rate, channels);
		}
		else if (oggplay_get_track_type (player, i) == OGGZ_CONTENT_KATE) 
		{
			const char *category = "<unknown>", *language = "<unknown>";
			int ret = oggplay_get_kate_category(player, i, &category);
			ret = oggplay_get_kate_language(player, i, &language);
			printf("category %s, language %s\n", category, language);
		}

		if (oggplay_set_track_active(player, i) < 0) 
		{
			//printf("\tNote: Could not set this track active!\n");
			OutputDebugString("\tNote: Could not set this track active!\n");
		}
	}

	if (video_track == -1) 
	{
		if (audio_track >= 0) 
		{
			oggplay_set_callback_num_frames(player, audio_track, 2048);
		}
	}

	oggplay_use_buffer(player, 20);

	/* create the dsound buffer and set size variables */
	fullBufferSize = CreateFMVAudioBuffer(channels, rate);
	halfBufferSize = fullBufferSize / 2;

	fmvPlaying = true;

	/* create decoding threads */
	_beginthread(drive_decoding, 0, 0);
	_beginthread(display_frame, 0, 0);

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
		if (track_info == NULL) 
		{
			/* sleep for 40ms - one frame at 25fps, if no new data available */
			//Sleep(delay);
			continue;
		}

//		char buf[100];
//		sprintf(buf, "presentationTime: %ld totalAudioTimePlayed: %ld\n", presentationTime, totalAudioTimePlayed);
//		OutputDebugString(buf);

		if (totalAudioTimePlayed < presentationTime)
		{
//			OutputDebugString("totalAudioTimePlayed < presentationTime\n");
			long temp = presentationTime - totalAudioTimePlayed;
			//if (temp < delay)
			{
				Sleep(temp);
			}
		}
		else 
		{
//			Sleep(totalAudioTimePlayed - presentationTime);
		}

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
					presentationTime = oggplay_callback_info_get_presentation_time(headers[0]);
					
//					printf("video fst %ld lst %ld\n",
//						oggplay_callback_info_get_presentation_time(headers[0]),
//						oggplay_callback_info_get_presentation_time(headers[required - 1]));


					handle_video_data(player, i, video_data, n_frames);
					break;
				case OGGPLAY_FLOATS_AUDIO:
					#if USE_AUDIO
					required = oggplay_callback_info_get_required(track_info[i]);          
       
					for (j = 0; j < required; j++) 
					{      
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
	for(i = 0; i < len; i++) 
	{				
		scaled_value = floorf(0.5 + 32768 * in[i]);
		if (in[i] < 0) 
		{
			out[i] = (scaled_value < -32768.0) ? -32768 : (short)scaled_value;
		} 
		else 
		{
			out[i] = (scaled_value > 32767.0) ? 32767 : (short)scaled_value;
		}
	}
}

void handle_audio_data(OggPlay * player, int track, OggPlayAudioData * data, int samples) 
{
	int dataSize = (samples * sizeof (short) * channels);

	if (audioDataBufferSize < dataSize) 
	{
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
//	sprintf(buf, "writable size: %d\n", GetWritableBufferSize());
//	OutputDebugString(buf);

//	UpdateAudioBuffer(dataSize, (short*)audioDataBuffer);

//	if (audio_opened) 
	{
		int freeSpace = 0;
		int firstSize = 0;
		int secondSize = 0;

		/* if our read and write positions are at the same location, does that mean the buffer is empty or full? */
		if (ring.readPos == ring.writePos)
		{
			/* if fill size is greater than 0, we must be full? .. */
			if (ring.fillSize == ring.bufferSize)
			{
				/* this should really just be zero store size free logically? */
				freeSpace = 0;
			}
			else if(ring.fillSize == 0)
			{
				/* buffer fill size must be zero, we can assume there's no data in buffer, ie freespace is entire buffer */
				freeSpace = ring.bufferSize;
			}
		}
		else 
		{
			/* standard logic - buffers have a gap, work out writable space between them */
			freeSpace = ring.readPos - ring.writePos;
			if (freeSpace < 0) freeSpace += ring.bufferSize;
		}

//		printf("free space is: %d\n", freeSpace);

		/* if we can't fit all our data.. */
		if (dataSize > freeSpace) 
		{
			char buf[100];
			sprintf(buf, "not enough room for all data. need %d, have free %d fillCount: %d\n", dataSize, freeSpace, ring.fillSize);
//			OutputDebugString(buf);
			dataSize = freeSpace;
		}

		/* space free from write cursor to end of buffer */
		firstSize = ring.bufferSize - ring.writePos;

		/* is first size big enough to hold all our data? */
		if (firstSize > dataSize) firstSize = dataSize;

		/* first part. from write cursor to end of buffer */
		memcpy( &ring.buffer[ring.writePos], &audioDataBuffer[0], firstSize);

		secondSize = dataSize - firstSize;

		/* need to do second copy due to wrap */
		if (secondSize > 0)
		{
//			printf("wrapped. firstSize: %d secondSize: %d totalWrite: %d dataSize: %d\n", firstSize, secondSize, (firstSize + secondSize), dataSize);
//			printf("write pos: %d read pos: %d free space: %d\n", writePos, readPos, freeSpace);
			/* copy second part. start of buffer to play cursor */
			memcpy( &ring.buffer[0], &audioDataBuffer[firstSize], secondSize);
		}

//		printf("data: %d firstSize: %d secondSize: %d total: %d\n", dataSize, firstSize, secondSize, firstSize + secondSize);

		/* update the write cusor position */
//		printf("buffer write was: %d now: %d\n", writePos, ((writePos + firstSize + secondSize) % MAX_STORE_SIZE));
		ring.writePos = (ring.writePos + firstSize + secondSize) % ring.bufferSize;
		ring.fillSize += firstSize + secondSize;

		/* update read cursor */
//		readPos = (readPos + firstSize + secondSize) % MAX_STORE_SIZE;
//		fillSize -= firstSize + secondSize;

		if (ring.fillSize > ring.bufferSize)
		{
			OutputDebugString("fillSize greater than store size!\n");
		}

		ring.bytesWritten += firstSize + secondSize;
	}

	int writableSize = GetWritableBufferSize();

	/* this SEEMS to be ok.. don't want to write data too often */
	if (writableSize > halfBufferSize)
	{
		WriteToDsound(writableSize);
	}
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

//	sprintf(buf, "getWrite - playPosition: %d writePosition: %d\n", playPosition, writePosition);
//	OutputDebugString(buf);

	if (writeOffset <= playPosition)
	{
		writeLen = playPosition - writeOffset;
	}

    else // (writeOffset > playPosition)
    {
		// Play cursor has wrapped
		writeLen = (fullBufferSize - writeOffset) + playPosition;
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

	/* test */
	float timeTaken = 0.0f;
	long first, second;

	timeTaken = (totalBytesPlayed / (audioBytesPerSec * 1.0f));

	first = (long)(timeTaken);
	second = (long)(fmodf(timeTaken, 1.0f) * 1000);

	totalAudioTimePlayed = ((first * 1000) + second);

//	sprintf(buf, "total bytes played: %d\n", totalBytesPlayed);
//	OutputDebugString(buf);

	return writeLen;
}

int WriteToDsound(int dataSize)
{
	/* find out how we need to read data. do we split into 2 memcpys or not? */
	int firstSize = 0;
	int secondSize = 0;
	int readableSpace = 0;

	/* if our read and write positions are at the same location, does that mean the buffer is empty or full? */
	if (ring.readPos == ring.writePos)
	{
		/* if fill size is greater than 0, we must be full? .. */
		if (ring.fillSize == ring.bufferSize)
		{
			/* this should really just be entire buffer logically? */
			readableSpace = ring.bufferSize;
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
		readableSpace = ring.writePos - ring.readPos;
		if (readableSpace < 0) readableSpace += ring.bufferSize;
	}

	if (readableSpace == 0)
	{
		OutputDebugString("no readable audio data in ring buffer!\n");
	}

	if (dataSize > readableSpace)
	{
		OutputDebugString("not enough audio data to satisfy dsound\n");
	}

/*
	char buf[100];
	sprintf(buf, "dsound wants: %d we have: %d\n", dataSize, readableSpace);
	OutputDebugString(buf);
*/
	firstSize = ring.bufferSize - ring.readPos;

	if (dataSize > readableSpace) dataSize = readableSpace;
	if (firstSize > dataSize) firstSize = dataSize;

	secondSize = dataSize - firstSize;

	/* copy audio into temp buffer, reordering to make copying to dsound buffer a bit easier */
	memcpy(&testBuffer[0], &ring.buffer[ring.readPos], firstSize);
	if (secondSize) memcpy(&testBuffer[firstSize], &ring.buffer[0], secondSize);

	/* lock the fmv buffer at our write offset */
	if(FAILED(fmvAudioBuffer->Lock(writeOffset, dataSize, &fmvaudioPtr1, &fmvaudioBytes1, &fmvaudioPtr2, &fmvaudioBytes2, NULL))) 
	{
		OutputDebugString("couldn't lock fmv audio buffer for update\n");
		return 0;
	}

	count++;

	int dataWritten = 0;

	if(!fmvaudioPtr2) // buffer didn't wrap
	{
		memcpy(fmvaudioPtr1, &testBuffer[0], dataSize);
		dataWritten += fmvaudioBytes1;
	}
	else
	{
		memcpy(fmvaudioPtr1, &testBuffer[0], fmvaudioBytes1);
		dataWritten += fmvaudioBytes1;

		memcpy(fmvaudioPtr2, &testBuffer[fmvaudioBytes1], dataSize - fmvaudioBytes1);
		dataWritten += fmvaudioBytes2;
	}
#if 0
	/* write data to buffer */
	if(!fmvaudioPtr2) // buffer didn't wrap
	{
		if (fmvaudioBytes1 <= firstSize)
		{
			memcpy(fmvaudioPtr1, &ring.buffer[ring.readPos], fmvaudioBytes1);
			dataWritten += fmvaudioBytes1;
		}
		else
		{
			memcpy(fmvaudioPtr1, &ring.buffer[ring.readPos], firstSize);
			dataWritten += firstSize;
		}
		if (secondSize)
		{
			/* how much left to fill */
			if ((fmvaudioBytes1 - dataWritten) <=  secondSize)
			{
				memcpy((unsigned char*)fmvaudioPtr1 + dataWritten, &ring.buffer[0], fmvaudioBytes1 - dataWritten);
				dataWritten += (fmvaudioBytes1 - dataWritten);
			}
			else
			{
				memcpy((unsigned char*)fmvaudioPtr1 + dataWritten, &ring.buffer[0], secondSize);
				dataWritten += secondSize;
			}
		}
	}
	else // need to split memcpy
	{
		OutputDebugString("dsound wrap\n");
	}
#endif

	if(FAILED(fmvAudioBuffer->Unlock(fmvaudioPtr1, fmvaudioBytes1, fmvaudioPtr2, fmvaudioBytes2))) 
	{
		OutputDebugString("couldn't unlock fmv audio buffer\n");
	}

	/* update our ring buffer read position and track the fact we've "removed" some data from it */
	ring.readPos = (ring.readPos + dataWritten) % ring.bufferSize;
	ring.fillSize -= dataWritten;

	/* update our write cursor to next write position */
	writeOffset += dataWritten;
	if (writeOffset >= ring.bufferSize) writeOffset = writeOffset - ring.bufferSize;

	return dataWritten;
}

void handle_video_data (OggPlay * player, int track_num, OggPlayVideoData * video_data, int frame) 
{
	int					y_width;
	int					y_height;
	int					uv_width;
	int					uv_height;
	OggPlayYUVChannels	yuv;
	OggPlayRGBChannels	rgb;
	long				frameTime = 0;
	int					frameDelay = 0;

	oggplay_get_video_y_size(player, track_num, &y_width, &y_height);
	oggplay_get_video_uv_size(player, track_num, &uv_width, &uv_height);

	if (textureData == NULL) 
	{
		textureData = new unsigned char[y_width * y_height * 4];
		imageWidth  = y_width;
		imageHeight = y_height;
	}

	/*
	* Convert the YUV data to RGB, using platform-specific optimisations
	* where possible.
	*/
	yuv.ptry		= video_data->y;
	yuv.ptru		= video_data->u;
	yuv.ptrv		= video_data->v;
	yuv.uv_width	= uv_width;
	yuv.uv_height	= uv_height;  
	yuv.y_width		= y_width;
	yuv.y_height	= y_height;  

	rgb.ptro		= textureData;
	rgb.rgb_width	= y_width;
	rgb.rgb_height	= y_height;  

#if USE_ARGB
	oggplay_yuv2bgra(&yuv, &rgb);
#else
	oggplay_yuv2rgba(&yuv, &rgb);
#endif

	frameReady = true;

	/* probably a better place to put this..start playing dsound buffer once we hit video */
	DWORD status;
	fmvAudioBuffer->GetStatus(&status);
	if (!(status & DSBSTATUS_PLAYING))
	{
		if(FAILED(fmvAudioBuffer->Play(0, 0, DSBPLAY_LOOPING)))
		{
			OutputDebugString("can't play dsound fmv buffer\n");
		}
		else OutputDebugString("started playing Dsound buffer\n");
	}
}

void WriteFmvData(unsigned char *destData, unsigned char *srcData, int width, int height, int pitch)
{
	unsigned char *srcPtr, *destPtr;

	if (srcData == NULL) return;

	srcPtr = (unsigned char *)srcData;

	/* can we copy the whole image in one go? */
	if (pitch == (width * 4))
	{
		memcpy(destData, srcPtr, (height * width * 4));
	}
	else
	{
		// then actual image data
		for (int y = 0; y < height; y++)
		{
			destPtr = (((unsigned char *)destData) + y*pitch);

#if USE_ARGB // copy line by line
			memcpy(destPtr, srcPtr, width * 4);
			srcPtr += (width * 4);
#else		//copy pixel by pixel
			for (int x = 0; x < width; x++)
			{
				// A R G B
				// A B G R
				destPtr[0] = srcPtr[3];
				destPtr[1] = srcPtr[2];
				destPtr[2] = srcPtr[1];
				destPtr[3] = srcPtr[0];

				destPtr+=4;
				srcPtr+=4;
			}
#endif
		}	
	}
}

int NextFMVFrame()
{
	D3DLOCKED_RECT textureRect;

	/* lock the d3d texture */
	fmvDynamicTexture->LockRect( 0, &textureRect, NULL, NULL );

	/* write frame image data to texture */
	WriteFmvData((unsigned char*)textureRect.pBits, textureData, imageWidth, imageHeight, textureRect.Pitch);

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
	waveFormat.nChannels		= channels;
	waveFormat.wBitsPerSample	= 16;
	waveFormat.nSamplesPerSec	= rate;	
	waveFormat.nBlockAlign		= waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
	waveFormat.nAvgBytesPerSec	= waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize			= sizeof(waveFormat);

	memset(&bufferFormat, 0, sizeof(DSBUFFERDESC));
	bufferFormat.dwSize			= sizeof(DSBUFFERDESC);
	bufferFormat.dwFlags		= DSBCAPS_CTRLVOLUME | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_LOCSOFTWARE | DSBCAPS_CTRLPAN;
	bufferFormat.dwBufferBytes	= waveFormat.nAvgBytesPerSec / 2;
	bufferFormat.lpwfxFormat	= &waveFormat;

	audioBytesPerSec = (rate * channels * 16 / 8);

	if(FAILED(DSObject->CreateSoundBuffer(&bufferFormat, &fmvAudioBuffer, NULL))) 
	{
///		LogDxErrorString("couldn't create audio buffer for fmv\n");
		OutputDebugString("couldn't create audio buffer for fmv\n");
		return 0;
	}
	else { OutputDebugString("created dsound fmv buffer....\n"); }

	/* lock entire buffer */
	if(FAILED(fmvAudioBuffer->Lock(0, bufferFormat.dwBufferBytes, &fmvaudioPtr1, &fmvaudioBytes1, &fmvaudioPtr2, &fmvaudioBytes2, NULL))) 
	{
		OutputDebugString("\n couldn't lock buffer");
		return 0;
	}

	/* fill entire buffer with silence */
	memset(fmvaudioPtr1, 0, fmvaudioBytes1);

	/* unlock buffer */
	if(FAILED(fmvAudioBuffer->Unlock(fmvaudioPtr1, fmvaudioBytes1, fmvaudioPtr2, fmvaudioBytes2))) 
	{
		OutputDebugString("\n couldn't unlock ds buffer");
		return 0;
	}

	char buf[100];
	sprintf(buf, "DSound fmv buffer size: %d\n", bufferFormat.dwBufferBytes);
	OutputDebugString(buf);

	/* initalise data buffer in ring buffer */
	ring.buffer = new unsigned char[bufferFormat.dwBufferBytes];
	ring.bufferSize = bufferFormat.dwBufferBytes;

	sprintf(buf, "Ring fmv buffer size: %d\n", ring.bufferSize);
	OutputDebugString(buf);

	/* test buffer for lazy copy */
	testBuffer = new unsigned char[bufferFormat.dwBufferBytes];

	return bufferFormat.dwBufferBytes;
}

} // extern "C"

#endif