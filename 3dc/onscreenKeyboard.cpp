
#include "onscreenKeyboard.h"
#include "logString.h"
#include "d3_func.h"
#include "stdio.h"
#include <string>
#include <vector>
#include <assert.h>

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
static int currentKey = 0;

static int columnOffset = 0;

static int osk_x = 320;
static int osk_y = 280;
static int oskWidth = 400;
static int oskHeight = 200;

static const int keyWidth = 30;
static const int keyHeight = 30;

static const int space_between_keys = 3;
static const int outline_border_size = 1;
static const int indent_space = 5;
/*
struct ButtonStruct 
{
	int height;
	int width;
	int positionOffset;
	std::string name;
};
std::vector<ButtonStruct> keyVector;
*/
struct ButtonStruct
{
	int id;
	int numRowBlocks;
	int height;
	int width;
	int positionOffset;
	int stringId;
};
std::vector<ButtonStruct> keyVector;
std::vector<std::string> stringVector;

const int numVerticalKeys = 5;
const int numHorizontalKeys = 12;
const int numKeys = numVerticalKeys * numHorizontalKeys;

static char buf[100];

static bool is_active = false;
static bool is_inited = false;


//int buttonsPerRow[5] = {11, 11, 11, 8, 4};

int id = 0;

template <class T> void Osk_AddKey(T name, int width)
{
	ButtonStruct newButton = {0};

	newButton.id = id;
	newButton.numRowBlocks = width;
	newButton.width = (width * keyWidth) + space_between_keys * (width - 1);
	newButton.height = keyHeight;

	// store the string in its own vector
//	stringVector.push_back(name);

	int position = 0;

	int tempWidth = width;

	while (tempWidth)
	{
		newButton.positionOffset = position;
		keyVector.push_back(newButton);
		position++;
		tempWidth--;
	}

	id++;

/*
	ButtonStruct newButton = {0};

	newButton.width		= width;
	newButton.height	= keyHeight;
	newButton.name		= name;
	newButton.positionOffset = 0;

	keyVector.push_back(newButton);
*/
}

void Osk_Init()
{
	currentRow = 0;
	currentColumn = 0;

	// do top row of numbers
	for (int i = 9; i >= 0; i--)
	{
		Osk_AddKey(IntToString(i), 1);
	}

	Osk_AddKey("Shift", 2);

	// second row..
	for (char letter = 'a'; letter <= 'j'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Symbols", 2);

	// third row..
	for (char letter = 'k'; letter <= 't'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Dunno", 2);

	// fourth row..
	for (char letter = 'u'; letter <= 'z'; letter++)
	{
		Osk_AddKey(letter, 1);
	}

	Osk_AddKey("Backspace", 4);
	Osk_AddKey("Done",		2);

	// fifth row
	Osk_AddKey("Space", 6);
	Osk_AddKey("<",		2);
	Osk_AddKey(">",		2);
	Osk_AddKey("Blank", 2);

	sprintf(buf, "number of keys added to osk: %d\n", keyVector.size());
	OutputDebugString(buf);

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

	char *letter = "a";

	for (int y = 0; y < numVerticalKeys; y++)
	{
		pos_x = osk_x + indent_space;

		int widthCount = 12;

		while (widthCount)
		{
			DrawQuad(pos_x, pos_y, keyVector.at(index).width, keyVector.at(index).height, D3DCOLOR_ARGB(200, 255, 255, 255));

			if (Osk_GetCurrentLocation() == index) // draw the selected item differently (highlight it)
				DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector.at(index).width - outline_border_size * 2, keyVector.at(index).height - outline_border_size * 2, D3DCOLOR_ARGB(220, 255, 255, 0));
			else
				DrawQuad(pos_x + outline_border_size, pos_y + outline_border_size, keyVector.at(index).width - outline_border_size * 2, keyVector.at(index).height - outline_border_size * 2, D3DCOLOR_ARGB(220, 128, 128, 128));

			pos_x += (keyVector.at(index).width + space_between_keys);
			widthCount -= keyVector.at(index).numRowBlocks;

			RenderSmallMenuText(letter, pos_x + (keyVector.at(index).width - outline_border_size * 2 / 2), pos_y + space_between_keys, ONE_FIXED, AVPMENUFORMAT_CENTREJUSTIFIED);

			index += keyVector.at(index).numRowBlocks;
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
	if (is_inited == false)
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
	// where are we now?
	int currentPosition = (currentRow * 12) + currentColumn;

	int buttonOffset = keyVector.at(currentPosition).positionOffset;
	int width = keyVector.at(currentPosition).numRowBlocks;

//	currentColumn--;
	currentColumn -= /*width +*/ buttonOffset + 1;//buttonOffset;

	// wrap?
	if (currentColumn < 0)
		currentColumn = numHorizontalKeys - 1;

	// move left across one whole key
//	int keyOffset = keyVector.at(currentValue).positionOffset;

//	int rowStartValue = (12 * currentRow);

//	currentValue--;
//	currentValue -= keyOffset;

//	if (currentValue < rowStartValue)
//		currentValue += 12;

	// where are we now?
	currentPosition = (currentRow * 12) + currentColumn;

	currentValue = currentPosition;

	// then align to button left..
	int keyOffset = keyVector.at(currentValue).positionOffset;
	currentValue -= keyOffset;
}

void Osk_MoveRight()
{
	// where are we now?

	int currentPosition = (currentRow * 12) + currentColumn;

	int buttonOffset = keyVector.at(currentPosition).positionOffset;
	int width = keyVector.at(currentPosition).numRowBlocks;

//	currentColumn++;
	currentColumn += width - buttonOffset;//buttonOffset;

	// wrap?
	if (currentColumn > numHorizontalKeys)
		currentColumn = 0;

	// where are we now?
	currentPosition = (currentRow * 12) + currentColumn;

	currentValue = currentPosition;

	// then align to button left..
	int keyOffset = keyVector.at(currentValue).positionOffset;
	currentValue -= keyOffset;

#if 0
	int colOffset = keyVector.at((currentRow * 12) + currentColumn).positionOffset;
	int width = keyVector.at(currentValue).numRowBlocks;

	currentColumn++;
	currentColumn += colOffset;

	// wrap?
	if (currentColumn > numHorizontalKeys)
		currentColumn = 0;

	// move right across one whole key
	int keyOffset = keyVector.at(currentValue).positionOffset;

	int rowEndValue = (12 * currentRow) + 12;

	currentValue += (width - keyOffset);

	if (currentValue >= rowEndValue)
		currentValue -= 12;

	// then align to button left..
	keyOffset = keyVector.at(currentValue).positionOffset;

	currentValue -= keyOffset;
#endif
}

void Osk_MoveUp()
{
	currentRow--;

	// make sure we're not outside the grid
	if (currentRow < 0)
		currentRow = 4;

	currentValue = (currentRow * 12) + currentColumn;

	// left align button value
	int keyOffset = keyVector.at(currentValue).positionOffset;
	currentValue -= keyOffset;
}

void Osk_MoveDown() 
{
	currentRow++;

	// make sure we're not outside the grid
	if (currentRow > 4)
		currentRow = 0;

	currentValue = (currentRow * 12) + currentColumn;

	// left align button value
	int keyOffset = keyVector.at(currentValue).positionOffset;
	currentValue -= keyOffset;
}