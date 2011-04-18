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

/*
 * this code is written to use fonts created with Codehead's Bitmap Font Generator
 * from http://www.codehead.co.uk/cbfg/
 */

#include "renderer.h"
#include "logString.h"
#include "font2.h"
#include "TextureManager.h"
#include <iostream>
#include <fstream>

#pragma pack(1) // ensure no padding on struct

struct BFD
{
	uint32_t	mapWidth;
	uint32_t	mapHeight;
	uint32_t	cellWidth;
	uint32_t	cellHeight;
	char		startChar;
	char		charWidths[256];
};

#pragma pack()

struct Font
{
	enum FONT_TYPE  type;
	texID_t	    textureID;
	uint32_t	mapWidth;
	uint32_t	mapHeight;
	uint32_t	cellWidth;
	uint32_t	cellHeight;
	char		startChar;
	char		charWidths[256];
};

static Font Fonts[NUM_FONT_TYPES];

void Font_Release()
{
	for (uint32_t i = 0; i < NUM_FONT_TYPES; i++)
	{
		Tex_Release(Fonts[i].textureID);
	}
}

void Font_Init()
{
	Fonts[FONT_SMALL].textureID = Tex_CreateFromFile("aa_font_512.png");

	uint8_t *srcPtr = NULL;
	uint32_t pitch = 0;

	// calculate widths
	if (!Tex_Lock(Fonts[FONT_SMALL].textureID, &srcPtr, &pitch))
	{
//		Con_PrintError("CalculateWidthsOfAAFont() failed - Can't lock texture");
		return;
	}

	uint8_t theWidths[256];

	uint8_t *s = NULL;

	uint8_t *theWidth = 0;

	uint32_t width = 0;

	for (uint32_t i = 0; i < 256; i++)
	{
		uint32_t row = i / 16; // get row
		uint32_t column = i % 16; // get column from remainder value

		uint32_t offset = ((32 * pitch) * row) + ((column*32)*4);

		s = &srcPtr[offset];

		// pink highlight block starts
/*
		*s = 255; s++; // B
		*s = 0; s++;   // G
		*s = 255; s++; // R
		*s = 255; s++; // A
		s-=4;
*/
		width = 0;
		theWidth = 0;

		for (int y = 0; y < 31; y++)
		{
			s = &srcPtr[offset+((y+1)*pitch)]; // FIXME - pitch?

			// move x to right side of block
			s += (31*4);

			// highlight end of block? where x is now..
/*
			*s = 0; s++;
			*s = 255; s++;
			*s = 250; s++;
			*s = 255; s++;
			s-=4;
*/
			for (int x = 30; x >= 0; x--)
			{
				if ((s[0] >= 20) && (s[1] >= 20) && (s[2] >= 20))
				{
					// we've hit some white..
					if (x > width)
					{
						width = x;
						theWidth = s;
					}
/*
					// highlight the width
					*s = 21;  s++; // B
					*s = 255; s++; // G
					*s = 0;   s++; // R
					*s = 255; s++; // A
					s-=4;
*/
					break;
				}

				s-=4;
			}
		}

		// colour the position where max width was found
		if (theWidth) 
		{
/*
			// highlight the width
			*theWidth = 255;  theWidth++; // B
			*theWidth = 0; theWidth++; // G
			*theWidth = 255;   theWidth++; // R
			*theWidth = 255; theWidth++; // A
*/
		}

		// char fully scanned
		theWidths[i] = width+3;
	}

	Tex_Unlock(Fonts[FONT_SMALL].textureID);
 
//	D3DXSaveTextureToFile("c:/ATI/test.png", D3DXIFF_PNG, Tex_GetTexture(Fonts[FONT_SMALL].textureID), NULL);
/*
	char buf[100];

	for (int i = 0; i < 256; i++)
	{
		sprintf(buf, "char: %c = %d\n", (char)i+32, theWidths[i]);
		OutputDebugString(buf);
	}
*/
	theWidths[0] = 10;

	for (int i = 0; i < 256; i++)
	{
		Fonts[FONT_SMALL].charWidths[i] = theWidths[i];
	}

	Fonts[FONT_SMALL].cellHeight = 32;
	Fonts[FONT_SMALL].cellWidth = 32;
	Fonts[FONT_SMALL].mapHeight = 512;
	Fonts[FONT_SMALL].mapWidth = 512;
	Fonts[FONT_SMALL].startChar = ' ';

/*
	// write data file
	std::ofstream outfile;
	outfile.open("c:/ATI/test1.dat", std::ofstream::out | std::ofstream::binary);

	if (outfile.good())
	{
		unsigned val = 512;
		outfile.write(reinterpret_cast<char*>(&val), sizeof(unsigned));
		outfile.write(reinterpret_cast<char*>(&val), sizeof(unsigned));

		val = 32;

		outfile.write(reinterpret_cast<char*>(&val), sizeof(unsigned));
		outfile.write(reinterpret_cast<char*>(&val), sizeof(unsigned));

		char start = ' ';
		outfile.write(reinterpret_cast<char*>(&val), sizeof(char));

		outfile.write(reinterpret_cast<char*>(&theWidths), 256);
		outfile.close();
	}
*/

#if 0
	// see if there's a font description file
	std::ifstream infile;
	infile.open("aa_font_512.dat", std::ifstream::in | std::ifstream::binary);

	if (infile.good())
	{
		// find out the size of the file
		infile.seekg(0, std::ios::end);
		size_t fileLength = infile.tellg();
		infile.seekg(0, std::ios::beg);

		if (fileLength == sizeof(BFD))
		{
			infile.read(reinterpret_cast<char*>(&Fonts[FONT_SMALL].mapWidth),   4);
			infile.read(reinterpret_cast<char*>(&Fonts[FONT_SMALL].mapHeight),  4);
			infile.read(reinterpret_cast<char*>(&Fonts[FONT_SMALL].cellWidth),  4);
			infile.read(reinterpret_cast<char*>(&Fonts[FONT_SMALL].cellHeight), 4);
			infile.read(reinterpret_cast<char*>(&Fonts[FONT_SMALL].startChar),  1);
			infile.read(reinterpret_cast<char*>(&Fonts[FONT_SMALL].charWidths), 256);
		}
	}
#endif
	// get the font texture width and height
	Tex_GetDimensions(Fonts[FONT_SMALL].textureID, Fonts[FONT_SMALL].mapWidth, Fonts[FONT_SMALL].mapHeight);
}

