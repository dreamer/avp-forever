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

#include "Fonts.h"
#include "TextureManager.h"
#include "FileStream.h"
#include "console.h"

const int kFontDATsize = 273; // size in bytes of a font .dat file
const int kNumChars = 256;    // number of ASCII characters

struct Font
{
	bool isValid;
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
		Fonts[i].isValid = false;
	}
}

bool Font_Load(const std::string &fontFile, enum eFontTypes fontType)
{
	// initially set to false
	Fonts[fontType].isValid = false;

	Fonts[fontType].textureID = Tex_CreateFromFile(fontFile + ".tga");

	if (Fonts[fontType].textureID == MISSING_TEXTURE)
	{
		Con_PrintError("Couldn't load font texture file '" + fontFile + ".tga'");
		return false;
	}

#if 0 // calculates the width of the chars when we dont have a .dat file

	uint8_t *srcPtr = NULL;
	uint32_t pitch = 0;

	// calculate widths
	if (!Tex_Lock(Fonts[fontType].textureID, &srcPtr, &pitch))
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

	Tex_Unlock(Fonts[fontType].textureID);

//	D3DXSaveTextureToFile("c:/ATI/test.png", D3DXIFF_PNG, Tex_GetTexture(Fonts[fontType].textureID), NULL);
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
		Fonts[fontType].charWidths[i] = theWidths[i];
	}

	Fonts[fontType].cellHeight = 32;
	Fonts[fontType].cellWidth  = 32;
	Fonts[fontType].mapHeight  = 512;
	Fonts[fontType].mapWidth   = 512;
	Fonts[fontType].startChar  = ' ';
#endif
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

#if 1

	memset(Fonts[fontType].charWidths, 0, kNumChars);
	
	// see if there's a font description file
	FileStream file;
	file.Open(fontFile + ".dat", FileStream::FileRead);

	if (!file.IsGood())
	{
		Con_PrintError("Can't open font data file '" + fontFile + ".dat'");
		Tex_Release(Fonts[fontType].textureID);
		return false;
	}

	size_t fileLength = file.GetFileSize();

	if (kFontDATsize == fileLength)
	{
		Fonts[fontType].mapWidth   = file.GetUint32LE();
		Fonts[fontType].mapHeight  = file.GetUint32LE();
		Fonts[fontType].cellWidth  = file.GetUint32LE();
		Fonts[fontType].cellHeight = file.GetUint32LE();
		Fonts[fontType].startChar  = file.GetByte();
		for (int i = 0; i < kNumChars; i++)
		{
			Fonts[fontType].charWidths[i] = file.GetByte();
		}
	}
#endif

	// get the font texture width and height
	Tex_GetDimensions(Fonts[fontType].textureID, Fonts[fontType].mapWidth, Fonts[fontType].mapHeight);

	Fonts[fontType].isValid = true;

	return true;
}

uint32_t Font_GetCharWidth(char c, enum eFontTypes fontType)
{
	return Fonts[fontType].charWidths[c];
}

uint32_t Font_GetCharHeight(enum eFontTypes fontType)
{
	return Fonts[fontType].cellHeight;
}

uint32_t Font_GetStringWidth(const std::string &text, enum eFontTypes fontType)
{	
	if (!Fonts[fontType].isValid)
		return 0;

	uint32_t width = 0;

	for (size_t i = 0; i < text.size(); i++)
	{
		width += Fonts[fontType].charWidths[text[i]];
	}

	return width;
}

uint32_t Font_DrawCenteredText(const std::string &text, enum eFontTypes fontType)
{
	if (!Fonts[fontType].isValid)
		return 0;

	uint32_t textWidth = Font_GetStringWidth(text, fontType);

	int x = (R_GetScreenWidth()/2) - (textWidth/2);
	int y = 40;

	return Font_DrawText(text, x, y, RGB_MAKE(255, 255, 255), fontType);
}

uint32_t Font_DrawText(const std::string &text, uint32_t x, uint32_t y, RCOLOR colour, enum eFontTypes fontType)
{
	if (!Fonts[fontType].isValid)
		return 0;

	float RecipW = (1.0f / Fonts[fontType].mapWidth);
	float RecipH = (1.0f / Fonts[fontType].mapHeight);

	uint32_t currentChar = 0;
	uint32_t charHeight = Font_GetCharHeight(fontType);

	while (currentChar < text.size())
	{
		// get the current char we're at in the string
		char c = text[currentChar] - Fonts[fontType].startChar;
		uint32_t charWidth = Fonts[fontType].charWidths[c];

		uint32_t nRowChars    = Fonts[fontType].mapWidth  / Fonts[fontType].cellWidth;
		uint32_t nColumnChars = Fonts[fontType].mapHeight / Fonts[fontType].cellHeight;

		uint32_t row = (uint32_t)(c / nRowChars);  // get row
		uint32_t column = c % nColumnChars;        // get column from remainder value

		uint32_t tex_x = column * Fonts[fontType].cellWidth;
		uint32_t tex_y = row * Fonts[fontType].cellHeight;

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

		DrawFontQuad(x, y, charWidth, charHeight, Fonts[fontType].textureID, uvArray, colour, TRANSLUCENCY_NORMAL);

		bool fixedWidth = false;

		if (fixedWidth)
		{
			x += Fonts[fontType].cellWidth;
		}
		else
		{
			x += charWidth;
		}

		currentChar++;
	}

	return 0;
}
