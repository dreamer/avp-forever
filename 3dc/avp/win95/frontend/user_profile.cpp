/* KJL 15:17:31 10/12/98 - user profile stuff */
#include "list_tem.hpp"
#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "strategy_def.h"
#include "user_profile.h"
#include "language.h"
#include "gammacontrol.h"
#include "psnd.h"
#include "MusicPlayer.h"
#define UseLocalAssert TRUE
#include "ourasert.h"
#include "pldnet.h"
#include <time.h>
#include "utilities.h"

static int LoadUserProfiles(void);

void EmptyUserProfilesList();
static int MakeNewUserProfile(void);
static void InsertProfileIntoList(AVP_USER_PROFILE *profilePtr);
static int ProfileIsMoreRecent(AVP_USER_PROFILE *profilePtr, AVP_USER_PROFILE *profileToTestAgainstPtr);
static void SetDefaultProfileOptions(AVP_USER_PROFILE *profilePtr);

extern int FmvSoundVolume;
extern int EffectsSoundVolume;
extern int MoviesAreActive;
extern int IntroOutroMoviesAreActive;
extern char MP_PlayerName[];
extern int AutoWeaponChangeOn;


List<AVP_USER_PROFILE *> UserProfilesList;
static AVP_USER_PROFILE DefaultUserProfile =
{
	"",
};
static AVP_USER_PROFILE *CurrentUserProfilePtr;

extern void ExamineSavedUserProfiles(void)
{
	// delete any existing profiles
	EmptyUserProfilesList();

	LoadUserProfiles();

	// this creates the "New Profile" entry which allows you to make new profiles ingame
	AVP_USER_PROFILE *profilePtr = new AVP_USER_PROFILE;
	*profilePtr = DefaultUserProfile;

	::GetLocalTime(&profilePtr->TimeLastUpdated);
	::SystemTimeToFileTime(&profilePtr->TimeLastUpdated, &profilePtr->FileTime);

	strncpy(profilePtr->Name, GetTextString(TEXTSTRING_USERPROFILE_NEW), MAX_SIZE_OF_USERS_NAME);
	profilePtr->Name[MAX_SIZE_OF_USERS_NAME] = 0;
	SetDefaultProfileOptions(profilePtr);

	InsertProfileIntoList(profilePtr);
}

extern int NumberOfUserProfiles(void)
{
	int n = UserProfilesList.size();

	LOCALASSERT(n>0);

	return n-1;
}

extern AVP_USER_PROFILE *GetFirstUserProfile(void)
{
	CurrentUserProfilePtr=UserProfilesList.first_entry();
	return CurrentUserProfilePtr;
}

extern AVP_USER_PROFILE *GetNextUserProfile(void)
{
	if (CurrentUserProfilePtr == UserProfilesList.last_entry())
	{
		CurrentUserProfilePtr = UserProfilesList.first_entry();
	}
	else
	{
		CurrentUserProfilePtr = UserProfilesList.next_entry(CurrentUserProfilePtr);
	}
	return CurrentUserProfilePtr;
}

void EmptyUserProfilesList()
{
	while (UserProfilesList.size())
	{
		delete UserProfilesList.first_entry();
		UserProfilesList.delete_first_entry();
	}
}

extern int SaveUserProfile(AVP_USER_PROFILE *profilePtr)
{
	// avp_fopen will add SaveFolderPath for us
	char *filename = new char [strlen(USER_PROFILES_PATH) + strlen(profilePtr->Name) + strlen(USER_PROFILES_SUFFIX) + 1];

	strcpy(filename, USER_PROFILES_PATH);
	strcat(filename, profilePtr->Name);
	strcat(filename, USER_PROFILES_SUFFIX);

	FILE* file = avp_fopen(filename, "wb");

	delete[] filename;

	if (!file)
		return 0;

	SaveSettingsToUserProfile(profilePtr);

	fwrite(profilePtr, sizeof(AVP_USER_PROFILE), 1, file);
	fclose(file);

	return 1;
}

