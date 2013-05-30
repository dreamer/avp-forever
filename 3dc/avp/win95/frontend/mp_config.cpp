#include "3dc.h"
#include "inline.h"
#include "module.h"
#include "strategy_def.h"
#include "gamedef.h"
#include "bh_types.h"
#include "pldnet.h"
#include "mp_config.h"
#include "env_info.h"
#include "menus.h"
#include "list_tem.hpp"
#define UseLocalAssert TRUE
#include "ourasert.h"
#include "FileStream.h"

extern void SetDefaultMultiplayerConfig();
extern char MP_SessionName[];
extern char MP_Config_Description[];
extern char IPAddressString[];

#define MP_CONFIG_DIR "MPConfig"
#define MP_CONFIG_WILDCARD "MPConfig/*.cfg"

#define SKIRMISH_CONFIG_WILDCARD "MPConfig/*.skirmish_cfg"

static List<char*> ConfigurationFilenameList;
static List<char*> ConfigurationLocalisedFilenameList;
char* LastDescriptionFile = 0;
char* LastDescriptionText = 0;

AVPMENU_ELEMENT* AvPMenu_Multiplayer_LoadConfig=0;

BOOL BuildLoadMPConfigMenu()
{
	//delete the old list of filenames
	while (ConfigurationFilenameList.size())
	{
		delete[] ConfigurationFilenameList.first_entry();
		ConfigurationFilenameList.delete_first_entry();
	}
	while (ConfigurationLocalisedFilenameList.size())
	{
		ConfigurationLocalisedFilenameList.delete_first_entry();
	}

	// grab the home directory path
	char filePath[MAX_PATH];
	strcpy(filePath, GetSaveFolderPath());

	// do a search for all the configuration in the configuration directory
	const char *load_name = MP_CONFIG_WILDCARD;
	if (netGameData.skirmishMode) {
		load_name = SKIRMISH_CONFIG_WILDCARD;
	}

	strcat(filePath, load_name);

	// allow a wildcard search
	WIN32_FIND_DATA wfd;

	HANDLE hFindFile = avp_FindFirstFile(filePath, &wfd);
	
	if (INVALID_HANDLE_VALUE == hFindFile) {
		return FALSE;
	}
	
	// get any path in the load_name
	ptrdiff_t nPathLen = 0;
	const char *pColon = strrchr(load_name,':');
	if (pColon) nPathLen = pColon - load_name + 1;
	const char *pBackSlash = strrchr(load_name,'/');
	if (pBackSlash)
	{
		ptrdiff_t nLen = pBackSlash - load_name + 1;
		if (nLen > nPathLen) nPathLen = nLen;
	}
	const char *pSlash = strrchr(load_name,'/');
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
				|FILE_ATTRIBUTE_HIDDEN))
				// not a directory, hidden or system file
		)
		{
			char *name = new char[strlen(wfd.cFileName)+1];
			strcpy(name, wfd.cFileName);
			char *dotpos = strchr(name,'.');
			if (dotpos) {
				*dotpos = 0;
			}

			ConfigurationFilenameList.add_entry(name);

			BOOL localisedFilename = FALSE;
			
			//seeif this is one of the default language localised configurations
			if (!strncmp(name, "Config", 6))
			{
				if (name[6] >= '1' && name[6] <= '7' && name[7] == '\0')
				{
					TEXTSTRING_ID string_index = (TEXTSTRING_ID)(TEXTSTRING_MPCONFIG1_FILENAME+(name[6]-'1'));
					ConfigurationLocalisedFilenameList.add_entry(GetTextString(string_index));
					localisedFilename = TRUE;
				}
			}
			if (!localisedFilename)
			{
				ConfigurationLocalisedFilenameList.add_entry(name);
			}
		}
	
	} while (::FindNextFile(hFindFile, &wfd));
	
	
	if (ERROR_NO_MORE_FILES != GetLastError()) {
		printf("Error finding next file\n");
	}
	
	::FindClose(hFindFile);

	//delete the old menu
	if (AvPMenu_Multiplayer_LoadConfig) {
		delete[] AvPMenu_Multiplayer_LoadConfig;
	}

	AvPMenu_Multiplayer_LoadConfig = 0;

	if (!ConfigurationFilenameList.size()) {
		return FALSE;
	}

	//create a new menu from the list of filenames
	AvPMenu_Multiplayer_LoadConfig = new AVPMENU_ELEMENT[ConfigurationFilenameList.size()+1];

	int i;
	for (i = 0;i < ConfigurationFilenameList.size(); i++)
	{
		AvPMenu_Multiplayer_LoadConfig[i].ElementID = AVPMENU_ELEMENT_LOADMPCONFIG;
		AvPMenu_Multiplayer_LoadConfig[i].TextDescription = TEXTSTRING_BLANK;
		AvPMenu_Multiplayer_LoadConfig[i].MenuToGoTo = AVPMENU_MULTIPLAYER_CONFIG;
		AvPMenu_Multiplayer_LoadConfig[i].TextPtr = ConfigurationLocalisedFilenameList[i];
		AvPMenu_Multiplayer_LoadConfig[i].HelpString = TEXTSTRING_LOADMULTIPLAYERCONFIG_HELP;
	}

	AvPMenu_Multiplayer_LoadConfig[i].ElementID = AVPMENU_ELEMENT_ENDOFMENU;	

	return TRUE;
}

