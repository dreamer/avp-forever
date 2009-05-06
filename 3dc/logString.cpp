
#include "logString.h"

std::string logFilename = "dx_log.txt";

void ClearLog() 
{
	std::ofstream file(logFilename.c_str(), std::ios::out );
}

void WriteToLog(const std::string &logLine)
{
	std::ofstream file(logFilename.c_str(), std::ios::out | std::ios::app );
	file << logLine;
	file.close();
}

void LogDxError(/*std::string errorString,*/ HRESULT hr)
{
/*
	std::string temp = "\t Error!: ";
	temp.append(DXGetErrorString9(hr));
	temp.append(" - ");
	temp.append(DXGetErrorDescription9(hr));
	temp.append("\n");
	WriteToLog(temp);
	return;
*/
}

void LogDxErrorString(const std::string &errorString)
{
	std::string temp = "\t" + errorString;
#ifdef _DEBUG
	OutputDebugString(temp.c_str());
#endif
	WriteToLog(temp);
}

void LogDxString(const std::string &logString)
{
	std::string temp = logString + "\n";
	WriteToLog(temp);
}

void LogDebugValue(int value)
{
	std::ostringstream stream;
	stream << "\n value was: " << value;
	OutputDebugString(stream.str().c_str());
}

std::string LogInteger(int value)
{
	// returns a string containing value
	std::ostringstream stream;
	stream << value;
	return stream.str();
}