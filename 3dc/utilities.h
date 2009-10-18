#ifndef _avp_utilities_h_
#define _avp_utilities_h_

#include <stdio.h>
#include <string.h>
#ifdef WIN32
	#include <windows.h>
#endif
#ifdef _XBOX
	#include <xtl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

FILE *avp_fopen(const char *fileName, const char *mode);
HANDLE avp_CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HANDLE avp_FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
void avp_GetCommandLineArgs(char *args, int size);
char *GetSaveFolderPath();

#ifdef __cplusplus
};
#endif

#endif