const char* GetMultiplayerConfigDescription(int index)
{
	if (index < 0 || index >= ConfigurationFilenameList.size()) {
		return 0;
	}

	const char* name = ConfigurationFilenameList[index];

	//see if we have already got the description for this file
	if (LastDescriptionFile)
	{
		if (!strcmp(LastDescriptionFile, name))
		{
			return LastDescriptionText;
		}
		else
		{
			delete[] LastDescriptionFile;
			delete[] LastDescriptionText;
			LastDescriptionFile = 0;
			LastDescriptionText = 0;
		}
	}

	LastDescriptionFile = new char[strlen(name)+1];
	strcpy(LastDescriptionFile, name);
	
	// see if this is one of the default language localised configurations
	if (!strncmp(name, "Config", 6))
	{
		if (name[6] >= '1' && name[6] <= '7' && name[7] == '\0')
		{
			TEXTSTRING_ID string_index = (TEXTSTRING_ID)(TEXTSTRING_MPCONFIG1_DESCRIPTION+(name[6]-'1'));
			LastDescriptionText = new char[strlen(GetTextString(string_index))+1];
			strcpy(LastDescriptionText,GetTextString(string_index));
			
			return LastDescriptionText;
		}
	}

	char filename[MAX_PATH];
	if (netGameData.skirmishMode) {
		sprintf(filename, "%s%s/%s.skirmish_cfg", GetSaveFolderPath(), MP_CONFIG_DIR, name);
	}
	else {
		sprintf(filename, "%s%s/%s.cfg", GetSaveFolderPath(), MP_CONFIG_DIR, name);
	}

	FILE* file = avp_fopen(filename, "rb");
	if (!file)
	{
		return 0;
	}

	// skip to the part of the file containing the description
	fseek(file, 169, SEEK_SET);

	int description_length = 0;
	fread(&description_length,sizeof(int),1,file);
	
	if (description_length)
	{
		LastDescriptionText = new char[description_length];
		fread(LastDescriptionText, 1, description_length, file);
	}
	fclose(file);
	
	return LastDescriptionText;
}

void LoadMultiplayerConfigurationByIndex(int index)
{
	if (index < 0 || index >= ConfigurationFilenameList.size()) {
		return;
	}

	LoadMultiplayerConfiguration(ConfigurationFilenameList[index]);	
}

