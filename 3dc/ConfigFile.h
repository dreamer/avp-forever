#ifndef _ConfigFile_h_
#define _ConfigFile_h_

#include <string>

bool Config_Load();
bool Config_Save();
int Config_GetInt(const std::string &heading, const std::string &variable, int defaultValue);
void Config_SetInt(const std::string &heading, const std::string &variable, int newValue);
void Config_SetString(const std::string &heading, const std::string &variable, const std::string newValue);
std::string Config_GetString(const std::string &heading, const std::string &variable, const std::string &defaultString);
bool Config_GetBool(const std::string &heading, const std::string &variable, bool defaultValue);
void Config_SetBool(const std::string &heading, const std::string &variable, bool newValue);

#endif
