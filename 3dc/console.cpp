/*
	Console system. Heavily influenced by Quake3's code :)
*/

#include <string>
#include <vector>
#include <sstream>

#include "console.h"
#include "d3_func.h"
#include "iofocus.h"
#include "logString.h"
#include "font2.h"

#ifdef WIN32
#include <d3dx9math.h>
#endif
#ifdef _XBOX
#include <xtl.h>
#endif

//Custom vertex format
static const DWORD D3DFVF_CUSTOMVERTEX = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1;

struct CUSTOMVERTEX 
{
	float x, y, z;  // Position in 3d space 
	DWORD colour;   // Colour  
	float u, v;     // Texture coordinates 
};

static CUSTOMVERTEX conVerts[4];

extern "C" 
{
	#include "avp_menugfx.hpp"
	#include "platform.h"
	extern unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
	extern int RealFrameTime;
	extern D3DINFO d3d;
}

#define CHAR_WIDTH	16
#define CHAR_HEIGHT	16

#define ONE_FIXED	65536

char buf[100];

struct Command 
{
	char *cmdName;
	char *cmdDescription;
	funcPointer cmdFuncPointer;
};

std::vector<Command> cmdList;
std::vector<Command>::iterator cmdIt;
std::vector<std::string>cmdArgs;

//const int CON_TEXTSIZE = 32768;
//char consoleTextArray[CON_TEXTSIZE];

struct Console 
{
	bool isActive;
	bool isOpen;

	int xPos;
	int yPos;
	int lines;
	int lineWidth;

	int indent;

	int width;
	int height;

	int destinationY;

	std::vector<std::string> text;
	std::string	inputLine;
};

Console console;

std::string& Con_GetArgument(int argNum)
{
	return cmdArgs.at(argNum);
}

int Con_GetNumArguments()
{
	return cmdArgs.size();
}

void Con_PrintError(const std::string &errorString)
{
	console.text.push_back(errorString);

	// write to log file for now
	LogErrorString(errorString);
}

void Con_PrintHRESULTError(HRESULT hr, const std::string &errorString, int lineNumber = 0, const char *fileName = 0)
{
#if 0
	if (lineNumber && fileName)
	{
		std::stringstream sstream = "\t Error:" << errorString << DXGetErrorString(hr) << " - " << DXGetErrorDescription(hr) << "Line: " << lineNumber << "File: " << fileName;
	}
/*
	if (lineNumber && fileName)
	{
		console.text.push_back("\t Error:" + errorString + DXGetErrorString(hr) + " - " + DXGetErrorDescription(hr) + "Line: " + lineNumber + "File: " + fileName);
		LogString("\t Error:" + errorString + DXGetErrorString(hr) + " - " + DXGetErrorDescription(hr) + "Line: " + lineNumber + "File: " + fileName);
	}
	else
	{
		console.text.push_back("\t Error:" + errorString + DXGetErrorString(hr) + " - " + DXGetErrorDescription(hr));
		LogString("\t Error:" + errorString + DXGetErrorString(hr) + " - " + DXGetErrorDescription(hr));
	}
*/
#endif
}

void Con_PrintMessage(const std::string &messageString)
{
	console.text.push_back(messageString);

	// write to log file for now
	LogString(messageString);
}

void Con_PrintDebugMessage(const std::string &messageString)
{
	// for now
	Con_PrintMessage(messageString);
}

void Con_AddLine(const std::string &temp)
{
	console.text.push_back(temp);
}

void Con_AddCommand(char *command, funcPointer function)
{
	for (cmdIt = cmdList.begin(); cmdIt < cmdList.end(); ++cmdIt)
	{
		if (!strcmp(command, cmdIt->cmdName))
		{
			OutputDebugString("command already exists\n");
			return;
		}
	}
	
	Command newCommand;
	
	newCommand.cmdName = command;
	newCommand.cmdDescription = "no description yet";
	newCommand.cmdFuncPointer = function;

	cmdList.push_back(newCommand);
}

void Con_Init()
{
	console.isActive = false;
	console.xPos = 0;
	console.yPos = 0;
	console.width = 640;

	console.lines = 240 / CHAR_HEIGHT;
	console.lineWidth = 640 / CHAR_WIDTH;

	console.height = console.lines * CHAR_HEIGHT;
	console.indent = CHAR_WIDTH / 2;

	console.destinationY = 0;

	sprintf(buf, "console height: %d num lines: %d\n", console.lines * CHAR_HEIGHT, console.lines);
	OutputDebugString(buf);

	Con_AddCommand("toggleconsole", Con_Toggle);
//	LoadConsoleFont();
/*
	for (int i = 0; i < CON_TEXTSIZE; i++)
	{
		consoleTextArray[i] = ' ';
	}
*/
	OutputDebugString("console initialised\n");
}

