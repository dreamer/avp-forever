#ifndef LOGSTRING_H
#define LOGSTRING_H

#include <string>
#include <sstream>
#include <fstream>

#ifdef WIN32
	#include <windows.h>
#endif
#ifdef _XBOX
	#include <xtl.h>
#endif

void ClearLog();
void WriteToLog(const std::string &logLine);
void LogDxError(HRESULT hr, int LINE, const char* FILE);
void LogErrorString(const std::string &errorString, int LINE, const char* FILE);
void LogErrorString(const std::string &errorString);
std::string IntToString(const int value);
void LogString(const std::string &logString);
void LogDebugValue(int value);
std::string LogInteger(int value);
//void LogMessageBox(const std::string &errorString);

#endif // include guard