#include "3dc.h"
#include "inline.h"
#include "tallfont.hpp"
#include "strtab.hpp"
#include "awTexLd.h"
#include "chnktexi.h"
#include "hud_layout.h"
#define UseLocalAssert TRUE
#include "ourasert.h"
#include "ffstdio.h"
#include "renderer.h"
#include "console.h"
#include "TextureManager.h"
#include "Fonts.h"
#include "AvP_Menus.h"
#include <assert.h>
#include "RimLoader.h"

void D3D_Rectangle(int x0, int y0, int x1, int y1, int r, int g, int b, int a);
extern void DrawMenuTextGlow(uint32_t topLeftX, uint32_t topLeftY, uint32_t size, uint32_t alpha);

extern void D3D_RenderHUDString(char *stringPtr,int x,int y,int colour);
static void LoadMenuFont(void);
static void UnloadMenuFont(void);
extern int RenderSmallFontString(char *textPtr,int sx,int sy,int alpha, int red, int green, int blue);
static void CalculateWidthsOfAAFont(void);

texID_t AVPMENUGFX_CLOUDY;
texID_t AVPMENUGFX_SMALL_FONT;
texID_t AVPMENUGFX_COPYRIGHT_SCREEN;

texID_t AVPMENUGFX_PRESENTS;
texID_t AVPMENUGFX_AREBELLIONGAME;
texID_t AVPMENUGFX_ALIENSVPREDATOR;

texID_t AVPMENUGFX_SLIDERBAR;
texID_t AVPMENUGFX_SLIDER;

texID_t AVPMENUGFX_BACKDROP;
texID_t AVPMENUGFX_ALIENS_LOGO;
texID_t AVPMENUGFX_ALIEN_LOGO;
texID_t AVPMENUGFX_MARINE_LOGO;
texID_t AVPMENUGFX_PREDATOR_LOGO;

texID_t AVPMENUGFX_GLOWY_LEFT;
texID_t AVPMENUGFX_GLOWY_MIDDLE;
texID_t AVPMENUGFX_GLOWY_RIGHT;

texID_t AVPMENUGFX_MARINE_EPISODE1;
texID_t AVPMENUGFX_MARINE_EPISODE2;
texID_t AVPMENUGFX_MARINE_EPISODE3;
texID_t AVPMENUGFX_MARINE_EPISODE4;
texID_t AVPMENUGFX_MARINE_EPISODE5;
texID_t AVPMENUGFX_MARINE_EPISODE6;

texID_t AVPMENUGFX_MARINE_EPISODE7;
texID_t AVPMENUGFX_MARINE_EPISODE8;
texID_t AVPMENUGFX_MARINE_EPISODE9;
texID_t AVPMENUGFX_MARINE_EPISODE10;
texID_t AVPMENUGFX_MARINE_EPISODE11;

texID_t AVPMENUGFX_PREDATOR_EPISODE1;
texID_t AVPMENUGFX_PREDATOR_EPISODE2;
texID_t AVPMENUGFX_PREDATOR_EPISODE3;
texID_t AVPMENUGFX_PREDATOR_EPISODE4;
texID_t AVPMENUGFX_PREDATOR_EPISODE5;
texID_t AVPMENUGFX_PREDATOR_EPISODE6;

texID_t AVPMENUGFX_PREDATOR_EPISODE7;
texID_t AVPMENUGFX_PREDATOR_EPISODE8;
texID_t AVPMENUGFX_PREDATOR_EPISODE9;
texID_t AVPMENUGFX_PREDATOR_EPISODE10;
texID_t AVPMENUGFX_PREDATOR_EPISODE11;

texID_t AVPMENUGFX_ALIEN_EPISODE1;
texID_t AVPMENUGFX_ALIEN_EPISODE2;
texID_t AVPMENUGFX_ALIEN_EPISODE3;
texID_t AVPMENUGFX_ALIEN_EPISODE4;
texID_t AVPMENUGFX_ALIEN_EPISODE5;
texID_t AVPMENUGFX_ALIEN_EPISODE6;
texID_t AVPMENUGFX_ALIEN_EPISODE7;
texID_t AVPMENUGFX_ALIEN_EPISODE8;
texID_t AVPMENUGFX_ALIEN_EPISODE9;
texID_t AVPMENUGFX_ALIEN_EPISODE10;