void Con_ProcessCommand()
{
	// if nothing typed, don't bother doing anything
	if (console.inputLine.length() == 0) 
		return;

	// clear the arg vector
	cmdArgs.clear();

	// find the command name first
	std::string commandName;
	Command theCommand = {0};
	std::stringstream stream(console.inputLine);

	// the command will be first word up to first space
	getline(stream, commandName, ' ');

	// see if the command actually exists first.
	for (cmdIt = cmdList.begin(); cmdIt < cmdList.end(); ++cmdIt)
	{
		if (commandName == cmdIt->cmdName)
		{
			// found it
			theCommand = *cmdIt;
			break;
		}
	}
	
	// in case we didnt find it
	if (theCommand.cmdFuncPointer == NULL)
	{
		Con_AddLine(commandName);
		Con_AddLine("Unknown command \"" + commandName + "\"");
		console.inputLine.clear();
		return;
	}

	// parse the rest of the string for arguments
	std::string currentArg;

	while (getline(stream, currentArg, ' '))
	{
		// add the current parameter to the arguments list
		cmdArgs.push_back(currentArg);
	}

	Con_AddLine(console.inputLine);

	console.inputLine.clear();

	// lets call the function now..
	theCommand.cmdFuncPointer();
}

void Con_ProcessInput()
{
	if (DebouncedKeyboardInput[KEY_BACKSPACE])
		Con_RemoveTypedChar();

	if (DebouncedKeyboardInput[KEY_CR])
		Con_ProcessCommand();
}

void Con_Toggle()
{
	console.isActive = !console.isActive;

	if (console.height > 0) 
		console.destinationY = 0;
	else 
		console.destinationY = console.lines * CHAR_HEIGHT;

	// toggle on/off
	IOFOCUS_Set(IOFOCUS_Get() ^ IOFOCUS_NEWCONSOLE);
}

bool Con_IsActive()
{
	return console.isActive;
}

bool Con_IsOpen()
{
	return console.isOpen;
}

void Con_CheckResize()
{
/*
	if (console.width != ScreenDescriptorBlock.SDB_Width)
	{
		console.width = ScreenDescriptorBlock.SDB_Width;
	}
*/
}

void Con_AddTypedChar(char c)
{
	if (!console.isActive) 
		return;

	if (c == 0x08) // backspace
	{
		Con_RemoveTypedChar();
	}
	else if (c == 13) // enter/return key
	{	
		Con_ProcessCommand();
	}
	else
	{
		if (c < 32)
			return;
	//	sprintf(buf, "Con_AddTypedChar: %c\n", c);
	//	OutputDebugString(buf);

		console.inputLine.push_back(c);
	}
}

void Con_RemoveTypedChar()
{
	if (console.inputLine.length())
	{
		console.inputLine.erase(console.inputLine.length() -1, 1);
	}
}

void Con_Draw()
{
	int charWidth = 0;

	if (!console.isActive)
		return;

	// is console moving to a new position?
	if (console.destinationY > console.height)
	{
		console.height += static_cast<int>(RealFrameTime * 0.01f);
		if (console.height > console.destinationY)
			console.height = console.destinationY;

		console.isOpen = false;
	}
	else if (console.destinationY < console.height)
	{
		console.height -= static_cast<int>(RealFrameTime * 0.01f);
		if (console.height < console.destinationY)
			console.height = console.destinationY;

		console.isOpen = false;
	}

	if (console.destinationY == console.height)
		console.isOpen = true;

	// draw the background quad
	DrawQuad(console.xPos, console.yPos, console.width, console.height, -1, D3DCOLOR_ARGB(255, 38, 80, 145));

	if (console.height > 0)
	{
		// draw the outline bar that runs along the bottom of the console
		DrawQuad(console.xPos, console.yPos + console.height, console.width, 2, -1, D3DCOLOR_ARGB(255, 255, 255, 255));
	}

	int charCount = 0;
	static int alpha = ONE_FIXED;

	alpha -= static_cast<int>(RealFrameTime * 1.2f);
	if (alpha < 0) 
		alpha = ONE_FIXED;

	// draw input cusor
	charWidth = RenderSmallChar('>', console.indent, console.height - CHAR_HEIGHT, ONE_FIXED, ONE_FIXED, ONE_FIXED, ONE_FIXED);
	RenderSmallChar('_', console.indent + charWidth, console.height - CHAR_HEIGHT, alpha, ONE_FIXED, ONE_FIXED, ONE_FIXED);

	int rows = console.text.size() - 1;

	int y = console.height - CHAR_HEIGHT * 2;

	int lines = console.lines;
	int xOffset = 0;

	// draw all the lines of text
	for (int i = rows; i >= 0; i--, y -= CHAR_HEIGHT)
	{
		xOffset = 0;
		charWidth = 0;

//		Font_DrawText(console.text[i].c_str(), console.indent + xOffset, y, D3DCOLOR_ARGB(255, 255, 255, 255), FONT_SMALL);

		for (int j = 0; j < console.text[i].length(); j++)
		{
			charWidth = RenderSmallChar(console.text.at(i).at(j), console.indent + xOffset, y, ONE_FIXED, ONE_FIXED / 2, ONE_FIXED, ONE_FIXED);
			//Font_DrawText(console.text.at(i).at(j), console.indent + xOffset, y, D3DCOLOR_ARGB(255, 255, 255, 255), FONT_SMALL);
			xOffset += charWidth;
		}
	}

	xOffset = CHAR_WIDTH;
	charWidth = 0;

	// draw the line of text we're currently typing
	for (int j = 0; j < console.inputLine.length(); j++)
	{
		charWidth = RenderSmallChar(console.inputLine.at(j), console.indent + xOffset, console.height - CHAR_HEIGHT, ONE_FIXED, ONE_FIXED, ONE_FIXED, ONE_FIXED);
		xOffset += charWidth;
	}
}