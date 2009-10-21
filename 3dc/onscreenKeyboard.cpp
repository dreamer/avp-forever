
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
static int columnOffset = 0;

static int osk_x = 320;
static int osk_y = 320;
static int oskWidth = 400;
static int oskHeight = 200;

static const int keyWidth = 30;
static const int keyHeight = 30;

static const int space_between_keys = 3;
static const int outline_border_size = 1;
static const int indent_space = 5;

struct BUTTONS 
{
	int height;
	int width;
	int positionOffset;
	std::string name;
};

std::vector<BUTTONS> keyVector;

const int numVerticalKeys = 5;
const int numHorizontalKeys = 11;

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


template <class T> void Osk_AddKey(T name, int width)
{
	BUTTONS newButton = {0};

	newButton.width		= width;
	newButton.height	= keyHeight;
	newButton.name		= name;
	newButton.positionOffset = 0;

	keyVector.push_back(newButton);
}

void Osk_Init()
{
	currentRow = 0;
	currentColumn = 0;

	// do top row of numbers
	for (int i = 9; i >= 0; i--)
	{
		Osk_AddKey(IntToString(i), keyWidth);
	}

	Osk_AddKey("Shift", (keyWidth * 2) + (space_between_keys));

	// second row..
	for (char letter = 'a'; letter <= 'j'; letter++)
	{
		Osk_AddKey(letter, keyWidth);
	}

	Osk_AddKey("Symbols", (keyWidth * 2) + (space_between_keys));

	// third row..
	for (char letter = 'k'; letter <= 't'; letter++)
	{
		Osk_AddKey(letter, keyWidth);
	}

	Osk_AddKey("Dunno", (keyWidth * 2) + (space_between_keys));

	// fourth row..
	for (char letter = 'u'; letter <= 'z'; letter++)
	{
		Osk_AddKey(letter, keyWidth);
	}

	Osk_AddKey("Backspace", (keyWidth * 4) + (space_between_keys * 3));
	Osk_AddKey("Done",		(keyWidth * 2) + (space_between_keys * 1));

	// fifth row
	Osk_AddKey("Space", (keyWidth * 6) + (space_between_keys * 5));
	Osk_AddKey("<",		(keyWidth * 2) + (space_between_keys * 1));
	Osk_AddKey(">",		(keyWidth * 2) + (space_between_keys * 1));
	Osk_AddKey("Blank", (keyWidth * 2) + (space_between_keys * 1));

	sprintf(buf, "number of keys added to osk: %d\n", keyVector.size());
	OutputDebugString(buf);

	currentValue = 0;

	oskWidth = (keyWidth * numHorizontalKeys) + (space_between_keys * numHorizontalKeys) + (indent_space * 2);
	oskHeight = (keyHeight * numVerticalKeys) + (space_between_keys * numVerticalKeys) + (indent_space * 2);
}

void Osk_Draw()
{
	if (!Osk_IsActive()) 
		return;

	int totalDrawn = 0;

	osk_x = (640 - oskWidth) / 2;

//	sprintf(buf, "currentRow: %d, currentColumn: %d, currentItem: %d curentChar: %s\n", currentRow, currentColumn, Osk_GetCurrentLocation(), keyVector.at(Osk_GetCurrentLocation()).name.c_str());
//	OutputDebugString(buf);

	// draw background rectangle
	DrawQuad(osk_x, osk_y, oskWidth, oskHeight, D3DCOLOR_ARGB(120, 80, 160, 120));

	// start off with an indent to space things out nicely
	int pos_x = osk_x + indent_space;
	int pos_y = osk_y + indent_space;

	int innerSquareWidth = keyWidth - outline_border_size * 2;
	int innerSquareHeight = keyHeight - outline_border_size * 2;

	int index = 0;

	for (int y = 0; y < numVerticalKeys; y++)
	{
		// reset x position each time we move to a new row
		pos_x = osk_x + indent_space;

		for (int x = 0; x < buttonsPerRow[y]; x++)
		{
			DrawQuad(pos_x, pos_y, keyVector.at(index).width, keyVector.at(index).height, D3DCOLOR_ARGB(200, 255, 255, 255));

			// draw the inner background for key, highlighting if its the currently selected key
			if (Osk_GetCurrentLocation() == index) // draw the selected item differently (highlight it)
				DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector.at(index).width - outline_border_size * 2, keyVector.at(index).height - outline_border_size * 2, D3DCOLOR_ARGB(220, 255, 255, 0));
			else
				DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector.at(index).width - outline_border_size * 2, keyVector.at(index).height - outline_border_size * 2, D3DCOLOR_ARGB(220, 128, 128, 128));

			RenderSmallMenuText((char*)keyVector.at(index).name.c_str(), pos_x + (keyVector.at(index).width - outline_border_size * 2 / 2), pos_y + space_between_keys, ONE_FIXED, AVPMENUFORMAT_CENTREJUSTIFIED);

			pos_x += (keyVector.at(index).width + space_between_keys);
			index++;
		}

		pos_y += (keyHeight + space_between_keys);
	}
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
	else 
		currentValue--;

	keyVector.at(currentValue).positionOffset = 0;

