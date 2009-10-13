
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

static int currentRow = 0;
static int currentColumn = 0;

static int currentValue = 0;

static int oskX = 0;
static int oskY = 0;
static int oskWidth = 400; //?
static int oskHeight = 200; //?

const int numTotalKeys = 40;//36; // 26 letters + 0 to 9

const int keysPerRow = 4;
const int keysPerColumn = 10;

static char buf[100];

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
}

void Osk_Draw()
{
	int totalDrawn = 0;

//	sprintf(buf, "currentRow: %d, currentColumn: %d, currentItem: %d\n", currentRow, currentColumn, Osk_GetCurrentLocation());
//	OutputDebugString(buf);

	// draw background rectangle
	DrawQuad(oskX, oskY, oskWidth, oskHeight, D3DCOLOR_ARGB(255, 80, 160, 120));

	int pos_x = 5;
	int pos_y = 5;

	// draw each key outline square
	for (int y = 0; y < keysPerRow; y++)
	{
		pos_x = 5;

		for (int x = 0; x < keysPerColumn; x++)
		{
			if (totalDrawn >= numTotalKeys) goto stopdraw;

			// draw background square first (for key outline)
			DrawQuad(pos_x, pos_y, 30, 30, D3DCOLOR_ARGB(255, 255, 255, 255));

			if (Osk_GetCurrentLocation() == totalDrawn)
				DrawQuad(pos_x+2, pos_y+2, 26, 26, D3DCOLOR_ARGB(255, 255, 255, 0));
			else
				DrawQuad(pos_x+2, pos_y+2, 26, 26, D3DCOLOR_ARGB(255, 128, 128, 128));


			// draw key letter
			RenderSmallChar(keyArray[totalDrawn], pos_x+2, pos_y+2, ONE_FIXED, ONE_FIXED, ONE_FIXED, ONE_FIXED);

			pos_x += 35;
			totalDrawn++;
		}

		pos_y += 35;
	}

stopdraw: {}

	// draw selected key highlight? (or do it above if i == currentKey or whatever)

}

bool Osk_IsActive()
{
	return true; // sort this later to only appear for text entry on xbox
}

static int Osk_GetCurrentLocation()
{
//	return (currentRow * ) + 1;
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
	currentValue--;

	int currentColOffset = currentValue % keysPerColumn;

	sprintf(buf, "currentColOffset %d currentPosition %d\n", currentColOffset, currentValue);
	OutputDebugString(buf);

	if (currentColOffset < 0)
		currentValue += keysPerColumn;
}

void Osk_MoveRight()
{
/*
	currentColumn += 1;

	if (currentColumn >= keysPerRow - 1)
		currentColumn = keysPerRow - 1;//0;
*/
	currentValue++;

	int currentColOffset = currentValue % keysPerColumn;

//	sprintf(buf, "currentColOffset %d currentPosition %d\n", currentColOffset, currentValue);
//	OutputDebugString(buf);

	if (currentColOffset == 0)
	{
		currentValue -= keysPerColumn;
	}
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