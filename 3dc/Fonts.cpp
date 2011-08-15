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

#include "logString.h"
#include "Fonts.h"
#include "TextureManager.h"
#include <iostream>
#include <fstream>

const int kFontDATsize = 273; // size in bytes of a font .dat file
const int kNumChars = 256;    // number of ASCII characters

struct Font
{
	enum eFontTypes  type;
	texID_t  textureID;
	uint32_t mapWidth;
	uint32_t mapHeight;
	uint32_t cellWidth;
	uint32_t cellHeight;
	char     startChar;
	char     charWidths[kNumChars];
};

static Font Fonts[kNumFontTypes];

void Font_Release()
{
	for (uint32_t i = 0; i < kNumFontTypes; i++)
	{
		Tex_Release(Fonts[i].textureID);
	}
}

bool Font_Init()
{
	Fonts[kFontSmall].textureID = Tex_CreateFromFile("aa_font_512.png");

	uint8_t *srcPtr = NULL;
	uint32_t pitch = 0;

	// calculate widths
	if (!Tex_Lock(Fonts[kFontSmall].textureID, &srcPtr, &pitch))
	{
		return false;
	}

	uint8_t theWidths[kNumChars];

	uint8_t *s = NULL;

	uint8_t *theWidth = 0;

	uint32_t width = 0;

	for (int i = 0; i < kNumChars; i++)
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

	Tex_Unlock(Fonts[kFontSmall].textureID);

//	D3DXSaveTextureToFile("c:/ATI/test.png", D3DXIFF_PNG, Tex_GetTexture(Fonts[kFontSmall].textureID), NULL);
/*
	char buf[100];

	for (int i = 0; i < kNumChars; i++)
	{
		sprintf(buf, "char: %c = %d\n", (char)i+32, theWidths[i]);
		OutputDebugString(buf);
	}
*/
	theWidths[0] = 10;

	for (int i = 0; i < kNumChars; i++)
	{
		Fonts[kFontSmall].charWidths[i] = theWidths[i];
	}

	Fonts[kFontSmall].cellHeight = 32;
	Fonts[kFontSmall].cellWidth  = 32;
	Fonts[kFontSmall].mapHeight  = 512;
	Fonts[kFontSmall].mapWidth   = 512;
	Fonts[kFontSmall].startChar  = ' ';

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

		outfile.write(reinterpret_cast<char*>(&theWidths), kNumChars);
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

		if (fileLength == kFontDATsize)
		{
			infile.read(reinterpret_cast<char*>(&Fonts[kFontSmall].mapWidth),   4);
			infile.read(reinterpret_cast<char*>(&Fonts[kFontSmall].mapHeight),  4);
			infile.read(reinterpret_cast<char*>(&Fonts[kFontSmall].cellWidth),  4);
			infile.read(reinterpret_cast<char*>(&Fonts[kFontSmall].cellHeight), 4);
			infile.read(reinterpret_cast<char*>(&Fonts[kFontSmall].startChar),  1);
			infile.read(reinterpret_cast<char*>(&Fonts[kFontSmall].charWidths), kNumChars);
		}
	}
#endif

	// get the font texture width and height
	Tex_GetDimensions(Fonts[kFontSmall].textureID, Fonts[kFontSmall].mapWidth, Fonts[kFontSmall].mapHeight);

	return true;
}

uint32_t Font_GetCharWidth(char c)
{
	return Fonts[kFontSmall].charWidths[c];
}

uint32_t Font_GetCharHeight()
{
	return Fonts[kFontSmall].cellHeight;
}

uint32_t Font_GetStringWidth(const std::string &text)
{
	uint32_t width = 0;

	for (size_t i = 0; i < text.size(); i++)
	{
		width += Fonts[kFontSmall].charWidths[text[i]];
	}

	return width;
}

uint32_t Font_DrawCenteredText(const std::string &text)
{
	uint32_t textWidth = Font_GetStringWidth(text);

	int x = (R_GetScreenWidth()/2) - (textWidth/2);
	int y = 40;

	return Font_DrawText(text, x, y, RGB_MAKE(255, 255, 255), kFontSmall);
}

uint32_t Font_DrawText(const std::string &text, uint32_t x, uint32_t y, RCOLOR colour, enum eFontTypes fontType)
{
	float RecipW = (1.0f / Fonts[kFontSmall].mapWidth);
	float RecipH = (1.0f / Fonts[kFontSmall].mapHeight);

	uint32_t charIndex = 0;
	uint32_t charHeight = Font_GetCharHeight();

	while (charIndex < text.size())
	{
		// get the current char we're at in the string
		char c = text[charIndex] - 32;
		uint32_t charWidth = Fonts[kFontSmall].charWidths[c];

		uint32_t row = (uint32_t)(c / 16);  // get row
		uint32_t column = c % 16;           // get column from remainder value

		uint32_t tex_x = column * Fonts[kFontSmall].cellWidth;
		uint32_t tex_y = row * Fonts[kFontSmall].cellHeight;

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

		DrawFontQuad(x, y, charWidth, charHeight, Fonts[kFontSmall].textureID, uvArray, colour, TRANSLUCENCY_NORMAL);

		bool fixedWidth = false;

		if (fixedWidth)
		{
			x += Fonts[kFontSmall].cellWidth;
		}
		else
		{
			x += charWidth;
		}

		charIndex++;
	}

	return 0;
}