//	sprintf(buf, "currentValue %d\n", currentValue);
//	OutputDebugString(buf);
}

void Osk_MoveRight()
{
	int currentColOffset = currentValue % buttonsPerRow[currentRow];

	currentColumn += 1;

	if (currentColumn >= numHorizontalKeys)
		currentColumn = 0;

	if (currentColOffset == buttonsPerRow[currentRow] - 1)
	{
		currentValue -= buttonsPerRow[currentRow] - 1;
	}
	else
		currentValue++;

	keyVector.at(currentValue).positionOffset = 0;

//	sprintf(buf, "currentValue %d\n", currentValue);
//	OutputDebugString(buf);
}

void Osk_MoveUp()
{
	// keep a record of which column we've moved from so we can move back from a big button
//	previousColumn = currentValue % numHorizontalKeys;

	int addme = 0;
	int widthCount = numHorizontalKeys;
	int startValue = currentValue;

	// are we moving upwards from a big button?
	if (keyVector.at(currentValue).positionOffset)
	{
		addme = keyVector.at(currentValue).positionOffset;
	}

	currentRow -= 1;

	if (currentRow < 0)
		currentRow = 0;

	while (widthCount >= 0)
	{
		currentValue--;

		if (currentValue < 0)
			currentValue = keyVector.size() + currentValue;

		widthCount -= keyVector.at(currentValue).width / keyWidth;
	}

	// did we land on a big button? remember which column from above that we came from
//	if (keyVector.at(currentValue).width > 30)
	{
		// how far into button are we?
		keyVector.at(currentValue).positionOffset = numHorizontalKeys - (startValue - currentValue);
		sprintf(buf, "offset into big button is: %d\n", keyVector.at(currentValue).positionOffset);
		OutputDebugString(buf);
	}

	if (keyVector.at(currentValue).width == keyWidth)
	{
		// we landed on a small button
		currentValue += addme;
		keyVector.at(currentValue).positionOffset = 0;
	}
//	sprintf(buf, "currentValue %d\n", currentValue);
//	OutputDebugString(buf);
}

void Osk_MoveDown() 
{
	int startValue = currentValue;

	// what column are we currently on?
	int thisColumn = currentValue % numHorizontalKeys;

	sprintf(buf, "pre move, we're on column: %d\n", thisColumn);
	OutputDebugString(buf);

	sprintf(buf, "pre move, we're on key value: %d\n", currentValue);
	OutputDebugString(buf);

	// drop down a row
	currentRow += 1;
	if (currentRow > numHorizontalKeys - 1)
		currentRow = numHorizontalKeys - 1;

	int widthCount = numHorizontalKeys;

	while (widthCount >= 0)
	{
		currentValue++;

		if (currentValue >= keyVector.size())
			currentValue = currentValue - keyVector.size();

		widthCount -= keyVector.at(currentValue).width / keyWidth;
	}

//	int tempCurrent = currentValue;

//	currentValue += columnOffset;

	// did we land on a big button? remember which column from above that we came from
/*
	if (keyVector.at(tempCurrent).width > keyWidth)
	{
		// how far into button are we?
		columnOffset = numHorizontalKeys - (tempCurrent - startValue);

//		keyVector.at(currentValue).positionOffset = numHorizontalKeys - (currentValue - startValue);
//		sprintf(buf, "offset into big button is: %d\n", keyVector.at(currentValue).positionOffset);
//		OutputDebugString(buf);
	}
	else columnOffset = 0;
*/
/*
	if (keyVector.at(currentValue).width == 30)
	{
		// we landed on a small button
		currentValue += addme;
		keyVector.at(currentValue).positionOffset = 0;
	}
*/
//	sprintf(buf, "currentValue %d\n", currentValue);
//	OutputDebugString(buf);

	// what column are we currently on?
	thisColumn = currentValue % numHorizontalKeys;

	sprintf(buf, "post move, we're on column: %d\n", thisColumn);
	OutputDebugString(buf);

	sprintf(buf, "post move, we're on key value: %d\n", currentValue);
	OutputDebugString(buf);
}