
#ifdef __cplusplus
extern "C" {
#endif

extern void LoadVorbisTrack(int track);
extern void PlayVorbis();
extern void StopVorbis();
extern void UpdateVorbisBuffer(void *arg);
extern bool LoadVorbisTrackList();
bool IsVorbisPlaying();
int CheckNumberOfVorbisTracks();

#ifdef __cplusplus

};
#endif