uint32_t Font_GetCharWidth(char c)
{
	return Fonts[FONT_SMALL].charWidths[c];
}

uint32_t Font_GetCharHeight()
{
	return Fonts[FONT_SMALL].cellHeight;
}

uint32_t Font_GetStringWidth(const std::string &text)
{
	uint32_t width = 0;

	for (int i = 0; i < text.size(); i++)
	{
		width += Fonts[FONT_SMALL].charWidths[text[i]];
	}

	return width;
}

uint32_t Font_DrawCenteredText(const std::string &text)
{
	uint32_t textWidth = Font_GetStringWidth(text);

	int x = (640/2) - (textWidth/2);
	int y = 40;

	return Font_DrawText(text, x, y, RGB_MAKE(255, 255, 255), FONT_SMALL);
}

uint32_t Font_DrawText(const std::string &text, uint32_t x, uint32_t y, uint32_t colour, enum FONT_TYPE_2 fontType)
{
	float RecipW = (1.0f / Fonts[FONT_SMALL].mapWidth);
	float RecipH = (1.0f / Fonts[FONT_SMALL].mapHeight);

	uint32_t charIndex = 0;
	uint32_t charHeight = Font_GetCharHeight();

	while (charIndex < text.size())
	{
		// get the current char we're at in the string
		char c = text[charIndex] - 32;
		uint32_t charWidth = Fonts[FONT_SMALL].charWidths[c];

		uint32_t row = (uint32_t)(c / 16);  // get row
		uint32_t column = c % 16;           // get column from remainder value

		uint32_t tex_x = column * Fonts[FONT_SMALL].cellWidth;
		uint32_t tex_y = row * Fonts[FONT_SMALL].cellHeight;

		// generate the texture UVs for this character
		float uvArray[8];

		// bottom left
		uvArray[0] = RecipW * tex_x;
		uvArray[1] = RecipH * (tex_y + charHeight);

		// top left
		uvArray[2] = RecipW * tex_x;
		uvArray[3] = RecipH * tex_y;

		// bottom right
		uvArray[4] = RecipW * (tex_x + charWidth);
		uvArray[5] = RecipH * (tex_y + charHeight);

		// top right
		uvArray[6] = RecipW * (tex_x + charWidth);
		uvArray[7] = RecipH * tex_y;

		DrawFontQuad(x, y, charWidth, charHeight, Fonts[FONT_SMALL].textureID, uvArray, colour, TRANSLUCENCY_GLOWING);

		bool fixedWidth = false;

		if (fixedWidth)
		{
			x += Fonts[FONT_SMALL].cellWidth;
		}
		else
		{
			x += charWidth;
		}

		charIndex++;
	}

	return 0;
}

