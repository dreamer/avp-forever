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
#include "ConfigFile.h"

static bool Config_CreateDefault();

typedef std::map<std::string, std::string> MapValue;
typedef std::map<std::string, MapValue> MapHeading;

static const std::string kCfgFileName = "AliensVsPredator.cfg";

MapHeading AvPConfig;

bool Config_Load()
{
	std::string filePath(GetSaveFolderPath());
	filePath += kCfgFileName;

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
		if (tempLine.length() == 0) { // skip empty lines
			continue;
		}

		if ((tempLine.at(0) == '[') || (tempLine.at(tempLine.length() - 1) == ']')) // found a header block
		{
			//std::cout << "Found header: " << tempLine << "\n";
			currentHeading = tempLine;
		}
		else
		{
			// if there's no equals sign in the string, don't add it
			size_t equalsPosition = tempLine.find("=");
			if (equalsPosition == std::string::npos) {
				continue;
			}

			// get the variable part and remove any spaces
			std::string variable = tempLine.substr(0, equalsPosition);
			variable.erase(std::remove(variable.begin(), variable.end(),' '), variable.end());

			// get the value part, skipping the = character and a single space after that if present. we dont want to remove any spaces after that (as they may be part of a file path)
			std::string value = tempLine.substr(equalsPosition + 1, std::string::npos);
			if (value.at(0) == ' ') {
				value.erase(value.begin(), value.begin() + 1);
			}

//			std::cout << "got variable name: " << tempLine.substr(0, lenOfVar) << "\n";
//			std::cout << "its value is: " << tempLine.substr(lenOfVar + 1) << "\n";

			// should only create a new key in AvPConfig if one doesn't already exists
			MapValue &tempValue = AvPConfig[currentHeading];

			tempValue.insert(std::make_pair(variable, value));
		}
	}

	LogString("Loaded config file");

	return true;
}

bool Config_Save()
{
	std::string filePath = GetSaveFolderPath() + kCfgFileName;
	std::ofstream file(filePath.c_str());
	if (!file.is_open())
	{
		std::cout << "Error opening config file " << kCfgFileName << " for save!" << std::endl;
		return false;
	}

	MapHeading::iterator headingIt = AvPConfig.begin();

	while (headingIt != AvPConfig.end())
	{
		// writes the heading, eg [Video]
		file << (*headingIt).first << "\n";

		MapValue::iterator variableIt = (*headingIt).second.begin();

		while (variableIt != (*headingIt).second.end())
		{
			file << (*variableIt).first << " = " << (*variableIt).second << "\n";
			++variableIt;
		}

		++headingIt;

		// puts a newline between each new heading section
		if (headingIt != AvPConfig.end()) {
			file << "\n";
		}
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
		return Util::StringToInt(AvPConfig.find(heading)->second.find(variable)->second);
	}
	else
	{
		// should we be adding this to the map if it doesn't exist using default value? i guess so..
		MapValue &tempValue = AvPConfig[heading];
		tempValue.insert(std::make_pair(variable, Util::IntToString(defaultValue)));

		return defaultValue;
	}
}

void Config_SetInt(const std::string &heading, const std::string &variable, int newValue)
{
	if (CheckValuesExist(heading, variable))
	{
		AvPConfig.find(heading)->second.find(variable)->second = Util::IntToString(newValue);
	}
}

void Config_SetString(const std::string &heading, const std::string &variable, const std::string &newValue)
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
		else
		{
			// invalid value
			LogErrorString("Invalid bool value found in configuration file for " + heading + " - \'" + variable + "\'. setting to default value");
			Config_SetBool(heading, variable, defaultValue);
			return defaultValue;
		}
	}
	else
	{
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

// create a new config file if one doesn't exist. defaults will be added when values are requested
static bool Config_CreateDefault()
{
	std::string filePath(GetSaveFolderPath() + kCfgFileName);

	std::ofstream file(filePath.c_str());
	if (!file.is_open())
	{
		LogErrorString("Couldn't create default config file!");
		return false;
	}

	file.close();

	return true;
}
