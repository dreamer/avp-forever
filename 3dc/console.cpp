/*
	Console system. Heavily influenced by Quake3's code :)
*/

#if 0
#include <string>
#include <vector>
#include <sstream>
#include "console.h"
#include "renderer.h"
#include "iofocus.h"
#include "logString.h"
#include "Fonts.h"
#include <stdint.h>
#include "Input.h"
#include "io.h"

const int kCharWidth  = 12;
const int kCharHeight = 16;
const int kOutlineBarHeight = 2;
const char kBackspaceASCIIcode = 0x08;
const char kReturnASCIIcode = 0x0D;

const int kTextSize = 32768;

const colourIndex_t kColourWhite = 1;
const colourIndex_t kColourRed   = 2;
const colourIndex_t kColourGreen = 3;
const colourIndex_t kColourBlue  = 4;
const colourIndex_t kColourYellow = 5;

struct ConsoleCommand
{
	char *name;
	char *description;
	funcPointer functionPointer;
};

std::vector<ConsoleCommand> cmdList;
std::vector<ConsoleCommand>::iterator cmdIt;
std::vector<std::string>cmdArgs;

struct Console
{
	int xPos;
	int yPos;
	int lines;


	int indent;

	int width;
	int height;

	float currentY;
	float destinationY;

	std::vector<int16_t> text;
	uint32_t currentLine; // line where next message will be printed
	uint32_t x;           // offset in current line for next print
	uint32_t display;     // bottom of console displays this line
	uint32_t nLines;
	uint32_t lineWidth;   // number of characters per line
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
//	console.text.push_back(errorString);

	// write to log file for now
	LogErrorString(errorString);
}

void Con_PrintMessage(const std::string &messageString)
{
//	console.text.push_back(messageString);

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
//	console.text.push_back(temp);
}

void Con_DrawChar(const char theChar, colourIndex_t colourIndex)
{
	RCOLOR theColour;

	switch (colourIndex)
	{
		case kColourWhite:
			theColour = RCOLOR_ARGB(255, 255, 255, 255);
			break;
		case kColourRed:
			theColour = RCOLOR_ARGB(255, 255, 0, 0);
			break;
		case kColourGreen:
			theColour = RCOLOR_ARGB(255, 0, 255, 0);
			break;
		case kColourBlue:
			theColour = RCOLOR_ARGB(255, 0, 0, 255);
			break;
		case kColourYellow:
			theColour = RCOLOR_ARGB(255, 255, 255, 0);
			break;
		default:
			// just set white
			theColour = RCOLOR_ARGB(255, 255, 255, 255);
			break;
	}
}

void Con_AddCommand(char *commandName, funcPointer function)
{
	for (cmdIt = cmdList.begin(); cmdIt < cmdList.end(); ++cmdIt)
	{
		if (!strcmp(commandName, cmdIt->name))
		{
			LogErrorString("console command already exists");
			return;
		}
	}

	ConsoleCommand newCommand;

	newCommand.name = commandName;
	newCommand.description = "no description yet";
	newCommand.functionPointer = function;

	cmdList.push_back(newCommand);
}

void Con_Init()
{
	Font_Load("console_font", kFontConsole);

	console.xPos = 0;
	console.yPos = 0;
	console.width = 640;

	console.lines = 240 / kCharHeight;
	console.lineWidth = 640 / kCharWidth;

	console.height = console.lines * kCharHeight;
	console.indent = kCharWidth / 2;

	console.currentY = 0;
	console.destinationY = 0;

//	console.xOffset = 0;
	console.currentLine = 0;
	console.nLines = 0;

	console.text.resize(kTextSize);

	// set all text to blank white spaces
	for (int i = 0; i < kTextSize; i++)
	{
		// store colour index (char sized) plus char
		console.text[i] = (kColourWhite << 8) | ' ';
	}

	Con_AddCommand("toggleconsole", Con_Toggle);
}

