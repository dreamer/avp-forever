#ifndef _console_h_
#define _console_h_

#include <string>
#ifdef WIN32
#include <windows.h>
#endif
#ifdef _XBOX
#include <xtl.h>
#endif

typedef void (*funcPointer) (void);
std::string& Con_GetArgument(int argNum);
size_t Con_GetNumArguments();
void Con_AddCommand(char *command, funcPointer function);
void Con_PrintError(const std::string &errorString);
void Con_PrintMessage(const std::string &messageString);
void Con_PrintDebugMessage(const std::string &messageString);

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

#endif
