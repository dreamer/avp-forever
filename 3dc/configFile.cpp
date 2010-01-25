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

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>

#include "logString.h"
#include "utilities.h"
#include "configFile.h"

extern int StringToInt(const std::string &string);
extern std::string IntToString(const int value);

static bool Config_CreateDefault();

typedef std::map<std::string, std::string> MapValue;
typedef std::map<std::string, MapValue> MapHeading;

#ifdef _XBOX
	const char* FILENAME = "d:\\AliensVsPredator.cfg";
#else
	const char* FILENAME = "AliensVsPredator.cfg";
#endif

MapHeading AvPConfig;

bool Config_Load()
{
	std::string filePath(GetSaveFolderPath());
	filePath += FILENAME;

	std::ifstream file(filePath.c_str());

	std::string tempLine;
	std::string currentHeading;

	// open the config file firstly
	if (!file.is_open())
	{
		LogErrorString("Can't find file AliensVsPredator.cfg - creating file with default values");
		if (!Config_CreateDefault())
		{
			LogErrorString("Error creating config file!");
			return false;
		}
		else
		{
			file.open(filePath.c_str());
			if (!file.is_open())
			{
				LogErrorString("Error opening config file!");
				return false;
			}
			file.seekg(0, std::ios::beg);
		}
	}

	// go through the cfg file line by line
	while (getline(file, tempLine))
	{	
		if (tempLine.length() == 0) // skip empty lines
			continue;

		if ((tempLine.at(0) == '[') || (tempLine.at(tempLine.length() - 1) == ']')) // found a header block
		{
			//std::cout << "Found header: " << tempLine << "\n";
			currentHeading = tempLine;
		}
		else
		{
			// special case for strings such as command line
			int stringCheck = tempLine.find('"'); // check for a quote..

			if (stringCheck != std::string::npos)
			{
				// we want to remove the quotes
				//tempLine.erase(std::remove(tempLine.begin(), tempLine.end(),'"'), tempLine.end());

				// and also, the whitespace in the pre quotes section
				std::string tempString = tempLine.substr(0, stringCheck);
				std::string tempString2 = tempLine.substr(stringCheck, tempLine.length());

				// remove spaces
				tempString.erase(std::remove(tempString.begin(), tempString.end(),' '), tempString.end());

				// remove quotes
//				tempString2.erase(std::remove(tempString2.begin(), tempString2.end(),'"'), tempString2.end());

				// recreate original line string
				tempLine = tempString + tempString2;
			}
			else
			{
				// remove whitespace
				tempLine.erase(std::remove(tempLine.begin(), tempLine.end(),' '), tempLine.end());
			}

			// assume we got a variable and value
			int lenOfVar = tempLine.find("=");

			// if there's no equals sign in the string, don't add it
			if (lenOfVar == std::string::npos)
				continue;

//			std::cout << "got variable name: " << tempLine.substr(0, lenOfVar) << "\n";
//			std::cout << "its value is: " << tempLine.substr(lenOfVar + 1) << "\n";

			// should only create a new key in AvPConfig if one doesn't already exists
			MapValue &tempValue = AvPConfig[currentHeading];

			// +1 to skip over the equals sign
			tempValue.insert(std::make_pair(tempLine.substr(0, lenOfVar),
												tempLine.substr(lenOfVar + 1)));
		}
	}

	LogString("Loaded config file");
	
	return true;
}

bool Config_Save()
{
	std::string filePath(GetSaveFolderPath());
	filePath += FILENAME;

	std::ofstream file(filePath.c_str());

	if (!file.is_open())
	{
		LogErrorString("Error opening config file for save!");
		return false;
	}

	MapHeading::iterator headingIt = AvPConfig.begin();

	while (headingIt != AvPConfig.end())
	{
		file << (*headingIt).first << "\n";
		
		MapValue::iterator variableIt = (*headingIt).second.begin();

		while (variableIt != (*headingIt).second.end())
		{
			file << (*variableIt).first << " = " << (*variableIt).second << "\n";
			variableIt++;
		}
		
		headingIt++;
		file << "\n";
	}
	return true;
}

