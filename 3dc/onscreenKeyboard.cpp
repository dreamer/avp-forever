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

#include "onscreenKeyboard.h"
#include "logString.h"
#include "d3_func.h"
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

struct TextureResource
{
	std::string fileName;
	int width;
	int height;
	D3DTEXTURE texture;
};

std::vector<TextureResource> oskTextures;

extern "C"
{
	extern void D3D_DrawRectangle(int x, int y, int w, int h, int alpha);
	extern unsigned char KeyboardInput[];
	extern char AAFontWidths[256];

	#include "avp_menugfx.hpp"
	#include "platform.h"
}

#define ONE_FIXED	65536

static int Osk_GetCurrentLocation();
std::string Osk_GetKeyLabel(int buttonIndex);

static int currentRow = 0;
static int currentColumn = 0;

static int currentValue = 0;

static int osk_x = 320;
static int osk_y = 280;
static int oskWidth = 400;
static int oskHeight = 200;

static const int keyWidth = 30;
static const int keyHeight = 30;

static const int space_between_keys = 3;
static const int outline_border_size = 1;
static const int indent_space = 5;

struct ButtonStruct
{
	int numWidthBlocks;
	int height;
	int width;
	int positionOffset;
	int stringId;
	bool isBlank;
};
std::vector<ButtonStruct> keyVector;

// we store our strings seperately and index using the stringId (to avoid duplicates)
std::vector<std::string> stringVector;

const int numVerticalKeys = 5;
const int numHorizontalKeys = 12;
const int numKeys = numVerticalKeys * numHorizontalKeys;

static bool is_active = false;
static bool is_inited = false;

bool shift = false;
bool capsLock = false;

static int buttonId = 0;

static char buf[100];

template <class T> void Osk_AddKey(T buttonLabel, int numWidthBlocks)
{
	ButtonStruct newButton = {0};

	std::stringstream stringStream;

	stringStream << buttonLabel;

	newButton.stringId = buttonId;
	newButton.numWidthBlocks = numWidthBlocks;
	newButton.width = (numWidthBlocks * keyWidth) + space_between_keys * (numWidthBlocks - 1);
	newButton.height = keyHeight;

	// store the string in its own vector
	stringVector.push_back(stringStream.str());

	if (stringStream.str() == "BLANK")
	{
		newButton.isBlank = true;
	}

	int positionOffset = 0;

	int blockCount = numWidthBlocks;

	// for each block in a button, add it to the key vector
	while (blockCount)
	{
		newButton.positionOffset = positionOffset;
		keyVector.push_back(newButton);

		positionOffset++;
		blockCount--;
	}

	buttonId++;
}

void Osk_Init()
{
	return;

	currentRow = 0;
	currentColumn = 0;
/*
	// load the button textures
	TextureResource newTexture;
	newTexture.fileName = "images//a.png";
	newTexture.texture = CheckAndLoadUserTexture(newTexture.fileName.c_str(), &newTexture.width, &newTexture.height);
	oskTextures.push_back(newTexture);

	newTexture.fileName = "images//b.png";
	newTexture.texture = CheckAndLoadUserTexture(newTexture.fileName.c_str(), &newTexture.width, &newTexture.height);
	oskTextures.push_back(newTexture);
*/
	// do top row of numbers
	for (int i = 9; i >= 0; i--)
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

	is_inited = true;
}

