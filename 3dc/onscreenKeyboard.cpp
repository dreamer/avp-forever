
#include "onscreenKeyboard.h"
#include "logString.h"
#include "d3_func.h"
#include "stdio.h"
#include <string>
#include <vector>

extern "C" {
extern void D3D_DrawRectangle(int x, int y, int w, int h, int alpha);
};

extern "C" 
{
	#include "avp_menugfx.hpp"
}

#define ONE_FIXED	65536

static int Osk_GetCurrentLocation();
char Osk_GetSelectedKeyChar();
char Osk_GetSpecifiedKeyChar(int key);

static int currentRow = 0;
static int currentColumn = 0;

static int currentValue = 0;
static int previousValue = 0;

static int osk_x = 320;
static int osk_y = 320;
static int oskWidth = 400;
static int oskHeight = 200;

static const int outline_square_width = 30; // rename
static const int outline_square_height = 30;
static const int space_between_keys = 3;
static const int outline_border_size = 1;
static const int indent_space = 5;

struct BUTTONS 
{
	int height;
	int width;
	std::string name;
};

std::vector<BUTTONS> keyVector;

const int numVerticalKeys = 5;
const int numHorizontalKeys = 11;
const int numTotalKeys = 0;

static char buf[100];

static bool is_active = false;

/*
const static char keyArray[numTotalKeys] = 
{
	// 0 - 9
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	// 10 - 19
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	// 20 - 29
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 
	// 30 - 35
	'u', 'v', 'w', 'x', 'y', 'z'
};
*/

int buttonsPerRow[5] = {11, 11, 11, 8, 4};

void Osk_Init()
{
	currentRow = 0;
	currentColumn = 0;

	BUTTONS newButton = {0};

	// do top row of numbers
	for (int i = 9; i >= 0; i--)
	{
		newButton.height = 30;
		newButton.width = 30;
		newButton.name = IntToString(i);
		keyVector.push_back(newButton);
	}

	newButton.height = 30;
	newButton.width = (outline_square_width * 2) + (space_between_keys);
	newButton.name = "Shift";
	keyVector.push_back(newButton);

	// second row..
	for (char letter = 'a'; letter <= 'j'; letter++)
	{
		newButton.height = 30;
		newButton.width = 30;
		newButton.name = letter;
		keyVector.push_back(newButton);
	}

	newButton.height = 30;
	newButton.width = (outline_square_width * 2) + (space_between_keys);
	newButton.name = "Symbols";
	keyVector.push_back(newButton);

	// third row..
	for (char letter = 'k'; letter <= 't'; letter++)
	{
		newButton.height = 30;
		newButton.width = 30;
		newButton.name = letter;
		keyVector.push_back(newButton);
	}

	newButton.height = 30;
	newButton.width = (outline_square_width * 2) + (space_between_keys);
	newButton.name = "Dunno";
	keyVector.push_back(newButton);

	// fourth row..
	for (char letter = 'u'; letter <= 'z'; letter++)
	{
		newButton.height = 30;
		newButton.width = 30;
		newButton.name = letter;
		keyVector.push_back(newButton);
	}

	newButton.height = 30;
	newButton.width = (outline_square_width * 4) + (space_between_keys * 3);
	newButton.name = "Backspace";
	keyVector.push_back(newButton);

	newButton.height = 30;
	newButton.width = (outline_square_width * 2) + (space_between_keys * 1);
	newButton.name = "Done";
	keyVector.push_back(newButton);

	// fifth row
	newButton.height = 30;
	newButton.width = (outline_square_width * 6) + (space_between_keys * 5);
	newButton.name = "Space";
	keyVector.push_back(newButton);

	newButton.height = 30;
	newButton.width = (outline_square_width * 2) + (space_between_keys * 1);
	newButton.name = "<";
	keyVector.push_back(newButton);

	newButton.height = 30;
	newButton.width = (outline_square_width * 2) + (space_between_keys * 1);
	newButton.name = ">";
	keyVector.push_back(newButton);

	newButton.height = 30;
	newButton.width = (outline_square_width * 2) + (space_between_keys * 1);
	newButton.name = "Blank";
	keyVector.push_back(newButton);

	sprintf(buf, "number of keys added to osk: %d\n", keyVector.size());
	OutputDebugString(buf);

	currentValue = 0;

	oskWidth = (outline_square_width * numHorizontalKeys) + (space_between_keys * numHorizontalKeys) + (indent_space * 2);
	oskHeight = (outline_square_height * numVerticalKeys) + (space_between_keys * numVerticalKeys) + (indent_space * 2);
}

