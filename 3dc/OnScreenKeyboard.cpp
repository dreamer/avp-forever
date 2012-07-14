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

#include "OnScreenKeyboard.h"
#include "logString.h"
#include "Fonts.h"
#include "renderer.h"
#include "stdio.h"
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <assert.h>

/* TODO:

	- align text properly
	- add Shift
	- add Caps lock
	- Symbols (can I be bothered?...)
	- xbox specific graphics to indicate press B to go back, A to select..
*/

#include "platform.h"

static int Osk_GetCurrentLocation();
std::string Osk_GetKeyLabel(uint32_t buttonIndex);

#ifdef _XBOX
extern void AddKeyToQueue(char virtualKeyCode);
#endif

static int currentRow = 0;
static int currentColumn = 0;

static int currentValue = 0;

static uint16_t osk_x = 320;
static uint16_t osk_y = 280;
static uint16_t oskWidth = 400;
static uint16_t oskHeight = 200;

static const uint8_t keyWidth = 30;
static const uint8_t keyHeight = 30;

static const uint8_t space_between_keys = 3;
static const uint8_t outline_border_size = 1;
static const uint8_t indent_space = 5;

static const uint8_t numVerticalKeys = 5;
static const uint8_t numHorizontalKeys = 12;
static const uint8_t numKeys = numVerticalKeys * numHorizontalKeys;

struct ButtonStruct
{
	uint32_t	numWidthBlocks;
	uint32_t	width;
	uint32_t	height;
	uint32_t	positionOffset;
	uint32_t	stringID;
	bool		isBlank;
};
std::vector<ButtonStruct> keyVector;

// we store our strings seperately and index using the stringID (to avoid duplicates)
std::vector<std::string> stringVector;

static bool isActive = false;
static bool isInitialised = false;

bool shift = false;
bool capsLock = false;

uint32_t buttonID = 0;

template <class T> void Osk_AddKey(T buttonLabel, uint32_t numWidthBlocks)
{
	ButtonStruct newButton = {0};

	std::stringstream stringStream;

	stringStream << buttonLabel;

	newButton.stringID = buttonID;
	newButton.numWidthBlocks = numWidthBlocks;
	newButton.width = (numWidthBlocks * keyWidth) + space_between_keys * (numWidthBlocks - 1);
	newButton.height = keyHeight;

	// store the string in its own vector
	stringVector.push_back(stringStream.str());

	if (stringStream.str() == "BLANK")
	{
		newButton.isBlank = true;
	}

	uint32_t positionOffset = 0;

	uint32_t blockCount = numWidthBlocks;

	// for each block in a button, add it to the key vector
	while (blockCount)
	{
		newButton.positionOffset = positionOffset;
		keyVector.push_back(newButton);

		positionOffset++;
		blockCount--;
	}

	buttonID++;
}

