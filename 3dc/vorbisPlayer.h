#ifndef _vorbisPlayer_h_
#define _vorbisPlayer_h_

#include "os_header.h"

#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include <vorbis/vorbisfile.h>
#include "audioStreaming.h"
#include <string>

struct VorbisCodec
{
	FILE *file;
	vorbis_info *pInfo;
	OggVorbis_File oggFile;
	StreamingAudioBuffer *audioStream;
	bool oggIsPlaying;

	unsigned int bufferSize;
	unsigned int halfBufferSize;

	HANDLE hPlaybackThreadFinished;

	byte *audioData;
};

VorbisCodec * Vorbis_LoadFile(const std::string &fileName);
void Vorbis_Release(VorbisCodec *VorbisStream);

#ifdef __cplusplus
extern "C" {
#endif

void Vorbis_CloseSystem();
extern void LoadVorbisTrack(int track);
extern bool LoadVorbisTrackList();
bool IsVorbisPlaying();
int CheckNumberOfVorbisTracks();

#ifdef __cplusplus
}
#endif

#endif