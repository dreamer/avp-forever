
#include "onscreenKeyboard.h"
#include "d3_func.h"
#include "stdio.h"

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

static int osk_x = 0;
static int osk_y = 0;
static int oskWidth = 400;
static int oskHeight = 200;

static const int outline_square_width = 30;
static const int outline_square_height = 30;
static const int space_between_keys = 3;
static const int outline_border_size = 1;
static const int indent_space = 5;

const int numTotalKeys = 40;//36; // 26 letters + 0 to 9

const int keysPerRow = 4;
const int keysPerColumn = 10;

static char buf[100];

static bool is_active = false;

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

void Osk_Init()
{
	currentRow = 0;
	currentColumn = 0;

	oskWidth = (outline_square_width * keysPerColumn) + (space_between_keys * keysPerColumn) + (indent_space * 2);
	oskHeight = (outline_square_height * keysPerRow) + (space_between_keys * keysPerRow) + (indent_space * 2);
}

void Osk_Draw()
{
	if (!Osk_IsActive()) 
		return;

	int totalDrawn = 0;

//	sprintf(buf, "currentRow: %d, currentColumn: %d, currentItem: %d\n", currentRow, currentColumn, Osk_GetCurrentLocation());
//	OutputDebugString(buf);

	// draw background rectangle
	DrawQuad(osk_x, osk_y, oskWidth, oskHeight, D3DCOLOR_ARGB(120, 80, 160, 120));

	// start off with an indent to space things out nicely
	int pos_x = indent_space;
	int pos_y = indent_space;

	// draw each key
	for (int y = 0; y < keysPerRow; y++)
	{
		// reset x position each time we move to a new row
		pos_x = indent_space;

		for (int x = 0; x < keysPerColumn; x++)
		{
			if (totalDrawn >= numTotalKeys) 
				return; // just break out of the function if we've drawn all the keys

			// draw background square first (for key outline)
			DrawQuad(pos_x, pos_y, outline_square_width, outline_square_height, D3DCOLOR_ARGB(200, 255, 255, 255));

			int innerSquareWidth = outline_square_width - outline_border_size * 2;
			int innerSquareHeight = outline_square_height - outline_border_size * 2;

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
}

bool Osk_IsActive()
{
	return is_active; // sort this later to only appear for text entry on xbox
}

char Osk_GetSelectedKeyChar()
{
	return keyArray[Osk_GetCurrentLocation()];
}

char Osk_GetSpecifiedKeyChar(int key)
{
	return keyArray[key];
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
/*
	currentColumn -= 1;
	
	if (currentColumn < 0)
//		currentColumn = (keysPerRow - 1); 
		currentColumn = 0;
*/

	int currentColOffset = currentValue % keysPerColumn;

//	sprintf(buf, "currentColOffset %d currentPosition %d\n", currentColOffset, currentValue);
//	OutputDebugString(buf);

	if (currentColOffset == 0)
	{
		currentValue += keysPerColumn - 1;
	}
	else currentValue--;
}

void Osk_MoveRight()
{
/*
	currentColumn += 1;

	if (currentColumn >= keysPerRow - 1)
		currentColumn = keysPerRow - 1;//0;
*/

	int currentColOffset = currentValue % keysPerColumn;

//	sprintf(buf, "currentColOffset %d currentPosition %d\n", currentColOffset, currentValue);
//
	OutputDebugString(buf);

	if (currentColOffset == keysPerColumn - 1)
	{
		currentValue -= keysPerColumn - 1;
	}
	else currentValue++;
}

void Osk_MoveUp()
{
	currentRow -= 1;

	if (currentRow < 0)
		currentRow = 0;

	currentValue -= keysPerColumn;
	if (currentValue < 0)
		currentValue = numTotalKeys + currentValue;
}

void Osk_MoveDown()
{
	currentRow += 1;

	if (currentRow >= keysPerColumn - 1)
		//currentRow = 0;
		currentRow = keysPerColumn - 1;

	currentValue += keysPerColumn;
	if (currentValue >= numTotalKeys)
		currentValue = currentValue - numTotalKeys;
}