texID_t AVPMENUGFX_WINNER_SCREEN;

texID_t AVPMENUGFX_SPLASH_SCREEN1;
texID_t AVPMENUGFX_SPLASH_SCREEN2;
texID_t AVPMENUGFX_SPLASH_SCREEN3;
texID_t AVPMENUGFX_SPLASH_SCREEN4;
texID_t AVPMENUGFX_SPLASH_SCREEN5;

extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern AVPMENU_ELEMENT AvPMenu_SinglePlayer[];

char AAFontWidths[256];

AVPIndexedFont IntroFont_Light;

static void LoadMenuFont(void)
{
	/* notes on converting to quad textured menu system:

		This function loads a font image with height 8192. We'll want to reorder this to a nice
		compatible square 512 x 512 texture

		224 characters, each 33 pixels high?
		30 pixels wide

		16 x 16 grid?

		15 x 15 = 225, so better

		height:	495
		width : 450

		will fit nicely into a 512 x 512 d3d texture
	*/

	IntroFont_Light.height = 33;
	IntroFont_Light.swidth = 5;
	IntroFont_Light.ascii  = 32;

	RimLoader introFontRIM;
	if (!introFontRIM.Open("graphics/Menus/IntroFont.RIM"))
	{
		Con_PrintError("Can't find Menus/IntroFont.RIM");
		return;
	}

	uint32_t width, height;

	introFontRIM.GetDimensions(width, height);

	// allocate buffer to decode into
	uint8_t *srcBuffer = new uint8_t[width*height*4];
	introFontRIM.Decode(srcBuffer, width*4);

	uint8_t *srcPtr = srcBuffer;

	IntroFont_Light.numchars = height / 33;
	IntroFont_Light.FontWidth[32] = 5;

	for (int c = 33; c < (32+IntroFont_Light.numchars); c++)
	{
		int y1 = 1+(c-32)*33;

		IntroFont_Light.FontWidth[c] = 31;

		for (int x = 29; x > 0; x--)
		{
			bool blank = true;

			for (int y = y1; y < y1+31; y++)
			{
				uint8_t *s = &srcPtr[(x + y * width) * sizeof(uint32_t)];
				if (s[2])
				{
					blank = false;
					break;
				}
			}

			if (blank) {
				IntroFont_Light.FontWidth[c]--;
			}
			else {
				break;
			}
		}
	}

	AVPTEXTURE image;
	image.buffer = srcBuffer;
	image.width  = width;
	image.height = height;

	// we're going to try create a square texture
	IntroFont_Light.textureID = Tex_CreateTallFontTexture("graphics/Menus/IntroFont.RIM", image, TextureUsage_Normal);
}

static void UnloadMenuFont(void)
{
	Tex_Release(IntroFont_Light.textureID);
}

extern int LengthOfMenuText(const char *textPtr)
{
	int width = 0;
	
	while (textPtr && *textPtr) 
	{
		width += IntroFont_Light.FontWidth[(unsigned int) *textPtr];
		
		textPtr++;
	}
	return width;
}

extern int RenderMenuText(char *textPtr, int pX, int pY, int alpha, enum AVPMENUFORMAT_ID format) 
{
	int width = LengthOfMenuText(textPtr);
	int word_length = 0;

	switch (format)
	{
		default:
		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			pX -= width;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			pX -= width / 2;
			break;
		}
	}

	if (GetAvPMenuState() == MENUSSTATE_INGAMEMENUS)
	{
		// do nothing here for now
	}
	else
	{
		if (alpha > BRIGHTNESS_OF_DARKENED_ELEMENT)
		{
			int size = width - 18;
			if (size < 18) 
				size = 18;

			DrawMenuTextGlow(pX+18, pY-8, size-18, alpha);
		}
	}

	// we need initial values for cloud effect
	int positionX = pX;
	int positionY = pY;

	while (*textPtr)
	{
		char c = *textPtr++;

		if (c >= ' ')
		{
			uint32_t charWidth = IntroFont_Light.FontWidth[(unsigned int) c];

			c = c - 32;

			int row = (int)(c / 15); // get row 
			int column = c % 15;     // get column from remainder value

			int tex_y = row    * 33;
			int tex_x = column * 30;

			int topLeftU = tex_x;
			int topLeftV = tex_y;

			DrawTallFontCharacter(positionX, positionY, IntroFont_Light.textureID, topLeftU, topLeftV, charWidth, alpha);

			positionX   += charWidth;
			word_length += charWidth;
		}
	}

	return positionX;
}

