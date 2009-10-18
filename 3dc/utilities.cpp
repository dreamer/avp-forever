
#include "utilities.h"
#include <string>
#include "configFile.h"
#include <assert.h>

#ifdef WIN32
	#include <shlobj.h>
	#include <shlwapi.h>
#endif

static char saveFolder[MAX_PATH] = {0};

extern "C" {

char *GetSaveFolderPath()
{
	// check if we've got the path previously and use it again
	if (*saveFolder)
		return saveFolder;

#ifdef _XBOX
	// just blank the char array
	saveFolder[0] = '\0';
	return saveFolder;
#endif
#ifdef WIN32

	if (FAILED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, saveFolder)))
	{
		return 0;
	}

	strcat(saveFolder, "\\My Games");

	// first check if "My Games" folder exists, we try create it..
	if (CreateDirectory(saveFolder, NULL) == 0)
	{
		// something went wrong..
		DWORD error = GetLastError();
		if (error == ERROR_ALREADY_EXISTS)
		{
			// this is fine
		}
		else if (error == ERROR_PATH_NOT_FOUND)
		{
			return NULL;
		}
	}

	strcat(saveFolder, "\\Aliens versus Predator");
	
	// then check Aliens versus Predator
	if (CreateDirectory(saveFolder, NULL) == 0)
	{
		// something went wrong..
		DWORD error = GetLastError();
		if (error == ERROR_ALREADY_EXISTS)
		{
			// this is fine
		}
		else if (error == ERROR_PATH_NOT_FOUND)
		{
			return NULL;
		}
	}

	strcat(saveFolder, "\\");

	// also, create User_Profiles folder if required
	char tempPath[MAX_PATH] = {0};
	strcpy(tempPath, saveFolder);
	strcat(tempPath, "User_Profiles\\");

	if (CreateDirectory(tempPath, NULL) == 0)
	{
		// something went wrong..
		DWORD error = GetLastError();
		if (error == ERROR_ALREADY_EXISTS)
		{
			// this is fine
		}
		else if (error == ERROR_PATH_NOT_FOUND)
		{
			return NULL;
		}
	}

	// we're ok if we're here?
	return saveFolder;
#endif
}

FILE *avp_fopen(const char *fileName, const char *mode)
{
	int blah;
	FILE *theFile = 0;
	std::string finalPath;
#ifdef _XBOX
	finalPath.append("d:\\");
	finalPath.append(fileName);
//	return fopen(finalPath.c_str(), mode);
	theFile = fopen(finalPath.c_str(), mode);

	if (!theFile)
	{
		blah = 0;
	}
	return theFile;
#endif
#ifdef WIN32
	return fopen(fileName, mode);
#endif
}

HANDLE avp_CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	std::string finalPath;

#ifdef _XBOX
	finalPath.append("d:\\");
	finalPath.append(lpFileName);
	return CreateFile(finalPath.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#endif
#ifdef WIN32
	return CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#endif
}

HANDLE avp_FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
	std::string finalPath;
#ifdef _XBOX
	finalPath.append("d:\\");
	finalPath.append(lpFileName);
	return FindFirstFile(finalPath.c_str(), lpFindFileData);
#endif
#ifdef WIN32
	return FindFirstFile(lpFileName, lpFindFileData);
#endif	
}

void avp_GetCommandLineArgs(char *args, int size)
{
	assert (args);
	std::string commandLine = Config_GetString("[Misc]", "CommandLine");
	strcpy(args, commandLine.c_str());
}

};