void Osk_Draw()
{
	if (!Osk_IsActive()) 
		return;

	int totalDrawn = 0;

	osk_x = (640 - oskWidth) / 2;

//	sprintf(buf, "currentRow: %d, currentColumn: %d, currentItem: %d\n", currentRow, currentColumn, Osk_GetCurrentLocation());
//	OutputDebugString(buf);

	// draw background rectangle
	DrawQuad(osk_x, osk_y, oskWidth, oskHeight, D3DCOLOR_ARGB(120, 80, 160, 120));

	// start off with an indent to space things out nicely
	int pos_x = osk_x + indent_space;
	int pos_y = osk_y + indent_space;

	int innerSquareWidth = outline_square_width - outline_border_size * 2;
	int innerSquareHeight = outline_square_height - outline_border_size * 2;

	int index = 0;

	for (int y = 0; y < numVerticalKeys; y++)
	{
		// reset x position each time we move to a new row
		pos_x = osk_x + indent_space;

		for (int x = 0; x < buttonsPerRow[y]; x++)
		{
			DrawQuad(pos_x, pos_y, keyVector[index].width, keyVector[index].height, D3DCOLOR_ARGB(200, 255, 255, 255));

			// draw the inner background for key, highlighting if its the currently selected key
			if (Osk_GetCurrentLocation() == index) // draw the selected item differently (highlight it)
				DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector[index].width - outline_border_size * 2, keyVector[index].height - outline_border_size * 2, D3DCOLOR_ARGB(220, 255, 255, 0));
			else
				DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector[index].width - outline_border_size * 2, keyVector[index].height - outline_border_size * 2, D3DCOLOR_ARGB(220, 128, 128, 128));

//			RenderSmallMenuText((char*)keyVector[index].name.c_str(), pos_x + (keyVector[index].width - outline_border_size * 2 / 2), pos_y + space_between_keys, ONE_FIXED, AVPMENUFORMAT_CENTREJUSTIFIED);

			pos_x += (keyVector[index].width + space_between_keys);
			index++;
		}

		pos_y += (outline_square_height + space_between_keys);
	}
#if 0
	// draw each key
	for (int y = 0; y < keysPerRow; y++)
	{
		// reset x position each time we move to a new row
		pos_x = osk_x + indent_space;

		for (int x = 0; x < keysPerColumn; x++)
		{
			if (totalDrawn >= numTotalKeys) 
				return; // just break out of the function if we've drawn all the keys

			// draw background square first (for key outline)
			DrawQuad(pos_x, pos_y, outline_square_width, outline_square_height, D3DCOLOR_ARGB(200, 255, 255, 255));

			// draw the inner background for key, highlighting if its the currently selected key
			if (Osk_GetCurrentLocation() == totalDrawn) // draw the selected item differently (highlight it)
				DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, innerSquareWidth, innerSquareHeight, D3DCOLOR_ARGB(220, 255, 255, 0));
			else
				DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, innerSquareWidth, innerSquareHeight, D3DCOLOR_ARGB(220, 128, 128, 128));

			// draw key letter
			RenderSmallChar(Osk_GetSpecifiedKeyChar(totalDrawn), pos_x + (26 / 2), pos_y + space_between_keys, ONE_FIXED, ONE_FIXED, ONE_FIXED, ONE_FIXED);

			pos_x += (outline_square_width + space_between_keys);
			totalDrawn++;
		}

		pos_y += (outline_square_height + space_between_keys);
	}

	pos_x = osk_x + indent_space;

	// draw extra button row
	DrawQuad(pos_x, pos_y, outline_square_width * 5, outline_square_height, D3DCOLOR_ARGB(200, 255, 255, 255));

	pos_x = osk_x + indent_space + outline_square_width * 5;

	DrawQuad(pos_x, pos_y, outline_square_width * 5, outline_square_height, D3DCOLOR_ARGB(200, 255, 255, 255));