extern int RenderMenuText_Clipped(char *textPtr, int pX, int pY, int alpha, enum AVPMENUFORMAT_ID format, int topY, int bottomY) 
{
	RenderMenuText(textPtr, pX, pY, alpha, format); 

	return pX;
}


extern int RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format) 
{
	int length;
	char *ptr;

	switch(format)
	{
		default:
		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			length = 0;
			ptr = textPtr;

			while(*ptr)
			{
				//length+=AAFontWidths[*ptr++];
				length += AAFontWidths[(unsigned int) *ptr++];
			}

			x -= length;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			length = 0;
			ptr = textPtr;

			while(*ptr)
			{
				//length+=AAFontWidths[*ptr++];
				length += AAFontWidths[(unsigned int) *ptr++];
			}

			x -= length/2;
			break;
		}
	}

	LOCALASSERT(x>0);

	x = RenderSmallFontString(textPtr, x, y, alpha, ONE_FIXED, ONE_FIXED, ONE_FIXED);
	return x;
}

extern int RenderSmallMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue) 
{
	switch(format)
	{
		default:
		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			while(*ptr)
			{
				length+=AAFontWidths[*ptr++];
			}

			x -= length;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			while(*ptr)
			{
				length+=AAFontWidths[*ptr++];
			}

			x -= length/2;
			break;
		}
	}

	LOCALASSERT(x>0);

	x = RenderSmallFontString(textPtr, x, y, alpha, red, green, blue);

	return x;
}

extern int Hardware_RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format) 
{
	switch(format)
	{
		default:
		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			while (*ptr)
			{
				length += AAFontWidths[*ptr++];
			}

			x -= length;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			while (*ptr)
			{
				length += AAFontWidths[*ptr++];
			}

			x -= length/2;
			break;
		}
	}

	LOCALASSERT(x>0);

	{
		unsigned int colour = alpha>>8;
		if (colour>255) colour = 255;
		colour = (colour<<24)+0xffffff;

		D3D_RenderHUDString(textPtr,x,y,colour);
	}
	return x;
}

extern int Hardware_RenderSmallMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue)
{
	switch(format)
	{
		default:
		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			while (*ptr)
			{
				length+=AAFontWidths[*ptr++];
			}

			x -= length;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			int length = 0;
			char *ptr = textPtr;

			while (*ptr)
			{
				length+=AAFontWidths[*ptr++];
			}

			x -= length/2;
			break;
		}
	}

	LOCALASSERT(x>0);

	{
		unsigned int colour = alpha>>8;
		if (colour>255) colour = 255;
		colour = (colour<<24);
		colour += MUL_FIXED(red,255)<<16;
		colour += MUL_FIXED(green,255)<<8;
		colour += MUL_FIXED(blue,255);
		D3D_RenderHUDString(textPtr,x,y,colour);
	}
	return x;
}

extern void RenderKeyConfigRectangle(int alpha)
{
	uint32_t colour = alpha>>8;

	if (colour > 255)
		colour = 255;

	colour = (colour<<24)+0xffffff;

	int segHeight = 2;
	int segWidth = 2;

	int totalWidth = 620;
	int totalHeight = 250;

	int x = 10;
	int y = 150;

	// top horizonal segment
	DrawQuad(x, y, totalWidth, segHeight, NO_TEXTURE, colour, TRANSLUCENCY_NORMAL);

	// draw left segment
	DrawQuad(x, y + segHeight, segWidth, (totalHeight-segHeight*2), NO_TEXTURE, colour, TRANSLUCENCY_NORMAL);

	// right segment
	DrawQuad(x + totalWidth - segWidth, y + segHeight, segWidth, (totalHeight-segHeight*2), NO_TEXTURE, colour, TRANSLUCENCY_NORMAL);

	// bottom segment
	DrawQuad(x, y + totalHeight-segHeight, totalWidth, segHeight, NO_TEXTURE, colour, TRANSLUCENCY_NORMAL);
}

extern void RenderHighlightRectangle(int x1, int y1, int x2, int y2, int r, int g, int b)
{
	D3D_Rectangle(x1, y1, x2, y2, r, g, b, 255);
}

