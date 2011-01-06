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
	uint32_t	textureWidth;
	uint32_t	textureHeight;
	texID_t	textureID;
	uint32_t	fontWidths[256];
	uint32_t	blockWidth;
	uint32_t	blockHeight;
	BFD			desc;
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
	Fonts[FONT_SMALL].textureID = Tex_CreateFromFile("test_font.tga");

	// zero out all values in the description struct
	memset(&Fonts[FONT_SMALL].desc, 0, sizeof(BFD));

	// see if there's a font description file
	std::ifstream infile;
	infile.open("test_font.dat", std::ifstream::in | std::ifstream::binary);

	if (infile.good())
	{
		// find out the size of the file
		infile.seekg(0, std::ios::end);
		size_t fileLength = infile.tellg();
		infile.seekg(0, std::ios::beg);

		if (fileLength == sizeof(BFD))
		{
			// read in the data
			infile.read(reinterpret_cast<char*>(&Fonts[FONT_SMALL].desc), sizeof(BFD));
		}
	}

	// get the font texture width and height
	Tex_GetDimensions(Fonts[FONT_SMALL].textureID, Fonts[FONT_SMALL].textureWidth, Fonts[FONT_SMALL].textureHeight);

	// work out how big each character cell/block is
	Fonts[FONT_SMALL].blockWidth = Fonts[FONT_SMALL].textureWidth / 16;
	Fonts[FONT_SMALL].blockHeight = Fonts[FONT_SMALL].textureHeight / 16;
}

uint32_t Font_DrawText(const std::string &text, uint32_t x, uint32_t y, uint32_t colour, enum FONT_TYPE fontType)
{
	float RecipW = (1.0f / Fonts[FONT_SMALL].textureWidth);
	float RecipH = (1.0f / Fonts[FONT_SMALL].textureHeight);

	uint32_t charIndex = 0;
	uint32_t charHeight = 16;

	while (charIndex < text.size())
	{
		// get the current char we're at in the string
		char c = text[charIndex] - 32;

		uint32_t charWidth = Fonts[FONT_SMALL].desc.charWidths[c];

		uint32_t row = (uint32_t)(c / 16);  // get row
		uint32_t column = c % 16;			// get column from remainder value

		uint32_t tex_x = column * Fonts[FONT_SMALL].blockWidth;
		uint32_t tex_y = row * Fonts[FONT_SMALL].blockHeight;

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

		if (/*widthSpaced*/1)
		{
			x += charWidth;
		}
		else
		{
			x += Fonts[FONT_SMALL].blockWidth;
		}

		charIndex++;
	}

	return 0;
}