bool CheckValuesExist(const std::string &heading, const std::string &variable)
{
	MapHeading::iterator headingIt = AvPConfig.find(heading);

	if (headingIt == AvPConfig.end())
	{
		LogErrorString("Heading " + heading + " not found!");
		return false;
	}

	MapValue::iterator variableIt = headingIt->second.find(variable);

	if (variableIt == headingIt->second.end())
	{
		LogErrorString("Variable " + variable + " not found!");
		return false;
	}

	return true;
}

int Config_GetInt(const std::string &heading, const std::string &variable, int defaultValue)
{
	if (CheckValuesExist(heading, variable))
	{
		return StringToInt(AvPConfig.find(heading)->second.find(variable)->second);
	}
	else
	{
		// should we be adding this to the map if it doesn't exist using default value? i guess so..
		MapValue &tempValue = AvPConfig[heading];
		tempValue.insert(std::make_pair(variable, IntToString(defaultValue)));

		return defaultValue;
	}
}

void Config_SetInt(const std::string &heading, const std::string &variable, int newValue)
{
	if (CheckValuesExist(heading, variable))
	{
		AvPConfig.find(heading)->second.find(variable)->second = IntToString(newValue);
	}
}

void Config_SetString(const std::string &heading, const std::string &variable, const std::string newValue)
{
	if (CheckValuesExist(heading, variable))
	{
		AvPConfig.find(heading)->second.find(variable)->second = newValue;
	}
}

std::string Config_GetString(const std::string &heading, const std::string &variable, const std::string &defaultString)
{
	if (CheckValuesExist(heading, variable))
	{
		// grab a copy of the string
		std::string tempString = AvPConfig.find(heading)->second.find(variable)->second;
	
		// if it has quotes, remove them
		tempString.erase(std::remove(tempString.begin(), tempString.end(),'"'), tempString.end());

		return tempString;
	}
	else
	{
		// should we be adding this to the map if it doesn't exist using default value? i guess so..
		MapValue &tempValue = AvPConfig[heading];
		tempValue.insert(std::make_pair(variable, defaultString));

		return defaultString;
	}
}

bool Config_GetBool(const std::string &heading, const std::string &variable, bool defaultValue)
{
	// just treat it as a string I guess?
	if (CheckValuesExist(heading, variable))
	{
		// grab a copy of the string
		std::string tempString = AvPConfig.find(heading)->second.find(variable)->second;
	
		// if it has quotes, remove them
		tempString.erase(std::remove(tempString.begin(), tempString.end(),'"'), tempString.end());

		if (tempString == "true")
		{
			return true;
		}
		else if (tempString == "false")
		{
			return false;
		}
	}
	else
	{
		// should we be adding this to the map if it doesn't exist using default value? i guess so..
		MapValue &tempValue = AvPConfig[heading];

		if (defaultValue == true)
		{
			tempValue.insert(std::make_pair(variable, "true"));
		}
		else if (defaultValue == false)
		{
			tempValue.insert(std::make_pair(variable, "false"));
		}
	}

	return defaultValue;
}

void Config_SetBool(const std::string &heading, const std::string &variable, bool newValue)
{
	if (CheckValuesExist(heading, variable))
	{
		if (newValue == true)
		{
			AvPConfig.find(heading)->second.find(variable)->second = "true";
		}
		else if (newValue == false)
		{
			AvPConfig.find(heading)->second.find(variable)->second = "false";
		}
	}
}

// create a new config file if one doesn't exist, with defaults
static bool Config_CreateDefault()
{
	std::string filePath(GetSaveFolderPath());
	filePath += FILENAME;

	std::ofstream file(filePath.c_str());
	if (!file.is_open())
	{
		LogErrorString("Couldn't create default config file!");
		return false;
	}
/*
	file << "[VideoMode]\n";
	file << "Width = 640\n";
	file << "Height = 480\n";
	file << "ColourDepth = 32\n";
	file << "UseTripleBuffering = false\n";
	file << "SafeZoneOffset = 0\n";
	file << "\n";
	file << "[Misc]\n";
	file << "CommandLine = \"\"\n";
	file << "\n";
	file << "[Networking]\n";
	file << "PortNumber = 1234\n";
*/
	file.close();

	return true;
}