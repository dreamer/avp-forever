// Copyright (C) 2010 Barry Duncan. All Rights Reserved.
// The original author of this code can be contacted at: bduncan22@hotmail.com

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

extern void ReleaseDirect3D();

#ifdef WIN32
	extern HWND	hWndMain;
#endif

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

	// lets create MPConfig folder too
	tempPath[0] = '\0';
	strcpy(tempPath, saveFolder);
	strcat(tempPath, "MPConfig\\");

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
//	FILE *theFile = 0;
	std::string finalPath;

#ifdef _XBOX

	finalPath.append("d:\\");
	finalPath.append(fileName);
/*
	// if write mode, direct to home path
	if (strcmp(mode, "wb") == 0)
	{
		finalPath.append("d:\\");
		finalPath.append(fileName);
	}
	else
	{
		finalPath += fileName;
	}
*/
	return fopen(finalPath.c_str(), mode);
#endif
#ifdef WIN32

	// if write mode, direct to home path
	if ((strcmp(mode, "wb") == 0) || (strcmp(mode, "w") == 0))
	{
		finalPath += GetSaveFolderPath();
		finalPath += fileName;
	}
	else
	{
		finalPath += fileName;
	}

	return fopen(finalPath.c_str(), mode);
#endif
}

DWORD avp_GetFileAttributes(LPCTSTR lpFileName)
{
#ifdef _XBOX

	std::string finalPath;

	finalPath.append("d:\\");
	finalPath.append(lpFileName);

	return GetFileAttributes(finalPath.c_str());
#endif
#ifdef WIN32
	return GetFileAttributes(lpFileName);
#endif
}

HANDLE avp_CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
#ifdef _XBOX

	std::string finalPath;

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
#ifdef _XBOX

	std::string finalPath;

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
	std::string commandLine = Config_GetString("[Misc]", "CommandLine", "");
	strcpy(args, commandLine.c_str());
}

void avp_MessageBox(const char* message, int type)
{
#ifdef WIN32
	ReleaseDirect3D();
	MessageBox(hWndMain, message, "AvP Error", type);
#endif
}

void avp_exit(int code)
{
#ifdef WIN32
	exit(code);
#endif
}

}
