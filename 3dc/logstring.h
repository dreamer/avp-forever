#ifndef h_logstring
#define h_logstring

#include <string>
#include <sstream>
#include <fstream>
#include <windows.h>

void ClearLog();
void WriteToLog(const std::string &logLine);
void LogDxError(/*std::string errorString,*/ HRESULT hr);
void LogDxErrorString(const std::string &errorString);
void LogDxString(const std::string &logString);
void LogDebugValue(int value);
std::string LogInteger(int value);

#endif // include guard