void Osk_Init()
{
	// disable for now
	return;


	currentRow = 0;
	currentColumn = 0;

	// do top row of numbers
	for (int32_t i = 9; i >= 0; i--)
	{
		Osk_AddKey(i, 1);
	}

	Osk_AddKey("Done", 2);

	// second row..
	for (char letter = 'a'; letter <= 'j'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Shift", 2);

	// third row..
	for (char letter = 'k'; letter <= 't'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Caps Lock", 2);

	// fourth row..
	for (char letter = 'u'; letter <= 'z'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Backspace", 4);
	Osk_AddKey("Symbols",	2);

	// fifth row
	Osk_AddKey("Space", 6);
	Osk_AddKey("<",		2);
	Osk_AddKey(">",		2);
	Osk_AddKey("BLANK", 2);

//	sprintf(buf, "number of keys added to osk: %d\n", keyVector.size());
//	OutputDebugString(buf);

	assert (keyVector.size() == 60);

	currentValue = 0;

	oskWidth = (keyWidth * numHorizontalKeys) + (space_between_keys * numHorizontalKeys) + (indent_space * 2);
	oskHeight = (keyHeight * numVerticalKeys) + (space_between_keys * numVerticalKeys) + (indent_space * 2);

	isInitialised = true;
}

void Osk_Draw()
{
	// disable for now
	return;


	if (!Osk_IsActive())
		return;

	osk_x = (640 - oskWidth) / 2;

//	sprintf(buf, "currentRow: %d, currentColumn: %d, currentItem: %d curentChar: %s\n", currentRow, currentColumn, Osk_GetCurrentLocation(), keyVector.at(Osk_GetCurrentLocation()).name.c_str());
//	OutputDebugString(buf);

	// draw background rectangle
	DrawQuad(osk_x, osk_y, oskWidth, oskHeight, NO_TEXTURE, RCOLOR_ARGB(120, 80, 160, 120), TRANSLUCENCY_GLOWING);

	// start off with an indent to space things out nicely
	int pos_x = osk_x + indent_space;
	int pos_y = osk_y + indent_space;

	size_t index = 0;

	for (uint32_t y = 0; y < numVerticalKeys; y++)
	{
		pos_x = osk_x + indent_space;

		int widthCount = numHorizontalKeys;

		while (widthCount)
		{
			ButtonStruct &thisButton = keyVector.at(index);

			// only draw keys not marked as blank keys
			if (!thisButton.isBlank)
			{
				DrawQuad(pos_x, pos_y, thisButton.width, thisButton.height, NO_TEXTURE, RCOLOR_ARGB(200, 255, 255, 255), TRANSLUCENCY_NORMAL);

				if (Osk_GetCurrentLocation() == index) // draw the selected item differently (highlight it)
				{
					DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, thisButton.width - outline_border_size * 2, thisButton.height - outline_border_size * 2, NO_TEXTURE, RCOLOR_ARGB(220, 0, 128, 0), TRANSLUCENCY_OFF);
				}
				else
				{
					DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, thisButton.width - outline_border_size * 2, thisButton.height - outline_border_size * 2, NO_TEXTURE, RCOLOR_ARGB(220, 38, 80, 145), TRANSLUCENCY_OFF);
				}

//				int positionX = pos_x + ((thisButton.width - 16) / 2);

				Font_DrawText(Osk_GetKeyLabel(index), pos_x + (thisButton.width / 2), pos_y + space_between_keys, RCOLOR_ARGB(255, 255, 255, 0), kFontSmall);
			}

			pos_x += (thisButton.width + space_between_keys);
			widthCount -= thisButton.numWidthBlocks;

			index += thisButton.numWidthBlocks;
		}
		pos_y += (keyHeight + space_between_keys);
	}
}

bool Osk_IsActive()
{
	return isActive; // sort this later to only appear for text entry on xbox
}

std::string Osk_GetKeyLabel(uint32_t buttonIndex)
{
	// quick test for shift and caps lock..
	if ((shift || capsLock) && ((stringVector.at(keyVector.at(buttonIndex).stringID).length() == 1)))
	{
		std::string tempString = stringVector.at(keyVector.at(buttonIndex).stringID);

		std::transform(tempString.begin(), tempString.end(), tempString.begin(), toupper);

		return tempString;
	}

	return stringVector.at(keyVector.at(buttonIndex).stringID);
}

void Osk_Activate()
{
	// disable for now
	return;


	if (isInitialised == false)
		Osk_Init();

	isActive = true;
}

void Osk_Deactivate()
{
	// disable for now
	return;


	isActive = false;
}

KEYPRESS Osk_HandleKeypress()
{
	std::string buttonLabel = Osk_GetKeyLabel(Osk_GetCurrentLocation());

	KEYPRESS newKeypress = {0};

	if (buttonLabel == "Done")
	{
		newKeypress.keyCode = KEY_CR;
		return newKeypress;
	}

	else if (buttonLabel == "Shift")
	{
		shift = !shift;

		// if capslock on, turn it off?
		if (capsLock)
			capsLock = false;

		return newKeypress;
	}

	else if (buttonLabel == "Caps Lock")
	{
		capsLock = !capsLock;

		// if shift is on, turn it off?
		if (shift)
			shift = false;

		return newKeypress;
	}

	else if (buttonLabel == "Symbols")
	{
		return newKeypress;
	}

	else if (buttonLabel == "Space")
	{
		newKeypress.asciiCode = ' ';
		return newKeypress;
	}

	else if (buttonLabel == "Backspace")
	{
		newKeypress.keyCode = KEY_BACKSPACE;
		return newKeypress;
	}

	else if (buttonLabel == "<")
	{
		return newKeypress;
	}

	else if (buttonLabel == ">")
	{
		return newKeypress;
	}

	else
	{
		newKeypress.asciiCode = buttonLabel.at(0);

		if (shift) // turn it off, as it's a use-once thing
			shift = false;

		return newKeypress;
	}

	return newKeypress;
}

static int Osk_GetCurrentLocation()
{
	return currentValue;
}

void Osk_MoveLeft()
{
	// where are we now?
	int currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// store some information about current key
	int buttonOffset = keyVector.at(currentPosition).positionOffset;

	// lets do the actual left move
	currentColumn -= buttonOffset + 1;

	// wrap?
	if (currentColumn < 0)
		currentColumn = numHorizontalKeys - 1;

	// where are we now?
	currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (keyVector.at(currentPosition).isBlank)
	{
		buttonOffset = keyVector.at(currentPosition).positionOffset;

		currentColumn -= buttonOffset + 1;

		// wrap?
		if (currentColumn < 0)
			currentColumn = numHorizontalKeys - 1;

		// where are we now?
		currentPosition = (currentRow * numHorizontalKeys) + currentColumn;
	}

	currentValue = currentPosition;

	// then align to button left..
	currentValue -= keyVector.at(currentValue).positionOffset;
}

void Osk_MoveRight()
{
	// where are we now?
	int currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	ButtonStruct &thisButton = keyVector.at(currentPosition);

	int buttonOffset = thisButton.positionOffset;
	int width		 = thisButton.numWidthBlocks;

	currentColumn += width - buttonOffset;

	// wrap?
	if (currentColumn >= numHorizontalKeys) { // add some sort of numColumns?
		currentColumn = 0;
	}

	// where are we now?
	currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (thisButton.isBlank)
	{
		buttonOffset = keyVector.at(currentPosition).positionOffset;

		currentColumn += width - buttonOffset;

		// wrap?
		if (currentColumn >= numHorizontalKeys) { // add some sort of numColumns?
			currentColumn = 0;
		}

		// where are we now?
		currentPosition = (currentRow * numHorizontalKeys) + currentColumn;
	}

	currentValue = currentPosition;

	// then align to button left..
	currentValue -= keyVector.at(currentValue).positionOffset;
}

void Osk_MoveUp()
{
	currentRow--;

	// make sure we're not outside the grid
	if (currentRow < 0)
		currentRow = 4;

	// where are we now?
	int currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (keyVector.at(currentPosition).isBlank)
	{
		currentRow--;

		// make sure we're not outside the grid
		if (currentRow < 0)
			currentRow = 4;

		// where are we now?
		currentPosition = (currentRow * numHorizontalKeys) + currentColumn;
	}

	currentValue = currentPosition;

	// left align button value
	currentValue -= keyVector.at(currentValue).positionOffset;
}

void Osk_MoveDown()
{
	currentRow++;

	// make sure we're not outside the grid
	if (currentRow > 4)
		currentRow = 0;

	// where are we now?
	int currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (keyVector.at(currentPosition).isBlank)
	{
		currentRow++;

		// make sure we're not outside the grid
		if (currentRow > 4)
			currentRow = 0;

		// where are we now?
		currentPosition = (currentRow * numHorizontalKeys) + currentColumn;
	}

	currentValue = currentPosition;

	// left align button value
	currentValue -= keyVector.at(currentValue).positionOffset;
}