extern void DeleteUserProfile(int number)
{
	AVP_USER_PROFILE *profilePtr = GetFirstUserProfile();

	for (int i=0; i<number; i++) profilePtr = GetNextUserProfile();

	char *savePath = GetSaveFolderPath();
	size_t savePathLength = strlen(savePath);

	size_t fileNameLength = savePathLength + strlen(USER_PROFILES_PATH) + strlen(profilePtr->Name) + strlen(USER_PROFILES_SUFFIX)+1;
	if (fileNameLength > MAX_PATH)
	{

	}

	char *filename = new char[savePathLength + strlen(USER_PROFILES_PATH)+strlen(profilePtr->Name)+strlen(USER_PROFILES_SUFFIX)+1];
	strcpy(filename, savePath);
	strcat(filename, USER_PROFILES_PATH);
	strcat(filename, profilePtr->Name);
	strcat(filename, USER_PROFILES_SUFFIX);

	::DeleteFile(filename);

	// now delete all the user's savegame files
	delete[] filename;

	filename = new char[MAX_PATH];

	for (int i=0; i < NUMBER_OF_SAVE_SLOTS; i++)
	{
		sprintf(filename, "%s%s%s_%d.sav", savePath, USER_PROFILES_PATH, profilePtr->Name, i+1);
		::DeleteFile(filename);
	}

	delete[] filename;
}

static void InsertProfileIntoList(AVP_USER_PROFILE *profilePtr)
{
	if (UserProfilesList.size())
	{
		AVP_USER_PROFILE *profileInListPtr = GetFirstUserProfile();

		for (int i=0; i < UserProfilesList.size(); i++, profileInListPtr = GetNextUserProfile())
		{
			if (ProfileIsMoreRecent(profilePtr, profileInListPtr))
			{
				UserProfilesList.add_entry_before(profilePtr, profileInListPtr);
				return;
			}
		}
	}
	UserProfilesList.add_entry(profilePtr);
}

static int ProfileIsMoreRecent(AVP_USER_PROFILE *profilePtr, AVP_USER_PROFILE *profileToTestAgainstPtr)
{
	if (::CompareFileTime(&profilePtr->FileTime, &profileToTestAgainstPtr->FileTime) == 1)
	{
		return 1;
	}
		
	return 0;
}

#include "FileStream.h"

