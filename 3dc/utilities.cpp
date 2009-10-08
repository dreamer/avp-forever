
#include "utilities.h"
#include <string>
#include "configFile.h"
#include <assert.h>

extern "C" {

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