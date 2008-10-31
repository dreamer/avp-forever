
#ifdef __cplusplus
extern "C" {
#endif

// I really need to settle on a naming convention for functions..
extern void LoadVorbisTrack(int track);
extern void PlayVorbis();
extern void StopVorbis();
extern void UpdateVorbisBuffer();
extern bool LoadVorbisTrackList();
bool IsVorbisPlaying();
int CheckNumberOfVorbisTracks();

#ifdef __cplusplus

};
#endif