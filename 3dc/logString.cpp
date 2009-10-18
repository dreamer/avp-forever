
#include "logString.h"
#include "d3_func.h"
#include "utilities.h"

#ifdef _XBOX
	#include "D3dx8core.h"
	std::string logFilename = "d:\\avp_log.txt";
#else
	std::string logFilename = "avp_log.txt";
#endif

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

/* parses an int from a string and returns it */
int StringToInt(const std::string &string)
{
	std::stringstream ss;
	int value = 0;

	/* copy string to stringstream */
	ss << string;
	/* copy from stringstream to int */
	ss >> value;
	
	return value;
}

void ClearLog() 
{
#ifdef WIN32
	TCHAR strPath[MAX_PATH];

	strcpy(strPath, GetSaveFolderPath());
	strcat( strPath, logFilename.c_str());

	std::ofstream file(strPath, std::ios::out);

#else
	std::ofstream file(logFilename.c_str(), std::ios::out);
#endif
}

void WriteToLog(const std::string &logLine)
{
#ifdef WIN32
	TCHAR strPath[MAX_PATH];

	strcpy(strPath, GetSaveFolderPath());
	strcat(strPath, logFilename.c_str());

	std::ofstream file(strPath, std::ios::out | std::ios::app);
#else
	std::ofstream file(logFilename.c_str(), std::ios::out | std::ios::app);
#endif

//	std::ofstream file(logFilename.c_str(), std::ios::out | std::ios::app);
	file << logLine;
#if _DEBUG
	OutputDebugString(logLine.c_str());
#endif
	file.close();
}

void LogDxError(HRESULT hr, int LINE, const char* FILE)
{
	std::string temp = "\t DirectX Error! : ";

#ifdef _XBOX
	char buffer[60];
	D3DXGetErrorString(hr, LPSTR(&buffer), 60);
	temp.append(buffer);
#else
	temp.append(DXGetErrorString(hr));
	temp.append(" - ");
	temp.append(DXGetErrorDescription(hr));
#endif
	temp.append(" Line: ");
	temp.append(IntToString(LINE));
	temp.append(" File: ");
	temp.append(FILE);
	temp.append("\n");
	WriteToLog(temp);
}

/* logs a string to file, stating line number of error, and source file it occured in */
void LogErrorString(const std::string &errorString, int LINE, const char* FILE)
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
/*
void LogMessageBox(const std::string &errorString)
{
#ifdef _WIN32

	MessageBox(hWndMain, errorString.c_str(), "AvP Error", MB_OK+MB_SYSTEMMODAL);

#endif
}
*/
std::string LogInteger(int value)
{
	// returns a string containing value
	std::ostringstream stream;
	stream << value;
	return stream.str();
}