extern int RenderSmallChar(char c, int x, int y, int alpha, int red, int green, int blue)
{
	int alphaR = MUL_FIXED(alpha, red);
	int alphaG = MUL_FIXED(alpha, green);
	int alphaB = MUL_FIXED(alpha, blue);

	int topLeftU = 1+((c-32)&15)*16;
	int topLeftV = 1+((c-32)>>4)*16;

	DrawSmallMenuCharacter(x, y, topLeftU, topLeftV, alphaR, alphaG, alphaB, alphaR);

	return AAFontWidths[(unsigned int) c];
}

/*static*/extern int RenderSmallFontString(char *textPtr, int sx, int sy, int alpha, int red, int green, int blue)
{
	int alphaR = MUL_FIXED(alpha, red);
	int alphaG = MUL_FIXED(alpha, green);
	int alphaB = MUL_FIXED(alpha, blue);

	while (*textPtr)
	{
		char c = *textPtr++;

		if (c >= ' ')
		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			DrawSmallMenuCharacter(sx, sy, topLeftU, topLeftV, alphaR, alphaG, alphaB, alphaR);

			sx += AAFontWidths[(unsigned int) c];
		}
	}

	return sx;
}

extern void RenderSmallFontString_Wrapped(char *textPtr, RECT* area, int alpha, int* output_x, int* output_y)
{
	// text on menus in bottom black space
	int wordWidth;
	int sx = area->left;
	int sy = area->top;

/*
Determine area used by text , so we can draw it centrally
*/
	const char *textPtr2 = textPtr;
	while (*textPtr2) 
	{
		int widthFromSpaces = 0;
		int widthFromChars = 0;
		
		while (*textPtr2 && *textPtr2==' ') 
		{
			widthFromSpaces += AAFontWidths[(unsigned int) *textPtr2++];
		}
		
		while (*textPtr2 && *textPtr2!=' ') 
		{
			widthFromChars += AAFontWidths[(unsigned int) *textPtr2++];
		}
		
		wordWidth=widthFromSpaces+widthFromChars;
		
		if (wordWidth > area->right - sx) 
		{
			if (wordWidth > area->right - area->left) 
			{
				int extraLinesNeeded = 0;
				
				wordWidth -= (area->right - sx);
				
				sy += HUD_FONT_HEIGHT;
				sx = area->left;
				
				extraLinesNeeded = wordWidth / (area->right - area->left);
				
				sy += HUD_FONT_HEIGHT * extraLinesNeeded;
				wordWidth %= (area->right - area->left);
				
				if (sy + HUD_FONT_HEIGHT > area->bottom) 
					break;
			}
			else 
			{
				sy += HUD_FONT_HEIGHT;
				sx = area->left;
				
				if (sy + HUD_FONT_HEIGHT > area->bottom) 
					break;
				
				if (wordWidth > area->right - sx) 
					break;
				
				wordWidth -= widthFromSpaces;
			}
		}
		sx += wordWidth;
	}
	
	if (sy == area->top) 
	{
		sx = area->left + (area->right-sx)/2;
	} 
	else 
	{
		sx = area->left;
	}
	
	sy += HUD_FONT_HEIGHT;
	if (sy < area->bottom) 
	{
		sy = area->top + (area->bottom - sy) / 2;
	} 
	else 
	{
		sy = area->top;
	}

	while (*textPtr) 
	{
		const char* textPtr2 = textPtr;
		wordWidth = 0;
		
		while (*textPtr2 && *textPtr2==' ') {
			wordWidth += AAFontWidths[(unsigned int) *textPtr2++];
		}
		
		while (*textPtr2 && *textPtr2!=' ') {
			wordWidth += AAFontWidths[(unsigned int) *textPtr2++];
		}
		
		if (wordWidth > area->right-sx) 
		{
			if (wordWidth>area->right - area->left) 
			{
				/* 
				  word is too long too fit on one line 
				  so we'll just have to allow it to be split
				 */
			} 
			else 
			{
				sy += HUD_FONT_HEIGHT;
				sx = area->left;
				
				if (sy + HUD_FONT_HEIGHT > area->bottom) break;
				
				if (wordWidth > area->right - sx) 
					break;
				
				while (*textPtr && *textPtr==' ') {
					textPtr++;
				}
			}
		}

		while (*textPtr && *textPtr==' ') 
		{
			sx += AAFontWidths[(unsigned int) *textPtr++];
		}
		
		if (sx > area->right) 
		{
			while (sx > area->right) 
			{
				sx -= (area->right - area->left);
				sy += HUD_FONT_HEIGHT;
			}
			
			if (sy + HUD_FONT_HEIGHT> area->bottom) 
				break;
		}
		
		while (*textPtr && *textPtr != ' ') 
		{
			char c = *textPtr++;
			int letterWidth = AAFontWidths[(unsigned int) c];
			
			if (sx + letterWidth > area->right) 
			{
				sx = area->left;
				sy += HUD_FONT_HEIGHT;
				
				if (sy + HUD_FONT_HEIGHT > area->bottom) 
					break;
			}
			
			if (c>=' ' || c<='z') 
			{
				int topLeftU = 1+((c-32)&15)*16;
				int topLeftV = 1+((c-32)>>4)*16;

				DrawSmallMenuCharacter(sx, sy, topLeftU, topLeftV, ONE_FIXED, ONE_FIXED, ONE_FIXED, alpha);

				sx += AAFontWidths[(unsigned int) c];
			}
		}
	}
	
	if (output_x) *output_x=sx;
	if (output_y) *output_y=sy;
}

