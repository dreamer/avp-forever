
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>

#ifdef WIN32
	#include <shlobj.h>
	#include <shlwapi.h>
#endif

#include "logString.h"

extern int StringToInt(const std::string &string);
extern std::string IntToString(const int value);

static bool Config_CreateDefault();

typedef std::map<std::string, std::string> MapValue;
typedef std::map<std::string, MapValue> MapHeading;

#ifdef _XBOX
	#define FILENAME "d:\\AliensVsPredator.cfg"
#else
	#define FILENAME "AliensVsPredator.cfg"
#endif

MapHeading AvPConfig;

bool Config_Load()
{
#ifdef WIN32
	TCHAR strPath[MAX_PATH];

	/* finds the path to the folder. On Win7, this would be "C:\Users\<username>\AppData\Local\ as an example */
	if( FAILED(SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath ) ) )
	{
		return 0;
	}

	PathAppend( strPath, TEXT( "Fox\\Aliens versus Predator\\" ) );
	strcat( strPath, FILENAME);

	std::ifstream file(strPath);

#else
	std::ifstream file(FILENAME);
#endif

	std::string tempLine;
	std::string currentHeading;

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
#ifdef WIN32
			file.open(strPath);
#else
			file.open(FILENAME);
#endif
			if (!file.is_open())
			{
				LogErrorString("Error opening config file!");
				return false;
			}
			file.seekg(0, std::ios::beg);
		}
	}

	/* go through the cfg file line by line */
	while (getline(file, tempLine))
	{	
		if (tempLine.length() == 0)
			continue;

		if ((tempLine.at(0) == '[') || (tempLine.at(tempLine.length() - 1) == ']'))
		{
			//std::cout << "Found header: " << tempLine << "\n";
			currentHeading = tempLine;
		}
		else
		{
			/* assume we got a variable and value */
			int lenOfVar = tempLine.find("=");

			/* special case for strings such as command line */
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
				tempString2.erase(std::remove(tempString2.begin(), tempString2.end(),'"'), tempString2.end());

				// recreate original line string
				tempLine = tempString + tempString2;
			}
			else
			{
				/* remove whitespace */
				tempLine.erase(std::remove(tempLine.begin(), tempLine.end(),' '), tempLine.end());
			}

			/* if there's no equals sign in the string, don't add it */
			if (lenOfVar == std::string::npos)
				continue;

//			std::cout << "got variable name: " << tempLine.substr(0, lenOfVar) << "\n";
//			std::cout << "its value is: " << tempLine.substr(lenOfVar + 1) << "\n";

			/* should only create a new key in AvPConfig if one doesn't already exists */
			MapValue &tempValue = AvPConfig[currentHeading];

			/* +1 to skip over the equals sign */
			tempValue.insert(std::make_pair(tempLine.substr(0, lenOfVar),
												tempLine.substr(lenOfVar + 1)));
		}
	}

	LogString("Loaded config file");
	
	return true;
}

bool Config_Save()
{
#ifdef WIN32
	TCHAR strPath[MAX_PATH];

	/* finds the path to the folder. On Win7, this would be "C:\Users\<username>\AppData\Local\ as an example */
	if( FAILED(SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath ) ) )
	{
		return 0;
	}

	PathAppend( strPath, TEXT( "Fox\\Aliens versus Predator\\" ) );
	strcat( strPath, FILENAME);

	std::ofstream file(strPath);

#else
	std::ofstream file(FILENAME);
#endif

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

std::string Config_GetString(const std::string &heading, const std::string &variable)
{
	if (CheckValuesExist(heading, variable))
	{
		return (AvPConfig.find(heading)->second.find(variable)->second);
	}
	else
	{
		return "";
	}
}

/* create a new config file if one doesn't exist, with defaults */
static bool Config_CreateDefault()
{
#ifdef WIN32
	TCHAR strPath[MAX_PATH];

	/* finds the path to the folder. On Win7, this would be "C:\Users\<username>\AppData\Local\ as an example */
	if ( FAILED(SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath ) ) )
	{
		return false;
	}

	PathAppend( strPath, TEXT( "Fox\\Aliens versus Predator\\" ) );
	strcat( strPath, FILENAME);

	std::ofstream file(strPath);

#else
	std::ofstream file(FILENAME);
#endif

	if (!file.is_open())
	{
		LogErrorString("Couldn't create default config file!");
		return false;
	}

	file << "[VideoMode]\n";
	file << "Width = 640\n";
	file << "Height = 480\n";
	file << "ColourDepth = 32\n";
	file << "SafeZoneOffset = 0\n";
	file << "\n";
	file << "[Misc]\n";
	file << "CommandLine = \"\"\n";

	file.close();

	return true;
}