#ifndef WIN32
extern "C" {

int SmackerSoundVolume=65536/512;
int MoviesAreActive;
int IntroOutroMoviesAreActive=1;
int FmvColourRed;
int FmvColourGreen;
int FmvColourBlue;

extern void EndMenuBackgroundBink()
{
}

extern void StartMenuBackgroundBink()
{
}

extern void PlayBinkedFMV(char *filenamePtr)
{
}

extern int PlayMenuBackgroundBink()
{
	return 0;
}

void UpdateAllFMVTextures()
{
}

extern void StartTriggerPlotFMV(int number)
{
}

void StartMenuMusic()
{
}

extern void StartFMVAtFrame(int number, int frame)
{ 
}

void ScanImagesForFMVs()
{
}

void ReleaseAllFMVTextures()
{
}

void PlayMenuMusic()
{
}

extern void InitialiseTriggeredFMVs()
{
}

void EndMenuMusic()
{
}

extern void GetFMVInformation(int *messageNumberPtr, int *frameNumberPtr)
{
}

void CloseFMV()
{
}

} // extern "C"

void LoadTheora()
{

}
#endif