static int LoadUserProfiles(void)
{
#if 0 // filestream test
	std::vector<std::string> profileFiles;
	std::string path = GetSaveFolderPath();
	path += "User_Profiles/";

//	path += USER_PROFILES_WILDCARD_NAME;

	if (FindFiles(path + "*.prf", profileFiles))
	{
		for (size_t i = 0; i < profileFiles.size(); i++)
		{
			FileStream findFile;

			findFile.Open(path + profileFiles[i], FileStream::FileRead, true);
			if (!findFile.IsGood())
				continue;

			AVP_USER_PROFILE *profilePtr = new AVP_USER_PROFILE;

			// read in profile file
			findFile.ReadBytes((uint8_t*)profilePtr, sizeof(AVP_USER_PROFILE));

			// do extra stuff
/* TODO
			FILETIME ftLocal;
			FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &ftLocal);
			FileTimeToSystemTime(&ftLocal, &profilePtr->TimeLastUpdated);
			profilePtr->FileTime = ftLocal;
*/
			InsertProfileIntoList(profilePtr);
		}
	}

	return 1;
#else

	TCHAR strPath[MAX_PATH];

	strcpy(strPath, GetSaveFolderPath());
	strcat(strPath, USER_PROFILES_WILDCARD_NAME);

	const char* load_name = &strPath[0];

	// allow a wildcard search
	WIN32_FIND_DATA wfd;

	HANDLE hFindFile = avp_FindFirstFile(load_name, &wfd);
	if (INVALID_HANDLE_VALUE == hFindFile)
	{
//		printf("File Not Found: <%s>\n",load_name);
		return 0;
	}

	// get any path in the load_name
	ptrdiff_t nPathLen = 0;
	const char * pColon = strrchr(load_name,':');
	if (pColon) nPathLen = pColon - load_name + 1;
	const char * pBackSlash = strrchr(load_name,'/');
	if (pBackSlash)
	{
		ptrdiff_t nLen = pBackSlash - load_name + 1;
		if (nLen > nPathLen) nPathLen = nLen;
	}
	const char * pSlash = strrchr(load_name,'/');
	if (pSlash)
	{
		ptrdiff_t nLen = pSlash - load_name + 1;
		if (nLen > nPathLen) nPathLen = nLen;
	}

	do
	{
		if
		(
			!(wfd.dwFileAttributes &
				(FILE_ATTRIBUTE_DIRECTORY
				|FILE_ATTRIBUTE_SYSTEM
				|FILE_ATTRIBUTE_HIDDEN
				|FILE_ATTRIBUTE_READONLY))
				// not a directory, hidden or system file
		)
		{
			char * pszFullPath = new char [nPathLen + strlen(wfd.cFileName)+1];
			strncpy(pszFullPath,load_name,nPathLen);
			strcpy(pszFullPath + nPathLen,wfd.cFileName);

			//make sure the file is a rif file
			HANDLE rif_file;
			rif_file = avp_CreateFile(pszFullPath, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, 0);
			if (rif_file==INVALID_HANDLE_VALUE)
			{
//				printf("couldn't open %s\n",pszFullPath);
				delete[] pszFullPath;
				continue;
			}

			AVP_USER_PROFILE *profilePtr = new AVP_USER_PROFILE;
			DWORD bytes_read;

			if (!ReadFile(rif_file, profilePtr, sizeof(AVP_USER_PROFILE), &bytes_read, 0))
			{
				CloseHandle (rif_file);
				delete[] pszFullPath;
				delete profilePtr;
				continue;
			}

			FILETIME ftLocal;
			FileTimeToLocalFileTime(&wfd.ftLastWriteTime, &ftLocal);
			FileTimeToSystemTime(&ftLocal, &profilePtr->TimeLastUpdated);
			profilePtr->FileTime = ftLocal;
			InsertProfileIntoList(profilePtr);
			CloseHandle (rif_file);
			delete[] pszFullPath;
		}
	}
	while (::FindNextFile(hFindFile, &wfd));

	if (ERROR_NO_MORE_FILES != GetLastError())
	{
		//	printf("Error finding next file\n");
	}

	::FindClose(hFindFile);

	return 1;
#endif
}

static void SetDefaultProfileOptions(AVP_USER_PROFILE *profilePtr)
{
	// set Gamma
	RequestedGammaSetting = 128;
	
	// controls
	MarineInputPrimaryConfig     = DefaultMarineInputPrimaryConfig;
	MarineInputSecondaryConfig   = DefaultMarineInputSecondaryConfig;
	PredatorInputPrimaryConfig   = DefaultPredatorInputPrimaryConfig;
	PredatorInputSecondaryConfig = DefaultPredatorInputSecondaryConfig;
	AlienInputPrimaryConfig      = DefaultAlienInputPrimaryConfig;
	AlienInputSecondaryConfig    = DefaultAlienInputSecondaryConfig;
	ControlMethods               = DefaultControlMethods;
	JoystickControlMethods       = DefaultJoystickControlMethods;

	FmvSoundVolume     = ONE_FIXED/512;
	EffectsSoundVolume = VOLUME_DEFAULT;
	musicVolume        = CDDA_VOLUME_DEFAULT;
	MoviesAreActive    = 1;
	AutoWeaponChangeOn = TRUE;
	IntroOutroMoviesAreActive = 1;

	strcpy(MP_PlayerName, "DeadMeat");

	SetToDefaultDetailLevels();

	for (int a=0; a < I_MaxDifficulties; a++)
	{
		for (int b=0; b < AVP_ENVIRONMENT_END_OF_LIST; b++)
		{
			profilePtr->PersonalBests[a][b] = DefaultLevelGameStats;
		}
	}

	SaveSettingsToUserProfile(profilePtr);
}