void LoadMultiplayerConfiguration(const char* name)
{
	char filename[MAX_PATH];
	if (netGameData.skirmishMode) {
		sprintf(filename, "%s%s/%s.skirmish_cfg", GetSaveFolderPath(), MP_CONFIG_DIR, name);
	}
	else {
		sprintf(filename, "%s%s/%s.cfg", GetSaveFolderPath(), MP_CONFIG_DIR, name);
	}

	FileStream file;
	file.Open(filename, FileStream::FileRead, FileStream::SkipFastFileCheck);
	if (!file.IsGood()) {
		return;
	}

	// set defaults first, in case there are entries not set by this file
	SetDefaultMultiplayerConfig();

	netGameData.gameType    = (NETGAME_TYPE)file.GetUint32LE();
	netGameData.levelNumber = file.GetInt32LE();
	netGameData.scoreLimit  = file.GetInt32LE();
	netGameData.timeLimit   = file.GetInt32LE();
	netGameData.invulnerableTime = file.GetInt32LE();
	netGameData.characterKillValues[0] = file.GetInt32LE();
	netGameData.characterKillValues[1] = file.GetInt32LE();
	netGameData.characterKillValues[2] = file.GetInt32LE();
	netGameData.baseKillValue          = file.GetInt32LE();
	netGameData.useDynamicScoring      = file.GetInt32LE();
	netGameData.useCharacterKillValues = file.GetInt32LE();

	netGameData.maxMarine   = file.GetInt32LE();
	netGameData.maxAlien    = file.GetInt32LE();
	netGameData.maxPredator = file.GetInt32LE();
	netGameData.maxMarineGeneral    = file.GetInt32LE();
	netGameData.maxMarinePulseRifle = file.GetInt32LE();
	netGameData.maxMarineSmartgun   = file.GetInt32LE();
	netGameData.maxMarineFlamer     = file.GetInt32LE();
	netGameData.maxMarineSadar      = file.GetInt32LE();
	netGameData.maxMarineGrenade    = file.GetInt32LE();
	netGameData.maxMarineMinigun    = file.GetInt32LE();

	netGameData.allowSmartgun = file.GetInt32LE();
	netGameData.allowFlamer   = file.GetInt32LE();
	netGameData.allowSadar    = file.GetInt32LE();
	netGameData.allowGrenadeLauncher = file.GetInt32LE();
	netGameData.allowMinigun  = file.GetInt32LE();
	netGameData.allowDisc     = file.GetInt32LE();
	netGameData.allowPistol   = file.GetInt32LE();
	netGameData.allowPlasmaCaster = file.GetInt32LE();
	netGameData.allowSpeargun = file.GetInt32LE();
	netGameData.allowMedicomp = file.GetInt32LE();

	file.ReadBytes((uint8_t*)MP_SessionName, 13); // BJD - TODO (limit size of MP_SessionName or increase this value (will break compatability..)

	netGameData.maxLives         = file.GetInt32LE();
	netGameData.useSharedLives   = file.GetInt32LE();
	netGameData.pointsForRespawn = file.GetInt32LE();
	netGameData.timeForRespawn   = file.GetInt32LE();
	
	netGameData.aiKillValues[0] = file.GetInt32LE();
	netGameData.aiKillValues[1] = file.GetInt32LE();
	netGameData.aiKillValues[2] = file.GetInt32LE();

	netGameData.gameSpeed = file.GetInt32LE();

	int description_length = file.GetInt32LE();
	file.ReadBytes((uint8_t*)MP_Config_Description, description_length);

	netGameData.preDestroyLights    = file.GetInt32LE();
	netGameData.disableFriendlyFire = file.GetInt32LE();
	netGameData.fallingDamage       = file.GetInt32LE();

#if LOAD_NEW_MPCONFIG_ENTRIES
	netGameData.maxMarineSmartDisc = file.GetInt32LE();
	netGameData.maxMarinePistols   = file.GetInt32LE();
	netGameData.allowSmartDisc     = file.GetInt32LE();
	netGameData.allowPistols       = file.GetInt32LE();
	netGameData.pistolInfiniteAmmo = file.GetInt32LE();
	netGameData.specialistPistols  = file.GetInt32LE();
#endif

	// read the custom level name
	int length = file.GetInt32LE();
	if (length) {
		file.ReadBytes((uint8_t*)netGameData.customLevelName, length);
	}
	else {
		netGameData.customLevelName[0] = 0;
	}

	file.Close();

	//if the custom level name has been set , we need to find the index
	if (netGameData.customLevelName[0])
	{
		netGameData.levelNumber = GetCustomMultiplayerLevelIndex(netGameData.customLevelName,netGameData.gameType);

		if (netGameData.levelNumber < 0)
		{
			//we don't have the level
			netGameData.levelNumber = 0;
			netGameData.customLevelName[0] = 0;
		}
	}
}

