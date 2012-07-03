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

#include "logString.h"
#include "renderer.h"
#include "utilities.h"

#ifdef _XBOX
	#include "D3dx8core.h"
	std::string logFilename = "d:/avp_log.txt";
#else
	std::string logFilename = "avp_log.txt";
#endif

std::string lastError;

const std::string& GetLastErrorMessage()
{
	return lastError;
}

void ClearLog() 
{
	std::string filePath(GetSaveFolderPath());
	filePath += logFilename;

	std::ofstream file(filePath.c_str(), std::ios::out);
}

void WriteToLog(const std::string &logLine)
{
	std::string filePath(GetSaveFolderPath());
	filePath += logFilename;

	std::ofstream file(filePath.c_str(), std::ios::out | std::ios::app);

	file << logLine;
#if _DEBUG
	OutputDebugString(logLine.c_str());
#endif
	file.close();
}

void LogDxError(HRESULT hr, int LINE, const char* FILE)
{
	std::string temp = "DirectX Error! : ";

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
	temp.append(Util::IntToString(LINE));
	temp.append(" File: ");
	temp.append(FILE);
	temp.append("\n");
	WriteToLog("\t" + temp);

	lastError = temp;
}

// logs a string to file, stating line number of error, and source file it occured in
void LogErrorString(const std::string &errorString, int LINE, const char* FILE)
{
	std::string temp = "Error: " + errorString + " Line: " + Util::IntToString(LINE) + " File: " + FILE + "\n";
	WriteToLog("\t" + temp);

	lastError = temp;
}

// more basic version of above function. just log a string.
void LogErrorString(const std::string &errorString)
{
	std::string temp = "Error: " + errorString + "\n";
	WriteToLog("\t" + temp);

	lastError = temp;
}

void LogDebugString(const std::string &debugString, int LINE, const char* FILE)
{
	std::string temp = "Debug message: " + debugString + " Line: " + Util::IntToString(LINE) + " File: " + FILE + "\n";
	WriteToLog("\t" + temp);
}

void LogDebugString(const std::string &debugString)
{
	// TODO - can specify a flag on cmd args to print these or not?
	std::string temp = debugString + "\n";
	WriteToLog(temp);
}

void LogString(const std::string &logString)
{
	std::string temp = logString + "\n";
	WriteToLog(temp);
}