void LoadAllMenuTextures()
{
	AVPMENUGFX_CLOUDY = Tex_CreateFromRIM("graphics/Menus/fractal.RIM");

	AVPMENUGFX_SMALL_FONT       = Tex_CreateFromRIM("graphics/Common/aa_font.RIM");
	AVPMENUGFX_COPYRIGHT_SCREEN = Tex_CreateFromRIM("graphics/Menus/Copyright.RIM");

	AVPMENUGFX_PRESENTS        = Tex_CreateFromRIM("graphics/Menus/FIandRD.RIM");
	AVPMENUGFX_AREBELLIONGAME  = Tex_CreateFromRIM("graphics/Menus/presents.RIM");
	AVPMENUGFX_ALIENSVPREDATOR = Tex_CreateFromRIM("graphics/Menus/AliensVPredator.RIM");

	AVPMENUGFX_SLIDERBAR       = Tex_CreateFromRIM("graphics/Menus/SliderBar.RIM");
	AVPMENUGFX_SLIDER          = Tex_CreateFromRIM("graphics/Menus/Slider.RIM");

	AVPMENUGFX_BACKDROP        = Tex_CreateFromRIM("graphics/Menus/Starfield.RIM");
	AVPMENUGFX_ALIENS_LOGO     = Tex_CreateFromRIM("graphics/Menus/aliens.RIM");
	AVPMENUGFX_ALIEN_LOGO      = Tex_CreateFromRIM("graphics/Menus/Alien.RIM");
	AVPMENUGFX_MARINE_LOGO     = Tex_CreateFromRIM("graphics/Menus/Marine.RIM");
	AVPMENUGFX_PREDATOR_LOGO   = Tex_CreateFromRIM("graphics/Menus/Predator.RIM");

	AVPMENUGFX_GLOWY_LEFT      = Tex_CreateFromRIM("graphics/Menus/glowy_left.RIM");
	AVPMENUGFX_GLOWY_MIDDLE    = Tex_CreateFromRIM("graphics/Menus/glowy_middle.RIM");
	AVPMENUGFX_GLOWY_RIGHT     = Tex_CreateFromRIM("graphics/Menus/glowy_right.RIM");

	AVPMENUGFX_MARINE_EPISODE1 = Tex_CreateFromRIM("graphics/Menus/MarineEpisode1.RIM");
	AVPMENUGFX_MARINE_EPISODE2 = Tex_CreateFromRIM("graphics/Menus/MarineEpisode2.RIM");
	AVPMENUGFX_MARINE_EPISODE3 = Tex_CreateFromRIM("graphics/Menus/MarineEpisode3.RIM");
	AVPMENUGFX_MARINE_EPISODE4 = Tex_CreateFromRIM("graphics/Menus/MarineEpisode4.RIM");
	AVPMENUGFX_MARINE_EPISODE5 = Tex_CreateFromRIM("graphics/Menus/MarineEpisode5.RIM");
	AVPMENUGFX_MARINE_EPISODE6 = Tex_CreateFromRIM("graphics/Menus/MarineEpisode6.RIM");

	AVPMENUGFX_MARINE_EPISODE7  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_MARINE_EPISODE8  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_MARINE_EPISODE9  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_MARINE_EPISODE10 = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_MARINE_EPISODE11 = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");

	AVPMENUGFX_PREDATOR_EPISODE1 = Tex_CreateFromRIM("graphics/Menus/PredatorEpisode1.RIM");
	AVPMENUGFX_PREDATOR_EPISODE2 = Tex_CreateFromRIM("graphics/Menus/PredatorEpisode2.RIM");
	AVPMENUGFX_PREDATOR_EPISODE3 = Tex_CreateFromRIM("graphics/Menus/PredatorEpisode3.RIM");
	AVPMENUGFX_PREDATOR_EPISODE4 = Tex_CreateFromRIM("graphics/Menus/PredatorEpisode4.RIM");
	AVPMENUGFX_PREDATOR_EPISODE5 = Tex_CreateFromRIM("graphics/Menus/PredatorEpisode5.RIM");
	AVPMENUGFX_PREDATOR_EPISODE6 = Tex_CreateFromRIM("graphics/Menus/PredatorEpisode5.RIM");

	AVPMENUGFX_PREDATOR_EPISODE7  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_PREDATOR_EPISODE8  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_PREDATOR_EPISODE9  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_PREDATOR_EPISODE10 = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_PREDATOR_EPISODE11 = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");

	AVPMENUGFX_ALIEN_EPISODE1  = Tex_CreateFromRIM("graphics/Menus/AlienEpisode2.RIM");
	AVPMENUGFX_ALIEN_EPISODE2  = Tex_CreateFromRIM("graphics/Menus/AlienEpisode4.RIM");
	AVPMENUGFX_ALIEN_EPISODE3  = Tex_CreateFromRIM("graphics/Menus/AlienEpisode1.RIM");
	AVPMENUGFX_ALIEN_EPISODE4  = Tex_CreateFromRIM("graphics/Menus/AlienEpisode3.RIM");
	AVPMENUGFX_ALIEN_EPISODE5  = Tex_CreateFromRIM("graphics/Menus/AlienEpisode5.RIM");
	AVPMENUGFX_ALIEN_EPISODE6  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_ALIEN_EPISODE7  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_ALIEN_EPISODE8  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_ALIEN_EPISODE9  = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");
	AVPMENUGFX_ALIEN_EPISODE10 = Tex_CreateFromRIM("graphics/Menus/bonus.RIM");

	// Splash screens
	#if MARINE_DEMO
		AVPMENUGFX_WINNER_SCREEN = Tex_CreateFromRIM("graphics/MarineSplash/splash00.RIM");

		AVPMENUGFX_SPLASH_SCREEN1 = Tex_CreateFromRIM("graphics/MarineSplash/splash01.RIM");
		AVPMENUGFX_SPLASH_SCREEN2 = Tex_CreateFromRIM("graphics/MarineSplash/splash02.RIM");
		AVPMENUGFX_SPLASH_SCREEN3 = Tex_CreateFromRIM("graphics/MarineSplash/splash03.RIM");
		AVPMENUGFX_SPLASH_SCREEN4 = Tex_CreateFromRIM("graphics/MarineSplash/splash04.RIM");
		AVPMENUGFX_SPLASH_SCREEN5 = Tex_CreateFromRIM("graphics/MarineSplash/splash05.RIM");
	#elif ALIEN_DEMO
		AVPMENUGFX_WINNER_SCREEN = Tex_CreateFromRIM("graphics/AlienSplash/splash00.RIM");

		AVPMENUGFX_SPLASH_SCREEN1 = Tex_CreateFromRIM("graphics/AlienSplash/splash01.RIM");
		AVPMENUGFX_SPLASH_SCREEN2 = Tex_CreateFromRIM("graphics/AlienSplash/splash02.RIM");
		AVPMENUGFX_SPLASH_SCREEN3 = Tex_CreateFromRIM("graphics/AlienSplash/splash03.RIM");
		AVPMENUGFX_SPLASH_SCREEN4 = Tex_CreateFromRIM("graphics/AlienSplash/splash04.RIM");
		AVPMENUGFX_SPLASH_SCREEN5 = Tex_CreateFromRIM("graphics/AlienSplash/splash05.RIM");
	#else
		AVPMENUGFX_WINNER_SCREEN = Tex_CreateFromRIM("graphics/PredatorSplash/splash00.RIM");

		AVPMENUGFX_SPLASH_SCREEN1 = Tex_CreateFromRIM("graphics/PredatorSplash/splash01.RIM");
		AVPMENUGFX_SPLASH_SCREEN2 = Tex_CreateFromRIM("graphics/PredatorSplash/splash02.RIM");
		AVPMENUGFX_SPLASH_SCREEN3 = Tex_CreateFromRIM("graphics/PredatorSplash/splash03.RIM");
		AVPMENUGFX_SPLASH_SCREEN4 = Tex_CreateFromRIM("graphics/PredatorSplash/splash04.RIM");
		AVPMENUGFX_SPLASH_SCREEN5 = Tex_CreateFromRIM("graphics/PredatorSplash/splash05.RIM");
	#endif
}