void SaveMultiplayerConfiguration(const char* name)
{
	char filename[MAX_PATH];
	if (netGameData.skirmishMode) {
		sprintf(filename, "%s%s/%s.skirmish_cfg", GetSaveFolderPath(), MP_CONFIG_DIR, name);
	}
	else {
		sprintf(filename, "%s%s/%s.cfg", GetSaveFolderPath(), MP_CONFIG_DIR, name);
	}
	
	::CreateDirectory(MP_CONFIG_DIR, 0);

	FileStream file;
	file.Open(filename, FileStream::FileWrite, FileStream::SkipFastFileCheck);
	if (!file.IsGood()) {
		return;
	}

	file.PutUint32LE((uint32_t)netGameData.gameType);
	file.PutInt32LE(netGameData.levelNumber);
	file.PutInt32LE(netGameData.scoreLimit);
	file.PutInt32LE(netGameData.timeLimit);
	file.PutInt32LE(netGameData.invulnerableTime);
	file.PutInt32LE(netGameData.characterKillValues[0]);
	file.PutInt32LE(netGameData.characterKillValues[1]);
	file.PutInt32LE(netGameData.characterKillValues[2]);
	file.PutInt32LE(netGameData.baseKillValue);
	file.PutInt32LE(netGameData.useDynamicScoring);
	file.PutInt32LE(netGameData.useCharacterKillValues);

	file.PutInt32LE(netGameData.maxMarine);
	file.PutInt32LE(netGameData.maxAlien);
	file.PutInt32LE(netGameData.maxPredator);
	file.PutInt32LE(netGameData.maxMarineGeneral);
	file.PutInt32LE(netGameData.maxMarinePulseRifle);
	file.PutInt32LE(netGameData.maxMarineSmartgun);
	file.PutInt32LE(netGameData.maxMarineFlamer);
	file.PutInt32LE(netGameData.maxMarineSadar);
	file.PutInt32LE(netGameData.maxMarineGrenade);
	file.PutInt32LE(netGameData.maxMarineMinigun);

	file.PutInt32LE(netGameData.allowSmartgun);
	file.PutInt32LE(netGameData.allowFlamer);
	file.PutInt32LE(netGameData.allowSadar);
	file.PutInt32LE(netGameData.allowGrenadeLauncher);
	file.PutInt32LE(netGameData.allowMinigun);
	file.PutInt32LE(netGameData.allowDisc);
	file.PutInt32LE(netGameData.allowPistol);
	file.PutInt32LE(netGameData.allowPlasmaCaster);
	file.PutInt32LE(netGameData.allowSpeargun);
	file.PutInt32LE(netGameData.allowMedicomp);

	file.WriteBytes((uint8_t*)&MP_SessionName[0], 13); // BJD - TODO (limit size of MP_SessionName or increase this value (will break compatability..)

	file.PutInt32LE(netGameData.maxLives);
	file.PutInt32LE(netGameData.useSharedLives);
	file.PutInt32LE(netGameData.pointsForRespawn);
	file.PutInt32LE(netGameData.timeForRespawn);
	
	file.PutInt32LE(netGameData.aiKillValues[0]);
	file.PutInt32LE(netGameData.aiKillValues[1]);
	file.PutInt32LE(netGameData.aiKillValues[2]);

	file.PutInt32LE(netGameData.gameSpeed);

	// write the description of the config
	// first the length
	int length = strlen(MP_Config_Description) + 1;
	file.PutInt32LE(length);

	// and now the string
	file.WriteBytes((uint8_t*)MP_Config_Description, length);

	file.PutInt32LE(netGameData.preDestroyLights);
	file.PutInt32LE(netGameData.disableFriendlyFire);
	file.PutInt32LE(netGameData.fallingDamage);

#if SAVE_NEW_MPCONFIG_ENTRIES
	file.PutInt32LE(netGameData.maxMarineSmartDisc);
	file.PutInt32LE(netGameData.maxMarinePistols);
	file.PutInt32LE(netGameData.allowSmartDisc);
	file.PutInt32LE(netGameData.allowPistols);
	file.PutInt32LE(netGameData.pistolInfiniteAmmo);
	file.PutInt32LE(netGameData.specialistPistols);
#endif

	// write the custom level name (if relevant)
	// first the length
	length = strlen(netGameData.customLevelName) + 1;
	file.PutInt32LE(length);

	// and now the string
	file.WriteBytes((uint8_t*)netGameData.customLevelName, length);

	file.Close();

	//clear the last description stuff
	delete[] LastDescriptionFile;
	delete[] LastDescriptionText;
	LastDescriptionFile = 0;
	LastDescriptionText = 0;
}