void Osk_Draw()
{
	if (!Osk_IsActive()) 
		return;

	osk_x = (640 - oskWidth) / 2;

//	sprintf(buf, "currentRow: %d, currentColumn: %d, currentItem: %d curentChar: %s\n", currentRow, currentColumn, Osk_GetCurrentLocation(), keyVector.at(Osk_GetCurrentLocation()).name.c_str());
//	OutputDebugString(buf);

	// draw background rectangle
	DrawQuad(osk_x, osk_y, oskWidth, oskHeight, -1, D3DCOLOR_ARGB(120, 80, 160, 120), TRANSLUCENCY_GLOWING);

	// start off with an indent to space things out nicely
	int pos_x = osk_x + indent_space;
	int pos_y = osk_y + indent_space;

	int innerSquareWidth = keyWidth - outline_border_size * 2;
	int innerSquareHeight = keyHeight - outline_border_size * 2;

	int index = 0;

	for (int y = 0; y < numVerticalKeys; y++)
	{
		pos_x = osk_x + indent_space;

		int widthCount = numHorizontalKeys;

		while (widthCount)
		{
			// only draw keys not marked as blank keys
			if (!keyVector.at(index).isBlank)
			{
				DrawQuad(pos_x, pos_y, keyVector.at(index).width, keyVector.at(index).height, -1, D3DCOLOR_ARGB(200, 255, 255, 255), TRANSLUCENCY_GLOWING);

				if (Osk_GetCurrentLocation() == index) // draw the selected item differently (highlight it)
					DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector.at(index).width - outline_border_size * 2, keyVector.at(index).height - outline_border_size * 2, -1, D3DCOLOR_ARGB(220, 0, 128, 0), TRANSLUCENCY_GLOWING);
				else
					DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector.at(index).width - outline_border_size * 2, keyVector.at(index).height - outline_border_size * 2, -1, D3DCOLOR_ARGB(220, 38, 80, 145), TRANSLUCENCY_GLOWING);

				// now draw text, but figure out how wide the string is first.. do this once on init?
				std::string tempString = Osk_GetKeyLabel(index);
				const char *tempPtr = tempString.c_str();
				int labelWidth = 0;
				int positionX = 0;

				while (*tempPtr)
				{
					labelWidth += AAFontWidths[(unsigned char)*tempPtr++];
				}

				positionX = pos_x + ((keyVector.at(index).width - labelWidth) / 2);

//				RenderSmallMenuText((char*)Osk_GetKeyLabel(index).c_str(), pos_x + (keyVector.at(index).width / 2)/*(keyVector.at(index).width - outline_border_size * 2 / 2)*/, pos_y + space_between_keys, ONE_FIXED, AVPMENUFORMAT_LEFTJUSTIFIED);
				RenderSmallMenuText((char*)tempString.c_str(), positionX, pos_y + space_between_keys, ONE_FIXED, AVPMENUFORMAT_LEFTJUSTIFIED);
				//RenderMenuText((char*)Osk_GetKeyLabel(index).c_str(), pos_x + (keyVector.at(index).width / 2), pos_y + space_between_keys, ONE_FIXED, AVPMENUFORMAT_LEFTJUSTIFIED);
			}

			pos_x += (keyVector.at(index).width + space_between_keys);
			widthCount -= keyVector.at(index).numWidthBlocks;

			index += keyVector.at(index).numWidthBlocks;
		}
		pos_y += (keyHeight + space_between_keys);
	}
}

bool Osk_IsActive()
{
	return is_active; // sort this later to only appear for text entry on xbox
}

std::string Osk_GetKeyLabel(int buttonIndex)
{
	// quick test for shift and caps lock..
	if ((shift || capsLock) && ((stringVector.at(keyVector.at(buttonIndex).stringId).length() == 1)))
	{
		std::string tempString = stringVector.at(keyVector.at(buttonIndex).stringId);

		std::transform(tempString.begin(), tempString.end(), tempString.begin(), toupper);

		return tempString;
	}

	return stringVector.at(keyVector.at(buttonIndex).stringId);
}

void Osk_Activate()
{
	return;

	if (is_inited == false)
		Osk_Init();

	is_active = true;
}

void Osk_Deactivate()
{
	is_active = false;
}

#ifdef _XBOX
extern void AddKeyToQueue(char virtualKeyCode);
#endif

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
	int width = keyVector.at(currentPosition).numWidthBlocks;

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

	int buttonOffset = keyVector.at(currentPosition).positionOffset;
	int width = keyVector.at(currentPosition).numWidthBlocks;

	currentColumn += width - buttonOffset;

	// wrap?
	if (currentColumn >= numHorizontalKeys) // add some sort of numColumns?
		currentColumn = 0;

	// where are we now?
	currentPosition = (currentRow * numHorizontalKeys) + currentColumn;

	// handle moving over any blank keys
	while (keyVector.at(currentPosition).isBlank)
	{
		buttonOffset = keyVector.at(currentPosition).positionOffset;

		currentColumn += width - buttonOffset;

		// wrap?
		if (currentColumn >= numHorizontalKeys) // add some sort of numColumns?
			currentColumn = 0;

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