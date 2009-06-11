
#include "logString.h"
#include "d3_func.h"

#ifdef _XBOX
#include "D3dx8core.h"
#endif

std::string logFilename = "avp_log.txt";

/* converts an int to a string and returns it */
std::string IntToString(const int value)
{
	std::stringstream ss;
	std::string temp;

	/* copy int to stringstream */
	ss << value;
	/* copy from stringstream to string */
	ss >> temp;
	
	return temp;
}

void ClearLog() 
{
	std::ofstream file(logFilename.c_str(), std::ios::out );
}

void WriteToLog(const std::string &logLine)
{
	std::ofstream file(logFilename.c_str(), std::ios::out | std::ios::app );
	file << logLine;
#if _DEBUG
	OutputDebugString(logLine.c_str());
#endif
	file.close();
}

void LogDxError(HRESULT hr, int LINE, char* FILE)
{
	std::string temp = "\t DirectX Error!: ";

#ifdef _XBOX
	char buffer[60];
	D3DXGetErrorString(hr, LPSTR(&buffer), 60);
	temp.append(buffer);
#else
	temp.append(DXGetErrorString9(hr));
	temp.append(" - ");
	temp.append(DXGetErrorDescription9(hr));
#endif
	temp.append( "Line: ");
	temp.append(IntToString(LINE));
	temp.append(" File: ");
	temp.append(FILE);
	temp.append("\n");
	WriteToLog(temp);
}

/* logs a string to file, stating line number of error, and source file it occured in */
void LogErrorString(const std::string &errorString, int LINE, char* FILE)
{
	std::string temp = "\t Error: " + errorString + " Line: " + IntToString(LINE) + " File: " + FILE + "\n";
	WriteToLog(temp);
}

/* more basic version of above function. just log a string. */
void LogErrorString(const std::string &errorString)
{
	std::string temp = "\t Error: " + errorString + "\n";
	WriteToLog(temp);
}

void LogString(const std::string &logString)
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