extern void LoadAllAvPMenuGfx(void)
{
	LoadAllMenuTextures();
	LoadMenuFont();
	CalculateWidthsOfAAFont();

	// call a function to remove the red grid from the small font texture- only if the texture is loaded
	if (MISSING_TEXTURE != AVPMENUGFX_SMALL_FONT)
	{
		DeRedTexture((Texture)Tex_GetTextureDetails(AVPMENUGFX_SMALL_FONT));
	}

	/*
		AVPMENUGFX_ALIEN_LOGO and friends were originally constants that could be assigned at 
		compile time (done in AvP_MenuData.c). As the IDs are no longer constants, we set them to 0
		at compile time, then update them here once they've been loaded and assigned a valid textureID
	*/
	AvPMenu_SinglePlayer[0].textureID = AVPMENUGFX_ALIEN_LOGO;
	AvPMenu_SinglePlayer[1].textureID = AVPMENUGFX_MARINE_LOGO;
	AvPMenu_SinglePlayer[2].textureID = AVPMENUGFX_PREDATOR_LOGO;
}

extern void ReleaseAllAvPMenuGfx(void)
{
	Tex_Release(AVPMENUGFX_CLOUDY);
	Tex_Release(AVPMENUGFX_SMALL_FONT);
	Tex_Release(AVPMENUGFX_COPYRIGHT_SCREEN);
	Tex_Release(AVPMENUGFX_PRESENTS);
	Tex_Release(AVPMENUGFX_AREBELLIONGAME);
	Tex_Release(AVPMENUGFX_ALIENSVPREDATOR);
	Tex_Release(AVPMENUGFX_SLIDERBAR);
	Tex_Release(AVPMENUGFX_SLIDER);
	Tex_Release(AVPMENUGFX_BACKDROP);
	Tex_Release(AVPMENUGFX_ALIENS_LOGO);
	Tex_Release(AVPMENUGFX_ALIEN_LOGO);
	Tex_Release(AVPMENUGFX_MARINE_LOGO);
	Tex_Release(AVPMENUGFX_PREDATOR_LOGO);
	Tex_Release(AVPMENUGFX_GLOWY_LEFT);
	Tex_Release(AVPMENUGFX_GLOWY_MIDDLE);
	Tex_Release(AVPMENUGFX_GLOWY_RIGHT);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE1);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE2);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE3);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE4);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE5);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE6);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE7);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE8);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE9);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE10);
	Tex_Release(AVPMENUGFX_MARINE_EPISODE11);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE1);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE2);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE3);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE4);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE5);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE6);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE7);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE8);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE9);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE10);
	Tex_Release(AVPMENUGFX_PREDATOR_EPISODE11);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE1);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE2);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE3);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE4);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE5);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE6);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE7);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE8);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE9);
	Tex_Release(AVPMENUGFX_ALIEN_EPISODE10);
	Tex_Release(AVPMENUGFX_WINNER_SCREEN);
	Tex_Release(AVPMENUGFX_SPLASH_SCREEN1);
	Tex_Release(AVPMENUGFX_SPLASH_SCREEN2);
	Tex_Release(AVPMENUGFX_SPLASH_SCREEN3);
	Tex_Release(AVPMENUGFX_SPLASH_SCREEN4);
	Tex_Release(AVPMENUGFX_SPLASH_SCREEN5);

	Tex_Release(IntroFont_Light.textureID);
}

