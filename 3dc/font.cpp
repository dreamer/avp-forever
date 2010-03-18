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

#include "d3_func.h"
#ifdef WIN32
#include <d3dx9.h>
#endif
#include "logString.h"
#include "stdint.h"
#include "font2.h"

static HRESULT LastError;

struct Font
{
	enum type;
	int textureWidth;
	int textureHeight;
	D3DTEXTURE texture;
	D3DXIMAGE_INFO imageInfo;
	int fontWidths[256];
	int blockWidth;
	int blockHeight;
};

static Font Fonts[NUM_FONT_TYPES];

void Font_Release()
{
	for (int i = 0; i < NUM_FONT_TYPES; i++)
	{
		if (Fonts[i].texture)
		{
			Fonts[i].texture->Release();
			Fonts[i].texture = NULL;
		}
	}
}

void Font_Init()
{
	// load the small font texture
	if (Fonts[FONT_SMALL].texture)
	{
		Fonts[FONT_SMALL].texture->Release();
		Fonts[FONT_SMALL].texture = NULL;
	}

	LastError = D3DXCreateTextureFromFileEx
	(
		d3d.lpD3DDevice, 
		"aa_font_grid_test.png", 
		D3DX_DEFAULT,			// width
		D3DX_DEFAULT,			// height
		1,						// mip levels
		0,						// usage	
		D3DFMT_UNKNOWN,			// format
		D3DPOOL_MANAGED,
		D3DX_FILTER_NONE,
		D3DX_FILTER_NONE,
		0,
		&Fonts[FONT_SMALL].imageInfo,
		NULL,
		&Fonts[FONT_SMALL].texture
	);
	
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return;
	}
	
	// get the font widths
	D3DLOCKED_RECT lock;
	uint8_t *srcPtr = NULL;
	int c;
	Fonts[FONT_SMALL].blockWidth = Fonts[FONT_SMALL].imageInfo.Width / 16;
	Fonts[FONT_SMALL].blockHeight = Fonts[FONT_SMALL].imageInfo.Height / 16;
	
	LastError = Fonts[FONT_SMALL].texture->LockRect(0, &lock, NULL, NULL );
	if (FAILED(LastError)) 
	{
		Fonts[FONT_SMALL].texture->Release();
		LogDxError(LastError, __LINE__, __FILE__);
		return;
	}

	srcPtr = static_cast<uint8_t*> (lock.pBits);

	Fonts[FONT_SMALL].fontWidths[32] = 12; // size of space character

	for (c = 33; c < 255; c++) 
	{
		int x, y;

//		int x1 = 1+((c-32)&15) * Fonts[FONT_SMALL].blockWidth;
//		int y1 = 1+((c-32)>>4) * Fonts[FONT_SMALL].blockHeight;

		int row = (int)((c-32) / 16);
		int column = (c-32) % 16;

		int x2 = column * Fonts[FONT_SMALL].blockWidth;
		int y2 = row * Fonts[FONT_SMALL].blockHeight;

		Fonts[FONT_SMALL].fontWidths[c] = Fonts[FONT_SMALL].blockWidth + 1;

		for (x = x2 + Fonts[FONT_SMALL].blockWidth; x > x2; x--)
		{
			int blank = 1;

			for (y = y2; y < y2 + Fonts[FONT_SMALL].blockHeight; y++)
			{
				uint8_t *s = &srcPtr[((x * 4) + y * lock.Pitch)];

				if ((s[2] >= 0x80))// && (s[1] >= 240) && (s[2] >= 240))
				{
					blank = 0;
					break;
				}
			}

			if (blank)
			{
				Fonts[FONT_SMALL].fontWidths[c]--;
			}
			else
			{
				break;
			}
		}
	}

	LastError = Fonts[FONT_SMALL].texture->UnlockRect(0);
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return;
	}
/*
	char buf[100];
	for (int i = 0; i < 256; i++)
	{
		sprintf(buf, "Fonts[FONT_SMALL].fontWidths[%d] == %d\n", i, Fonts[FONT_SMALL].fontWidths[i]);
		OutputDebugString(buf);
	}
*/
}

extern "C" {
extern char AAFontWidths[256];
}

