
#ifndef _MusicPlayer_h_
#define _MusicPlayer_h_

#define CDDA_VOLUME_MAX		(127)
#define CDDA_VOLUME_MIN		(0)
#define CDDA_VOLUME_DEFAULT	(127)
#define CDDA_VOLUME_RESTOREPREGAMEVALUE (-100)

extern int musicVolume;

bool Music_Init();
void Music_Deinit();
bool Music_Start();
void Music_Stop();
bool Music_IsPlaying();
void Music_Check();
void Music_CheckVolume();
void Music_ResetForLevel();

#endif