extern void DrawAvPMenuGfx(texID_t textureID, int topleftX, int topleftY, int alpha, enum AVPMENUFORMAT_ID format)
{
	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	switch (format)
	{
		default:
		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			topleftX -= texWidth;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			topleftX -= texWidth / 2;
			break;
		}
	}

	int32_t width = texWidth;
	if (640 - topleftX < width)
	{
		width = 640 - topleftX;
	}

	if (width <= 0) 
	{
		return;
	}

	if (alpha > ONE_FIXED) // ONE_FIXED = 65536
			alpha = ONE_FIXED;

	DrawAlphaMenuQuad(topleftX, topleftY, textureID, alpha);
}

extern void DrawAvPMenuGfx_Faded(texID_t textureID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format)
{
	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	switch(format)
	{
		default:
		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			topleftX -= texWidth;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			topleftX -= texWidth / 2;
			break;
		}
	}
	
	int32_t length = texWidth;//gfxPtr->Width;

	if (/*ScreenDescriptorBlock.SDB_Width*/640 - topleftX < length) {
		length = /*ScreenDescriptorBlock.SDB_Width*/640 - topleftX;
	}
	if (length <= 0) 
	{
		return;
	}

	if (alpha > ONE_FIXED) // ONE_FIXED = 65536
		alpha = ONE_FIXED;

	DrawAlphaMenuQuad(topleftX, topleftY, textureID, alpha);
}