void Con_ProcessCommand()
{
#if 0
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

	// see if the command already exists first.
	for (cmdIt = cmdList.begin(); cmdIt < cmdList.end(); ++cmdIt)
	{
		if (commandName == cmdIt->name)
		{
			// found it
			theCommand = *cmdIt;
			break;
		}
	}

	// in case we didn't find it
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
#endif
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

void Con_CheckResize()
{
}

void Con_AddTypedChar(char c)
{
//	if (!console.isActive)
//		return;

	switch (c)
	{
		case kBackspaceASCIIcode:
		{
			Con_RemoveTypedChar();
			return;
		}
		case kReturnASCIIcode:
		{
			Con_ProcessCommand();
			return;
		}
	}

	if (c < 32)
	{
		return;
	}

//	console.inputLine.push_back(c);
}

void Con_RemoveTypedChar()
{
/*
	if (console.inputLine.length())
	{
		console.inputLine.erase(console.inputLine.length() -1, 1);
	}
*/
}

void Con_Draw()
{
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

	// draw the outline bar that runs along the bottom of the console (if it's open)
	if (console.height > 0)
	{
		DrawQuad(console.xPos, console.yPos + console.height, console.width, kOutlineBarHeight, NO_TEXTURE, RCOLOR_ARGB(255, 255, 255, 255), TRANSLUCENCY_OFF);
	}

	uint32_t y = console.height - kCharHeight;

	// draw input cusor
	Font_DrawText(">", console.indent, y, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);
	Font_DrawText("_", console.indent + kCharWidth, y, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);

	y -= kCharHeight;

	// test text
	Font_DrawText("here is some test text 123 r_setfov 90", console.indent, y, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);

#if 0
	int32_t rows = console.text.size() - 1;

	int xOffset = 0;

	// draw all the lines of text
	for (; rows >= 0; rows--, y -= kCharHeight)
	{
		xOffset = 0;

		Font_DrawText(console.text.at(rows), console.indent + xOffset, y, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);
	}

	xOffset = kCharWidth;

	// draw the line of text we're currently typing
	Font_DrawText(console.inputLine, console.indent + xOffset, console.height - kCharHeight, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);
#endif
}







#else

#include <string>
#include <vector>
#include <sstream>
#include "console.h"
#include "renderer.h"
#include "iofocus.h"
#include "logString.h"
#include "Fonts.h"
#include <stdint.h>
#include "Input.h"
#include "io.h"

const int kCharWidth  = 12;
const int kCharHeight = 16;
const int kOutlineBarHeight = 2;
const char kBackspaceASCIIcode = 0x08;
const char kReturnASCIIcode = 0x0D;

const int kTextSize = 32768;
std::vector<int16_t> text2;

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

// can be used to check if any arguments were specified - can print help message if not
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
			LogErrorString("console command already exists");
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
	return;
	Font_Load("console_font", kFontConsole);

	console.xPos = 0;
	console.yPos = 0;
	console.width = 640;

	console.lines = 240 / kCharHeight;
	console.lineWidth = 640 / kCharWidth;

	console.height = console.lines * kCharHeight;
	console.indent = kCharWidth / 2;

	console.currentY = 0;
	console.destinationY = 0;

//	sprintf(buf, "console height: %d num lines: %d\n", console.lines * CHAR_HEIGHT, console.lines);
//	OutputDebugString(buf);

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

	// see if the command already exists first.
	for (cmdIt = cmdList.begin(); cmdIt < cmdList.end(); ++cmdIt)
	{
		if (commandName == cmdIt->cmdName)
		{
			// found it
			theCommand = *cmdIt;
			break;
		}
	}

	// in case we didn't find it
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

void Con_CheckResize()
{
}

void Con_AddTypedChar(char c)
{
//	if (!console.isActive)
//		return;

	// handle any buttons we want to process, such as backspace
	switch (c)
	{
		case kBackspaceASCIIcode:
		{
			Con_RemoveTypedChar();
			return;
		}
		case kReturnASCIIcode:
		{
			Con_ProcessCommand();
			return;
		}
	}

	// now process text characters
	if (c < 32)
	{
		return;
	}

	console.inputLine.push_back(c);
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

	// draw the outline bar that runs along the bottom of the console (if it's open)
	if (console.height > 0)
	{
		DrawQuad(console.xPos, console.yPos + console.height, console.width, kOutlineBarHeight, NO_TEXTURE, RCOLOR_ARGB(255, 255, 255, 255), TRANSLUCENCY_OFF);
	}

	uint32_t y = console.height - kCharHeight;

	// draw input cusor
	Font_DrawText(">", console.indent, y, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);
//	Font_DrawText("_", console.indent + kCharWidth, y, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);

	y -= kCharHeight;

	int32_t rows = console.text.size() - 1;

	int xOffset = 0;

	// draw all the lines of text
	for (; rows >= 0; rows--, y -= kCharHeight)
	{
		xOffset = 0;

		Font_DrawText(console.text.at(rows), console.indent + xOffset, y, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);
	}

	xOffset = kCharWidth;

	// draw the line of text we're currently typing
	Font_DrawText(console.inputLine, console.indent + xOffset, console.height - kCharHeight, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);

//	Font_DrawText("_", (console.indent + xOffset) /* + Font_GetStringWidth(console.inputLine)*/, y, RCOLOR_ARGB(255, 255, 255, 255), kFontConsole);
}

#endif