extern void GetSettingsFromUserProfile(void)
{
	RequestedGammaSetting = UserProfilePtr->GammaSetting;

	MarineInputPrimaryConfig      = UserProfilePtr->MarineInputPrimaryConfig;
	MarineInputSecondaryConfig    = UserProfilePtr->MarineInputSecondaryConfig;
	PredatorInputPrimaryConfig    = UserProfilePtr->PredatorInputPrimaryConfig;
	PredatorInputSecondaryConfig  = UserProfilePtr->PredatorInputSecondaryConfig;
	AlienInputPrimaryConfig       = UserProfilePtr->AlienInputPrimaryConfig;
	AlienInputSecondaryConfig     = UserProfilePtr->AlienInputSecondaryConfig;
	ControlMethods                = UserProfilePtr->ControlMethods;
	JoystickControlMethods        = UserProfilePtr->JoystickControlMethods;
	MenuDetailLevelOptions        = UserProfilePtr->DetailLevelSettings;
	FmvSoundVolume                = UserProfilePtr->FmvSoundVolume;
	EffectsSoundVolume            = UserProfilePtr->EffectsSoundVolume;
	musicVolume                   = UserProfilePtr->MusicVolume;
	MoviesAreActive               = UserProfilePtr->MoviesAreActive;
	IntroOutroMoviesAreActive     = UserProfilePtr->IntroOutroMoviesAreActive;
	AutoWeaponChangeOn            = !UserProfilePtr->AutoWeaponChangeDisabled;
	strncpy(MP_PlayerName, UserProfilePtr->MultiplayerCallsign, 15);

	SetDetailLevelsFromMenu();
}


extern void SaveSettingsToUserProfile(AVP_USER_PROFILE *profilePtr)
{
	profilePtr->GammaSetting = RequestedGammaSetting;

	profilePtr->MarineInputPrimaryConfig     = MarineInputPrimaryConfig;
	profilePtr->MarineInputSecondaryConfig   = MarineInputSecondaryConfig;
	profilePtr->PredatorInputPrimaryConfig   = PredatorInputPrimaryConfig;
	profilePtr->PredatorInputSecondaryConfig = PredatorInputSecondaryConfig;
	profilePtr->AlienInputPrimaryConfig      = AlienInputPrimaryConfig;
	profilePtr->AlienInputSecondaryConfig    = AlienInputSecondaryConfig;
	profilePtr->ControlMethods               = ControlMethods;
	profilePtr->JoystickControlMethods       = JoystickControlMethods;
	profilePtr->DetailLevelSettings          = MenuDetailLevelOptions;
	profilePtr->FmvSoundVolume               = FmvSoundVolume;
	profilePtr->EffectsSoundVolume           = EffectsSoundVolume;
	profilePtr->MusicVolume                  = musicVolume;
	profilePtr->MoviesAreActive              = MoviesAreActive;
	profilePtr->IntroOutroMoviesAreActive    = IntroOutroMoviesAreActive;
	profilePtr->AutoWeaponChangeDisabled     = !AutoWeaponChangeOn;
	strncpy(profilePtr->MultiplayerCallsign, MP_PlayerName, 15);
}

extern void FixCheatModesInUserProfile(AVP_USER_PROFILE *profilePtr)
{
	for (int a=0; a < MAX_NUMBER_OF_CHEATMODES; a++) 
	{
		if (profilePtr->CheatMode[a] == 2)
		{
			profilePtr->CheatMode[a] = 1;
		}
	}
}