int Font_DrawText(const char* text, int x, int y, int colour, int fontType)
{
	d3d.lpD3DDevice->SetTexture(0, Fonts[FONT_SMALL].texture);

//	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
//	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);

	D3DTLVERTEX fontQuad[4];

	float RecipW = (1.0f / Fonts[FONT_SMALL].imageInfo.Width);
	float RecipH = (1.0f / Fonts[FONT_SMALL].imageInfo.Height);

	int sixtyThree = 32;

	while (*text)
	{
		char c = *text++;

//		int charWidth = Fonts[FONT_SMALL].fontWidths[c];
		int charWidth = AAFontWidths[c] * 2;

		c = c - 32;

		int row = (int)(c / 16); // get row 
		int column = c % 16;	 // get column from remainder value

		int tex_x = column * Fonts[FONT_SMALL].blockWidth;
		int tex_y = row * Fonts[FONT_SMALL].blockHeight;

		// bottom left
		fontQuad[0].sx = x - 0.5f;
		fontQuad[0].sy = y + sixtyThree - 0.5f;
		fontQuad[0].sz = 0.0f;
		fontQuad[0].rhw = 1.0f;
		fontQuad[0].color = colour;
		fontQuad[0].specular = D3DCOLOR_ARGB(255, 0, 0, 0);
		fontQuad[0].tu = (float)((tex_x) * RecipW);
		fontQuad[0].tv = (float)((tex_y + Fonts[FONT_SMALL].blockWidth) * RecipH);

		// top left
		fontQuad[1].sx = x - 0.5f;
		fontQuad[1].sy = y - 0.5f;
		fontQuad[1].sz = 0.0f;
		fontQuad[1].rhw = 1.0f;
		fontQuad[1].color = colour;
		fontQuad[1].specular = D3DCOLOR_ARGB(255, 0, 0, 0);
		fontQuad[1].tu = (float)((tex_x) * RecipW);
		fontQuad[1].tv = (float)((tex_y) * RecipH);

		// bottom right
		fontQuad[2].sx = x + sixtyThree - 0.5f;
		fontQuad[2].sy = y + sixtyThree - 0.5f;
		fontQuad[2].sz = 0.0f;
		fontQuad[2].rhw = 1.0f;
		fontQuad[2].color = colour;
		fontQuad[2].specular = D3DCOLOR_ARGB(255, 0, 0, 0);
		fontQuad[2].tu = (float)((tex_x + Fonts[FONT_SMALL].blockWidth) * RecipW);
		fontQuad[2].tv = (float)((tex_y + Fonts[FONT_SMALL].blockHeight) * RecipH);

		// top right
		fontQuad[3].sx = x + sixtyThree - 0.5f;
		fontQuad[3].sy = y - 0.5f;
		fontQuad[3].sz = 0.0f;
		fontQuad[3].rhw = 1.0f;
		fontQuad[3].color = colour;
		fontQuad[3].specular = D3DCOLOR_ARGB(255, 0, 0, 0);
		fontQuad[3].tu = (float)((tex_x + Fonts[FONT_SMALL].blockWidth) * RecipW);
		fontQuad[3].tv = (float)((tex_y) * RecipH);

#ifdef _XBOX
		d3d.lpD3DDevice->SetVertexShader(D3DFVF_TLVERTEX);
#else
		d3d.lpD3DDevice->SetFVF (D3DFVF_TLVERTEX);
#endif

		LastError = d3d.lpD3DDevice->DrawPrimitiveUP (D3DPT_TRIANGLESTRIP, 2, fontQuad, sizeof(D3DTLVERTEX));
		if (FAILED(LastError)) 
		{
			OutputDebugString("Font_DrawText() failed on DrawPrimitiveUP\n");
		}

		if (/*widthSpaced*/1)
		{
			x += charWidth;
		}
		else
		{
			x += Fonts[FONT_SMALL].blockWidth;
		}
	}

	return 0;
}

int Font_GetTextLength(const char* text)
{
	int width = 0;

	while (text && *text) 
	{
		width += Fonts[FONT_SMALL].fontWidths[*text];
		text++;
	}

	return width;
}