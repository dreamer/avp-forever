#ifndef _configFile_h_
#define _configFile_h_

#include <string>

bool Config_Load();
bool Config_Save();
int Config_GetInt(const std::string &heading, const std::string &variable, int defaultValue);
void Config_SetInt(const std::string &heading, const std::string &variable, int newValue);
void Config_SetString(const std::string &heading, const std::string &variable, const std::string newValue);
std::string Config_GetString(const std::string &heading, const std::string &variable);
bool Config_CreateDefault();

#endif