void DeleteMultiplayerConfigurationByIndex(int index)
{
	if (index < 0 || index >= ConfigurationFilenameList.size()) {
		return;
	}

	char filename[MAX_PATH];
	if (netGameData.skirmishMode) {
		sprintf(filename, "%s%s/%s.skirmish_cfg", GetSaveFolderPath(), MP_CONFIG_DIR, ConfigurationFilenameList[index]);
	}
	else {
		sprintf(filename, "%s%s/%s.cfg", GetSaveFolderPath(), MP_CONFIG_DIR, ConfigurationFilenameList[index]);
	}

	::DeleteFile(filename);
}


#define IP_ADDRESS_DIR "IP_Address"
#define IP_ADDRESS_WILDCARD "IP_Address/*.IP Address"

static List<char*> IPAddFilenameList;

AVPMENU_ELEMENT* AvPMenu_Multiplayer_LoadIPAddress = 0;

BOOL BuildLoadIPAddressMenu()
{
	//delete the old list of filenames
	while (IPAddFilenameList.size())
	{
		delete[] IPAddFilenameList.first_entry();
		IPAddFilenameList.delete_first_entry();
	}

	//do a search for all the addresses in the address directory
	const char* load_name = IP_ADDRESS_WILDCARD;
	// allow a wildcard search
	WIN32_FIND_DATA wfd;

	HANDLE hFindFile = avp_FindFirstFile(load_name, &wfd);

	if (INVALID_HANDLE_VALUE == hFindFile) {
		return FALSE;
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
				|FILE_ATTRIBUTE_HIDDEN))
				// not a directory, hidden or system file
		)
		{
			char *name = new char[strlen(wfd.cFileName)+1];
			strcpy(name, wfd.cFileName);
			char *dotpos = strchr(name,'.');
			if (dotpos) {
				*dotpos = 0;
			}
			IPAddFilenameList.add_entry(name);
		}
	
	} while (::FindNextFile(hFindFile, &wfd));
	
	::FindClose(hFindFile);

	//delete the old menu
	if (AvPMenu_Multiplayer_LoadIPAddress) {
		delete[] AvPMenu_Multiplayer_LoadIPAddress;
	}

	//create a new menu from the list of filenames
	AvPMenu_Multiplayer_LoadIPAddress = new AVPMENU_ELEMENT[IPAddFilenameList.size()+1];

	int i;
	for (i = 0; i < IPAddFilenameList.size(); i++)
	{
		AvPMenu_Multiplayer_LoadIPAddress[i].ElementID = AVPMENU_ELEMENT_LOADIPADDRESS;	
		AvPMenu_Multiplayer_LoadIPAddress[i].TextDescription = TEXTSTRING_BLANK;	
		AvPMenu_Multiplayer_LoadIPAddress[i].MenuToGoTo = AVPMENU_MULTIPLAYERSELECTSESSION;	
		AvPMenu_Multiplayer_LoadIPAddress[i].TextPtr = IPAddFilenameList[i];	
		AvPMenu_Multiplayer_LoadIPAddress[i].HelpString = TEXTSTRING_MULTIPLAYER_LOADADDRESS_HELP;	
	}

	AvPMenu_Multiplayer_LoadIPAddress[i].ElementID = AVPMENU_ELEMENT_ENDOFMENU;	

	return (IPAddFilenameList.size() > 0); 
}


void SaveIPAddress(const char* name, const char* address)
{
	if (!name) return;
	if (!address) return;
	if (!strlen(name)) return;
	if (!strlen(address)) return;

	char filename[MAX_PATH];
	sprintf(filename, "%s/%s.IP Address", IP_ADDRESS_DIR,name);
	
	::CreateDirectory(IP_ADDRESS_DIR, 0);
	FILE* file = avp_fopen(filename,"wb");
	if (!file) return;

	fwrite(address,1,strlen(address)+1,file);
	
	fclose(file);
}

void LoadIPAddress(const char* name)
{
	if (!name) return;

	char filename[MAX_PATH];
	sprintf(filename,"%s/%s.IP Address",IP_ADDRESS_DIR,name);

	FILE* file = avp_fopen(filename,"rb");
	if (!file) return;
	
	fread(IPAddressString,1,16,file);
	IPAddressString[15] = 0;
	
	fclose(file);
}

extern AVPMENU_ELEMENT AvPMenu_Multiplayer_Config[];
extern AVPMENU_ELEMENT AvPMenu_Skirmish_Config[];
extern AVPMENU_ELEMENT AvPMenu_Multiplayer_Config_Join[];


int NumCustomLevels = 0;
int NumMultiplayerLevels = 0;
int NumCoopLevels = 0;