#endif
}

bool Osk_IsActive()
{
	return is_active; // sort this later to only appear for text entry on xbox
}

char Osk_GetSelectedKeyChar()
{
	return 'a';//keyVector[Osk_GetCurrentLocation()].name;
}

char Osk_GetSpecifiedKeyChar(int key)
{
	return 'a';//keyArray[key];
}

void Osk_Activate()
{
	if (is_active == false)
		Osk_Init();

	is_active = true;
}

void Osk_Deactivate()
{
	is_active = false;
}

static int Osk_GetCurrentLocation()
{
	return currentValue;
}

void Osk_MoveLeft()
{
	int currentColOffset = currentValue % /*keysPerColumn*/buttonsPerRow[currentRow];

//	sprintf(buf, "currentColOffset %d currentPosition %d\n", currentColOffset, currentValue);
//	OutputDebugString(buf);

	if (currentColumn < 0)
		currentColumn = numHorizontalKeys - 1;

	if (currentColOffset == 0)
	{
		currentValue += buttonsPerRow[currentRow] - 1;
	}
	else currentValue--;

	sprintf(buf, "currentValue %d\n", currentValue);
	OutputDebugString(buf);
}

void Osk_MoveRight()
{
	int currentColOffset = currentValue % /*keysPerColumn*/buttonsPerRow[currentRow];

	currentColumn += 1;

	if (currentColumn >= numHorizontalKeys)
		currentColumn = 0;

//	sprintf(buf, "currentColOffset %d currentPosition %d\n", currentColOffset, currentValue);
//	OutputDebugString(buf);

	if (currentColOffset == buttonsPerRow[currentRow] - 1)
	{
		currentValue -= buttonsPerRow[currentRow] - 1;
	}
	else currentValue++;

	sprintf(buf, "currentValue %d\n", currentValue);
	OutputDebugString(buf);
}

void Osk_MoveUp()
{
	// keep a record of which column we've moved from so we can move back from a big button
//	previousColumn = currentValue % numHorizontalKeys;

	int widthCount = numHorizontalKeys;

	currentRow -= 1;

	if (currentRow < 0)
		currentRow = 0;

	while (widthCount > 0)
	{
		currentValue -= keyVector[currentValue].width / 30; // increment in blocks, where some keys are multiple blocks wide
		widthCount -= keyVector[currentValue].width / 30;
	}

//	currentValue -= numHorizontalKeys;
	if (currentValue < 0)
		currentValue = keyVector.size() + currentValue;

	sprintf(buf, "currentValue %d\n", currentValue);
	OutputDebugString(buf);
}

void Osk_MoveDown()
{
//	previousColumn = currentValue % numHorizontalKeys;

	int widthCount = numHorizontalKeys;

	currentRow += 1;

	if (currentRow >= numHorizontalKeys - 1)
		currentRow = numHorizontalKeys - 1;

	previousValue = currentValue;

	while (widthCount > 0)
	{
		currentValue += keyVector[currentValue].width / 30; // increment in blocks, where some keys are multiple blocks wide
		widthCount -= keyVector[currentValue].width / 30;
	}

	if (currentValue >= keyVector.size())
		currentValue = currentValue - keyVector.size();

	sprintf(buf, "currentValue %d\n", currentValue);
	OutputDebugString(buf);
}