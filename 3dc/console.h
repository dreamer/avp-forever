#ifndef _console_h_
#define _console_h_

#include <string>

void Con_PrintError(const std::string &errorString);
void Con_PrintMessage(const std::string &messageString);

#ifdef __cplusplus
extern "C" {
#endif

void Con_Init();
void Con_Draw();
void Con_Toggle();
void Con_AddTypedChar(char c);
bool Con_IsActive();
bool Con_IsOpen();
void Con_Key_Enter(bool state);
void Con_Key_Backspace(bool state);
void Con_RemoveTypedChar();
void Con_ProcessInput();

#ifdef __cplusplus
}
#endif

#endif