extern void DrawAvPMenuGfx_Clipped(texID_t textureID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format, int topY, int bottomY)
{
	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	switch (format)
	{
		default:
		GLOBALASSERT("UNKNOWN TEXT FORMAT"==0);
		case AVPMENUFORMAT_LEFTJUSTIFIED:
		{
			// supplied x is correct
			break;
		}
		case AVPMENUFORMAT_RIGHTJUSTIFIED:
		{
			topleftX -= texWidth;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			topleftX -= texWidth / 2;
			break;
		}
	}

	int32_t length = texWidth;

	if (/*ScreenDescriptorBlock.SDB_Width*/640 - topleftX < length) 
	{
		length = /*ScreenDescriptorBlock.SDB_Width*/640 - topleftX;
	}
	if (length <= 0)
	{	
		return;
	}

	if (alpha > ONE_FIXED) // ONE_FIXED = 65536
		alpha = ONE_FIXED;

	DrawAlphaMenuQuad(topleftX, topleftY, textureID, alpha);
}

extern int HeightOfMenuGfx(texID_t textureID)
{
	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	return texHeight;
}

static void CalculateWidthsOfAAFont(void)
{
	if (MISSING_TEXTURE == AVPMENUGFX_SMALL_FONT)
	{
		// the texture isn't loaded or hasn't loaded correctly so we can't perform the width calculation
		return;
	}

	uint32_t textureWidth  = 0;
	uint32_t textureHeight = 0;

	Tex_GetDimensions(AVPMENUGFX_SMALL_FONT, textureWidth, textureHeight);

	// we'll assume it can't be smaller than the original AvP font texture
	assert(textureWidth  >= 256);
	assert(textureHeight >= 256);

	uint8_t *originalSrcPtr = NULL;
	uint32_t pitch = 0;

	if (!Tex_Lock(AVPMENUGFX_SMALL_FONT, &originalSrcPtr, &pitch))
	{
		Con_PrintError("CalculateWidthsOfAAFont() failed - Can't lock texture");
		return;
	}

	uint8_t *srcPtr = originalSrcPtr;

	AAFontWidths[32] = 3;

	for (int c = 33; c < 255; c++) 
	{
		int x,y;
		int x1 = 1+((c-32)&15)*16;
		int y1 = 1+((c-32)>>4)*16;

		AAFontWidths[c] = 17;

		for (x = x1 + HUD_FONT_WIDTH; x > x1; x--)
		{
			bool blank = true;

			for (y = y1; y < y1 + HUD_FONT_HEIGHT; y++)
			{
				uint8_t *s = &srcPtr[(x + y*textureWidth) * 4]; // FIXME - pitch?

				if (s[0] >= 0x80) // checking blue.
				{
					blank = false;
					break;
				}
			}

			if (blank)
			{
				AAFontWidths[c]--;
			}
			else
			{
				break;
			}
		}
	}

	Tex_Unlock(AVPMENUGFX_SMALL_FONT);
}
