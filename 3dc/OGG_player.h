
#ifdef __cplusplus
extern "C" {
#endif

// I really need to settle on a naming convention for functions..
extern void loadOgg(int track);
extern void playOgg();
extern void stopOgg();
extern void updateOggBuffer();
extern bool loadOggTrackList();
bool oggIsPlaying();
int OGG_CheckNumberOfTracks();

#ifdef __cplusplus

};
#endif