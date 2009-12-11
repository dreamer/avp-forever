#ifndef LOGSTRING_H
#define LOGSTRING_H

#include <string>
#include <sstream>
#include <fstream>
#include "os_header.h"

void ClearLog();
void WriteToLog(const std::string &logLine);
void LogDxError(HRESULT hr, int LINE, const char* FILE);
void LogErrorString(const std::string &errorString, int LINE, const char* FILE);
void LogErrorString(const std::string &errorString);
std::string IntToString(const int value);
int StringToInt(const std::string &string);
void LogString(const std::string &logString);
void LogDebugValue(int value);
std::string LogInteger(int value);

#endif // include guard