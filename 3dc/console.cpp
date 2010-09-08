/*
	Console system. Heavily influenced by Quake3's code :)
*/

#include <string>
#include <vector>
#include <sstream>

#include "console.h"
#include "renderer.h"
#include "iofocus.h"
#include "logString.h"
#include "font2.h"
#include <stdint.h>

extern "C"
{
	#include "avp_menugfx.hpp"
	#include "platform.h"
	extern unsigned char DebouncedKeyboardInput[MAX_NUMBER_OF_INPUT_KEYS];
	extern int RealFrameTime;
}

#define CHAR_WIDTH	12//16
#define CHAR_HEIGHT	16

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

struct Console
{
	int xPos;
	int yPos;
	int lines;
	int lineWidth;

	int indent;

	int width;
	int height;

	float currentY;
	float destinationY;

	std::vector<std::string> text;
	std::string	inputLine;
};

Console console;

std::string& Con_GetArgument(int argNum)
{
	return cmdArgs.at(argNum);
}

size_t Con_GetNumArguments()
{
	return cmdArgs.size();
}

void Con_PrintError(const std::string &errorString)
{
	console.text.push_back(errorString);

	// write to log file for now
	LogErrorString(errorString);
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
//	console.isActive = false;
	console.xPos = 0;
	console.yPos = 0;
	console.width = 640;

	console.lines = 240 / CHAR_HEIGHT;
	console.lineWidth = 640 / CHAR_WIDTH;

	console.height = console.lines * CHAR_HEIGHT;
	console.indent = CHAR_WIDTH / 2;

	console.currentY = 0;
	console.destinationY = 0;

	sprintf(buf, "console height: %d num lines: %d\n", console.lines * CHAR_HEIGHT, console.lines);
	OutputDebugString(buf);

	Con_AddCommand("toggleconsole", Con_Toggle);
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
	// toggle on/off
	IOFOCUS_Set(IOFOCUS_Get() ^ IOFOCUS_NEWCONSOLE);
}

/*
bool Con_IsActive()
{
	return console.isActive;
}

bool Con_IsOpen()
{
	return console.isOpen;
}
*/

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
//	if (!console.isActive)
//		return;

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
	uint32_t charWidth = 12;

	if (IOFOCUS_Get() & IOFOCUS_NEWCONSOLE)
	{
		console.destinationY = 0.5f;
	}
	else
	{
		console.destinationY = 0.0f;
	}

	// is console moving to a new position?
	if (console.destinationY > console.currentY)
	{
		console.currentY += RealFrameTime * 0.00002f;
		if (console.currentY > console.destinationY)
			console.currentY = console.destinationY;
	}
	else if (console.destinationY < console.currentY)
	{
		console.currentY -= RealFrameTime * 0.00002f;
		if (console.currentY < console.destinationY)
			console.currentY = console.destinationY;
	}

	console.height = 480 * console.currentY;

	if (console.height == 0)
		return;

	// draw the background quad
	DrawQuad(console.xPos, console.yPos, console.width, console.height, NO_TEXTURE, RCOLOR_ARGB(255, 38, 80, 145), TRANSLUCENCY_OFF);

	if (console.height > 0)
	{
		// draw the outline bar that runs along the bottom of the console
		DrawQuad(console.xPos, console.yPos + console.height, console.width, 2, NO_TEXTURE, RCOLOR_ARGB(255, 255, 255, 255), TRANSLUCENCY_OFF);
	}

//	int charCount = 0;

	uint32_t y = console.height;

	// draw input cusor
	Font_DrawText(">", console.indent, y, RCOLOR_ARGB(255, 255, 255, 255), FONT_SMALL);
	Font_DrawText("_", console.indent + charWidth, y, RCOLOR_ARGB(255, 255, 255, 255), FONT_SMALL);

	y -= CHAR_HEIGHT;

	int32_t rows = console.text.size() - 1;

//	int lines = console.lines;
	int xOffset = 0;

	// draw all the lines of text
	for (; rows >= 0; rows--, y -= CHAR_HEIGHT)
	{
		xOffset = 0;
		charWidth = 0;

		Font_DrawText(console.text.at(rows), console.indent + xOffset, y, RCOLOR_ARGB(255, 255, 255, 255), FONT_SMALL);
	}

	xOffset = CHAR_WIDTH;
	charWidth = 0;

	// draw the line of text we're currently typing
	Font_DrawText(console.inputLine, console.indent + xOffset, console.height - CHAR_HEIGHT, RCOLOR_ARGB(255, 255, 255, 255), FONT_SMALL);
}
