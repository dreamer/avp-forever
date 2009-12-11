#ifndef _avp_utilities_h_
#define _avp_utilities_h_

#include <stdio.h>
#include <string.h>
#include "os_header.h"

#ifdef __cplusplus
extern "C" {
#endif

FILE *avp_fopen(const char *fileName, const char *mode);
HANDLE avp_CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HANDLE avp_FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
DWORD avp_GetFileAttributes(LPCTSTR lpFileName);
void avp_GetCommandLineArgs(char *args, int size);
char *GetSaveFolderPath();

#ifdef __cplusplus
};
#endif

#endif