char** MultiplayerLevelNames = 0;
char** CoopLevelNames = 0;

List<char*> CustomLevelNameList;

void ClearMultiplayerLevelNameArray()
{
	for (int i = 0; i < CustomLevelNameList.size(); i++)
	{
		delete[] CustomLevelNameList.first_entry();
		CustomLevelNameList.delete_first_entry();
	}
}

void BuildMultiplayerLevelNameArray()
{
	char buffer[256];

	// only want to do this once
	if (MultiplayerLevelNames) {
		return;
	}

	// first do a search for custom level rifs
	// allow a wildcard search
	WIN32_FIND_DATA wfd;

	const char *load_name = "avp_rifs/custom/*.rif";

	HANDLE hFindFile = avp_FindFirstFile(load_name, &wfd);
	
	if (INVALID_HANDLE_VALUE != hFindFile)
	{
		char *custom_string = GetTextString(TEXTSTRING_CUSTOM_LEVEL); 
		do
		{
			if
			(
				!(wfd.dwFileAttributes &
					(FILE_ATTRIBUTE_DIRECTORY
					|FILE_ATTRIBUTE_SYSTEM
					|FILE_ATTRIBUTE_HIDDEN))
					// not a directory, hidden or system file
			)
			{
				strcpy(buffer, wfd.cFileName);
				char *dotpos = strchr(buffer,'.');
				if (dotpos) {
					*dotpos = 0;
				}
				strcat(buffer," (");
				strcat(buffer,custom_string);
				strcat(buffer,")");

				char *name = new char[strlen(buffer)+1];
				strcpy(name, buffer);

				CustomLevelNameList.add_entry(name);
			}
	
		} while (::FindNextFile(hFindFile, &wfd));

		::FindClose(hFindFile);
	}

	NumCustomLevels = CustomLevelNameList.size();

	NumMultiplayerLevels = MAX_NO_OF_MULTIPLAYER_EPISODES + NumCustomLevels;
	NumCoopLevels = MAX_NO_OF_COOPERATIVE_EPISODES + NumCustomLevels;

	MultiplayerLevelNames = (char**) AllocateMem(sizeof(char*)* NumMultiplayerLevels);

	// first the standard multiplayer levels
	for (int i = 0;i < MAX_NO_OF_MULTIPLAYER_EPISODES; i++)
	{
		char* level_name = GetTextString((TEXTSTRING_ID)(i+TEXTSTRING_MULTIPLAYERLEVELS_1));
		if (i >= 5)
		{
			// a new level
			char *new_string = GetTextString(TEXTSTRING_NEW_LEVEL);
			sprintf(buffer, "%s (%s)", level_name, new_string);

			// allocate memory and copy the string.
			MultiplayerLevelNames[i] = (char*) AllocateMem(strlen(buffer)+1);
			strcpy(MultiplayerLevelNames[i], buffer);
		}
		else {
			MultiplayerLevelNames[i] = level_name;
		}
	}

	CoopLevelNames = (char**) AllocateMem(sizeof(char*)* NumCoopLevels);

	//and the standard coop levels
	for (int i = 0; i < MAX_NO_OF_COOPERATIVE_EPISODES; i++)
	{
		char *level_name = GetTextString((TEXTSTRING_ID)(i+TEXTSTRING_COOPLEVEL_1));
		if (i >= 5)
		{
			//a new level
			char *new_string = GetTextString(TEXTSTRING_NEW_LEVEL);
			sprintf(buffer, "%s (%s)", level_name, new_string);

			//allocate memory and copy the string.
			CoopLevelNames[i] = (char*) AllocateMem(strlen(buffer)+1);
			strcpy(CoopLevelNames[i], buffer);
		}
		else {
			CoopLevelNames[i] = level_name;
		}
	}

	// now add the custom level names
	for (int i = 0; i < NumCustomLevels; i++)
	{
		CoopLevelNames[i+MAX_NO_OF_COOPERATIVE_EPISODES] = CustomLevelNameList[i];
		MultiplayerLevelNames[i+MAX_NO_OF_MULTIPLAYER_EPISODES] = CustomLevelNameList[i];
	}

	// now initialise the environment name entries for the various configuration menus
	AVPMENU_ELEMENT *elementPtr;

	elementPtr = AvPMenu_Multiplayer_Config;

	// search for the level name element
	while (elementPtr->TextDescription != TEXTSTRING_MULTIPLAYER_ENVIRONMENT)
	{
		GLOBALASSERT(elementPtr->ElementID != AVPMENU_ELEMENT_ENDOFMENU);
		elementPtr++;
	}

	elementPtr->MaxSliderValue = NumMultiplayerLevels-1;
	elementPtr->TextSliderStringPointer = MultiplayerLevelNames;
	
	elementPtr = AvPMenu_Multiplayer_Config_Join;
	
	// search for the level name element
	while (elementPtr->TextDescription != TEXTSTRING_MULTIPLAYER_ENVIRONMENT)
	{
		GLOBALASSERT(elementPtr->ElementID != AVPMENU_ELEMENT_ENDOFMENU);
		elementPtr++;
	}
	elementPtr->MaxSliderValue = NumMultiplayerLevels-1;
	elementPtr->TextSliderStringPointer = MultiplayerLevelNames;

	elementPtr = AvPMenu_Skirmish_Config;
	
	// search for the level name element
	while (elementPtr->TextDescription != TEXTSTRING_MULTIPLAYER_ENVIRONMENT)
	{
		GLOBALASSERT(elementPtr->ElementID != AVPMENU_ELEMENT_ENDOFMENU);
		elementPtr++;
	}
	elementPtr->MaxSliderValue = NumMultiplayerLevels-1;
	elementPtr->TextSliderStringPointer = MultiplayerLevelNames;
}

