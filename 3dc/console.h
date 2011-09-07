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

#ifndef _console_h_
#define _console_h_

#include <string>
#ifdef WIN32
#include <windows.h>
#endif
#ifdef _XBOX
#include <xtl.h>
#endif

// function pointer for commands
typedef void (*funcPointer) (void);

typedef unsigned char colourIndex_t;

std::string& Con_GetArgument (int argNum);
size_t Con_GetNumArguments   ();
void Con_AddCommand          (char *commandName, funcPointer function);
void Con_PrintError          (const std::string &errorString);
void Con_PrintMessage        (const std::string &messageString);
void Con_PrintDebugMessage   (const std::string &messageString);

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
void Con_DrawChar(const char c, colourIndex_t colourIndex);

#endif