// returns local index of a custom level (if it is a custom level)
int GetCustomMultiplayerLevelIndex(char* name,int gameType)
{
	char buffer[256];
	// tack ( custom) onto the end of the name, before doing the string compare
	char *custom_string = GetTextString(TEXTSTRING_CUSTOM_LEVEL); 
	sprintf(buffer,"%s (%s)", name, custom_string);

	// find the index of a custom level from its name
	if (gameType == NGT_Coop)
	{
		for (int i = MAX_NO_OF_COOPERATIVE_EPISODES; i < NumCoopLevels; i++)
		{
			if (!_stricmp(buffer, CoopLevelNames[i])) {
				return i;
			}
		}
	}
	else
	{
		for (int i = MAX_NO_OF_MULTIPLAYER_EPISODES; i < NumMultiplayerLevels; i++)
		{
			if (!_stricmp(buffer, MultiplayerLevelNames[i])) {
				return i;
			}
		}
	}

	return -1;
}

bool IsCustomLevel(int index, int gameType)
{
	bool isCustom = false;
	if (gameType == NGT_Coop)
	{
		if (index >= MAX_NO_OF_COOPERATIVE_EPISODES) {
			isCustom = true;
		}
	}
	else
	{
		if (index >= MAX_NO_OF_MULTIPLAYER_EPISODES) {
			isCustom = true;
		}
	}
	return isCustom;
}

// returns name of custom level (without stuff tacked on the end)
char *GetCustomMultiplayerLevelName(int index, int gameType)
{
	bool isCustom = false;

	static char return_string[100];
	return_string[0] = 0;
	
	// find the index of a custom level from its name
	if (gameType == NGT_Coop)
	{
		if (index >= MAX_NO_OF_COOPERATIVE_EPISODES) {
			isCustom = true;
		}

		strcpy(return_string, CoopLevelNames[index]);
	}
	else
	{
		if (index >= MAX_NO_OF_MULTIPLAYER_EPISODES) {
			isCustom = true;
		}

		strcpy(return_string, MultiplayerLevelNames[index]);
	}

	// need to remove ' (custom)' from the end of the level name
	if (isCustom) {
		char *bracket_pos = strrchr(return_string,'(');
		if (bracket_pos)
		{
			bracket_pos--; //to get back to the space
			*bracket_pos = 0;
		}
	}

	return return_string;
}


int GetLocalMultiplayerLevelIndex(int index, char* levelName, int gameType)
{
//	if (levelName[0] == 0)
	if (index < 100)
	{
		// not a custom level, just need to check to see if the level index is in range
		if (gameType == NGT_Coop)
		{
			if (index < MAX_NO_OF_COOPERATIVE_EPISODES) {
				return index;
			}
		}
		else
		{
			if (index < MAX_NO_OF_MULTIPLAYER_EPISODES) {
				return index;
			}
		}
 	}
	else
	{
		// see if we have the named level
		return GetCustomMultiplayerLevelIndex(levelName, gameType);
	}

	return -1;
}
