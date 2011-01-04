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

extern int Font_DrawText(const char* text, int x, int y, int colour, int fontType);

extern void D3D_RenderHUDString(char *stringPtr,int x,int y,int colour);
extern void DrawMenuQuad(int topX, int topY, int bottomX, int bottomY, int image_num, BOOL alpha);
extern void DrawMenuTextGlow(uint32_t topLeftX, uint32_t topLeftY, uint32_t size, uint32_t alpha);

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

#include "AvP_Menus.h"
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;

char AAFontWidths[256];

#if 0 // bjd - texture test
AVPMENUGFX AvPMenuGfxStorage[MAX_NO_OF_AVPMENUGFXS] =
{
	{"Menus\\fractal.rim"},
	{"Common\\aa_font.rim"},// Warning! Texture from common used

	{"Menus\\copyright.rim"},

	{"Menus\\FIandRD.rim"},
	{"Menus\\presents.rim"},
	{"Menus\\AliensVPredator.rim"},
	
	{"Menus\\sliderbar.rim"},//AVPMENUGFX_SLIDERBAR,
	{"Menus\\slider.rim"},//AVPMENUGFX_SLIDER,

	{"Menus\\starfield.rim"},
	{"Menus\\aliens.rim"},
	{"Menus\\Alien.rim"},
	{"Menus\\Marine.rim"},
	{"Menus\\Predator.rim"},

	{"Menus\\glowy_left.rim"},
	{"Menus\\glowy_middle.rim"},
	{"Menus\\glowy_right.rim"},
	
	// Marine level
	{"Menus\\MarineEpisode1.rim"},
	{"Menus\\MarineEpisode2.rim"},
	{"Menus\\MarineEpisode3.rim"},
	{"Menus\\MarineEpisode4.rim"},
	{"Menus\\MarineEpisode5.rim"},
	{"Menus\\MarineEpisode6.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	
	// Predator level
	{"Menus\\PredatorEpisode1.rim"},
	{"Menus\\PredatorEpisode2.rim"},
	{"Menus\\PredatorEpisode3.rim"},
	{"Menus\\PredatorEpisode4.rim"},
	{"Menus\\PredatorEpisode5.rim"},
	{"Menus\\PredatorEpisode5.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},

	// Alien level
	{"Menus\\AlienEpisode2.rim"},
	{"Menus\\AlienEpisode4.rim"},
	{"Menus\\AlienEpisode1.rim"},
	{"Menus\\AlienEpisode3.rim"},
	{"Menus\\AlienEpisode5.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},
	{"Menus\\bonus.rim"},

	// Splash screens
	#if MARINE_DEMO
	{"MarineSplash\\splash00.rim"},
	{"MarineSplash\\splash01.rim"},
	{"MarineSplash\\splash02.rim"},
	{"MarineSplash\\splash03.rim"},
	{"MarineSplash\\splash04.rim"},
	{"MarineSplash\\splash05.rim"},
	#elif ALIEN_DEMO
	{"AlienSplash\\splash00.rim"},
	{"AlienSplash\\splash01.rim"},
	{"AlienSplash\\splash02.rim"},
	{"AlienSplash\\splash03.rim"},
	{"AlienSplash\\splash04.rim"},
	{"AlienSplash\\splash05.rim"},
	#else
	{"PredatorSplash\\splash00.rim"},
	{"PredatorSplash\\splash01.rim"},
	{"PredatorSplash\\splash02.rim"},
	{"PredatorSplash\\splash03.rim"},
	{"PredatorSplash\\splash04.rim"},
	{"PredatorSplash\\splash05.rim"},
	#endif
};
#endif

static void LoadMenuFont(void);
static void UnloadMenuFont(void);
/*static*/ extern int RenderSmallFontString(char *textPtr,int sx,int sy,int alpha, int red, int green, int blue);
static void CalculateWidthsOfAAFont(void);

extern void D3D_Rectangle(int x0, int y0, int x1, int y1, int r, int g, int b, int a);

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

	AVPMENUGFX *gfxPtr;
	size_t fastFileLength;
	char buffer[100];
	void const *pFastFileData;
	
	IntroFont_Light.height = 33;
	IntroFont_Light.swidth = 5;
	IntroFont_Light.ascii = 32;

	gfxPtr = &IntroFont_Light.info;
//	gfxPtr->menuTexture = NULL;
	
	CL_GetImageFileName(buffer, 100, "Menus\\IntroFont.rim", LIO_RELATIVEPATH);
	
	pFastFileData = ffreadbuf(buffer, &fastFileLength);

	if (pFastFileData) 
	{
		gfxPtr->ImagePtr = AwCreateTexture(
			"pxfXY",
			pFastFileData,
			fastFileLength,
			AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
			&(gfxPtr->Width),
			&(gfxPtr->Height)
		);
	} 
	else 
	{
		gfxPtr->ImagePtr = AwCreateTexture(
			"sfXY",
			buffer,
			AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
			&(gfxPtr->Width),
			&(gfxPtr->Height)
		);
	}
	
	GLOBALASSERT(gfxPtr->ImagePtr);
	GLOBALASSERT(gfxPtr->Width>0);
	GLOBALASSERT(gfxPtr->Height>0);
	
	AVPTEXTURE *image = gfxPtr->ImagePtr;

	uint8_t *srcPtr = image->buffer;
/*
	if ((image->width != 30) || ((image->height % 33) != 0)) {
		// handle new texture
	}
*/	
	IntroFont_Light.numchars = image->height / 33;	
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
				uint8_t *s = &srcPtr[(x + y*image->width) * sizeof(uint32_t)];
				if (s[2]) 
				{
					blank = false;
					break;
				}
			}
			
			if (blank) 
			{
				IntroFont_Light.FontWidth[c]--;
			} 
			else 
			{
				break;
			}
		}
	}

	// we're going to try create a square texture
	gfxPtr->textureID = Tex_CreateTallFontTexture(buffer, *image, TextureUsage_Normal);
}

static void UnloadMenuFont(void)
{
	ReleaseAvPTexture(IntroFont_Light.info.ImagePtr);
	IntroFont_Light.info.ImagePtr = NULL;

	Tex_Release(IntroFont_Light.info.textureID);
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
			//pX -= (pFont->CalcSize(textPtr).w);
			pX -= width;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			//pX -= (pFont->CalcSize(textPtr).w)/2;
			pX -= width / 2;
			break;
		}
	}

	// bjd - fixme. Long video card names wont fit on screen!
//	LOCALASSERT(pX > 0);
	
	extern enum MENUSSTATE_ID;

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
			int column = c % 15; // get column from remainder value

			int tex_y = row * 33;
			int tex_x = column * 30;

			int topLeftU = tex_x;
			int topLeftV = tex_y;

			/* 
				bjd note
				need to fix the 'Cloud Table'
				ie the moving hazy smoke/cloud effect behind the large font text on the menus
			*/

			DrawTallFontCharacter(positionX, positionY, IntroFont_Light.info.textureID, topLeftU, topLeftV, charWidth, alpha);
/*			
			srcPtr = &image->buf[(topLeftU+topLeftV*image->w)*4];
			
			for (y=pY; y<33+pY; y++) {
				destPtr = (unsigned short *)(((unsigned char *)lock.pBits)+y*lock.Pitch) + pX;
				
				for (x=width; x>0; x--) {					
					if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
						unsigned int destR, destG, destB;
						
						int r = CloudTable[(x+pX+CloakingPhase/64)&127][(y+CloakingPhase/128)&127];
						r = MUL_FIXED(alpha, r);
						
						destR = (*destPtr & 0xF800)>>8;
						destG = (*destPtr & 0x07E0)>>3;
						destB = (*destPtr & 0x001F)<<3;
						
						destR += MUL_FIXED(r, srcPtr[0]);
						destG += MUL_FIXED(r, srcPtr[1]);
						destB += MUL_FIXED(r, srcPtr[2]);
						if (destR > 0x00FF) destR = 0x00FF;
						if (destG > 0x00FF) destG = 0x00FF;
						if (destB > 0x00FF) destB = 0x00FF;
						
						*destPtr =	((destR>>3)<<11) |
								((destG>>2)<<5 ) |
								((destB>>3));
					}
					srcPtr += 4;
					destPtr++;
				}
				srcPtr += (image->w - width) * 4;
			}
*/
			positionX += charWidth;
			word_length += charWidth;
		}
	}

	return positionX;
}

extern int RenderMenuText_Clipped(char *textPtr, int pX, int pY, int alpha, enum AVPMENUFORMAT_ID format, int topY, int bottomY) 
{
	// this'll do for now :)
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

			while(*ptr)
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

			while(*ptr)
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
//		Font_DrawText(textPtr, x, y, colour, 1);
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

//	float totalWidth2f = (((float)ScreenDescriptorBlock.SDB_Width / 100) * 96.875f);
//	float totalHeight2f = (((float)ScreenDescriptorBlock.SDB_Height / 100) * 52.083f);

	int x = 10;
	int y = 150;

//	float x2f = (((float)ScreenDescriptorBlock.SDB_Width / 100) * 1.5625f);
//	float y2f = (((float)ScreenDescriptorBlock.SDB_Height / 100) * 31.25f);

//	uint32_t totalWidth2 = XPercentToScreen(96.875f);
//	uint32_t totalHeight2 = YPercentToScreen(52.083f);

//	uint32_t x2 = XPercentToScreen(1.5625f);
//	uint32_t y2 = YPercentToScreen(31.25f);

//	char buf[150];
//	sprintf(buf, "x2: %f y2: %f width: %f height: %f\n", x2, y2, totalWidth2, totalHeight2);
//	OutputDebugString(buf);

	// top horizonal segment
	DrawQuad(x, y, totalWidth, segHeight, NO_TEXTURE, colour, TRANSLUCENCY_NORMAL);

	// draw left segment
	DrawQuad(x, y + segHeight, segWidth, (totalHeight-segHeight*2), NO_TEXTURE, colour, TRANSLUCENCY_NORMAL);

	// right segment
	DrawQuad(x + totalWidth - segWidth, y + segHeight, segWidth, (totalHeight-segHeight*2), NO_TEXTURE, colour, TRANSLUCENCY_NORMAL);

	// bottom segment
	DrawQuad(x, y + totalHeight-segHeight, totalWidth, segHeight, NO_TEXTURE, colour, TRANSLUCENCY_NORMAL);
}

extern void Hardware_RenderKeyConfigRectangle(int alpha)
{
	RenderKeyConfigRectangle(alpha);
}

extern void Hardware_RenderHighlightRectangle(int x1, int y1, int x2, int y2, int r, int g, int b)
{
	D3D_Rectangle(x1, y1, x2, y2, r, g, b, 255);
}

extern void RenderHighlightRectangle(int x1, int y1, int x2, int y2, int r, int g, int b)
{
	// green rectangle highlight on load screen etc
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

	while ( *textPtr )
	{
		char c = *textPtr++;

		if (c>=' ')
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

//	unsigned char *srcPtr;
//	AVPMENUGFX *gfxPtr;
//	AvPTexture *image;
	int wordWidth;
	int sx = area->left;
	int sy = area->top;
	
//	gfxPtr = &AvPMenuGfxStorage[AVPMENUGFX_SMALL_FONT];
//	image = (AvPTexture*)gfxPtr->ImagePtr;

/*
Determine area used by text , so we can draw it centrally
*/
	const char *textPtr2=textPtr;
	while (*textPtr2) 
	{
		int widthFromSpaces=0;
		int widthFromChars=0;
		
		while (*textPtr2 && *textPtr2==' ') 
		{
			widthFromSpaces+=AAFontWidths[(unsigned int) *textPtr2++];
		}
		
		while (*textPtr2 && *textPtr2!=' ') 
		{
			widthFromChars+=AAFontWidths[(unsigned int) *textPtr2++];
		}
		
		wordWidth=widthFromSpaces+widthFromChars;
		
		if (wordWidth> area->right-sx) 
		{
			if (wordWidth >area->right-area->left) 
			{
				int extraLinesNeeded=0;
				
				wordWidth-=(area->right-sx);
				
				sy+=HUD_FONT_HEIGHT;
				sx=area->left;
				
				extraLinesNeeded=wordWidth/(area->right-area->left);
				
				sy+=HUD_FONT_HEIGHT*extraLinesNeeded;
				wordWidth %= (area->right-area->left);
				
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
	
	sy+=HUD_FONT_HEIGHT;
	if(sy<area->bottom) 
	{
		sy=area->top + (area->bottom-sy)/2;
	} 
	else 
	{
		sy=area->top;
	}

	while ( *textPtr ) 
	{
		const char* textPtr2=textPtr;
		wordWidth=0;
		
		while(*textPtr2 && *textPtr2==' ') {
			wordWidth+=AAFontWidths[(unsigned int) *textPtr2++];
		}
		
		while(*textPtr2 && *textPtr2!=' ') {
			wordWidth+=AAFontWidths[(unsigned int) *textPtr2++];
		}
		
		if (wordWidth> area->right-sx) 
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
				sy+=HUD_FONT_HEIGHT;
				sx=area->left;
				
				if (sy+HUD_FONT_HEIGHT> area->bottom) break;
				
				if (wordWidth> area->right-sx) break;
				
				while (*textPtr && *textPtr==' ') {
					textPtr++;
				}
			}
		}

		while (*textPtr && *textPtr==' ') 
		{
			sx+=AAFontWidths[(unsigned int) *textPtr++];
		}
		
		if (sx>area->right) 
		{
			while (sx>area->right) 
			{
				sx-=(area->right-area->left);
				sy+=HUD_FONT_HEIGHT;
			}
			
			if (sy+HUD_FONT_HEIGHT> area->bottom) 
				break;
		}
		
		while (*textPtr && *textPtr!=' ') 
		{
			char c = *textPtr++;
			int letterWidth = AAFontWidths[(unsigned int) c];
			
			if (sx+letterWidth>area->right) 
			{
				sx=area->left;
				sy+=HUD_FONT_HEIGHT;
				
				if(sy+HUD_FONT_HEIGHT> area->bottom) break;
			}
			
			if (c>=' ' || c<='z') 
			{
				int topLeftU = 1+((c-32)&15)*16;
				int topLeftV = 1+((c-32)>>4)*16;
//				int x, y;
				
//				srcPtr = &image->buffer[(topLeftU+topLeftV*image->width)*4];

				DrawSmallMenuCharacter(sx, sy, topLeftU, topLeftV, ONE_FIXED, ONE_FIXED, ONE_FIXED, alpha);
#if 0	
				for (y=sy; y<HUD_FONT_HEIGHT+sy; y++) 
				{
//					destPtr = (unsigned short *)(((unsigned char *)lock.pBits) + y*lock.Pitch) + sx;
					
					for (x=0; x<HUD_FONT_WIDTH; x++) 
					{
/*
						if (srcPtr[0] || srcPtr[1] || srcPtr[2]) {
							unsigned int destR, destG, destB;
						
							destR = (*destPtr & 0xF800)>>8;
							destG = (*destPtr & 0x07E0)>>3;
							destB = (*destPtr & 0x001F)<<3;
						
							destR += MUL_FIXED(alpha, srcPtr[0]);
							destG += MUL_FIXED(alpha, srcPtr[1]);
							destB += MUL_FIXED(alpha, srcPtr[2]);
							if (destR > 0x00FF) destR = 0x00FF;
							if (destG > 0x00FF) destG = 0x00FF;
							if (destB > 0x00FF) destB = 0x00FF;
						
							*destPtr =	((destR>>3)<<11) |
									((destG>>2)<<5 ) |
									((destB>>3));
						}
*/
//						srcPtr += 4;
//						destPtr++;
					}
//					srcPtr += (image->w - HUD_FONT_WIDTH) * 4;	
				}
#endif
				sx += AAFontWidths[(unsigned int) c];
			}
		}
	}
	
	if (output_x) *output_x=sx;
	if (output_y) *output_y=sy;
}

void LoadAllMenuTextures()
{
	AVPMENUGFX_CLOUDY = CL_LoadImageOnce("Menus\\fractal.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_SMALL_FONT  = CL_LoadImageOnce("Common\\aa_font.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_COPYRIGHT_SCREEN = CL_LoadImageOnce("Menus\\copyright.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_PRESENTS = CL_LoadImageOnce("Menus\\FIandRD.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_AREBELLIONGAME = CL_LoadImageOnce("Menus\\presents.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIENSVPREDATOR = CL_LoadImageOnce("Menus\\AliensVPredator.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_SLIDERBAR = CL_LoadImageOnce("Menus\\sliderbar.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_SLIDER = CL_LoadImageOnce("Menus\\slider.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_BACKDROP = CL_LoadImageOnce("Menus\\starfield.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIENS_LOGO = CL_LoadImageOnce("Menus\\aliens.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_LOGO = CL_LoadImageOnce("Menus\\Alien.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_LOGO = CL_LoadImageOnce("Menus\\Marine.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_LOGO = CL_LoadImageOnce("Menus\\Predator.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_GLOWY_LEFT = CL_LoadImageOnce("Menus\\glowy_left.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_GLOWY_MIDDLE = CL_LoadImageOnce("Menus\\glowy_middle.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_GLOWY_RIGHT = CL_LoadImageOnce("Menus\\glowy_right.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_MARINE_EPISODE1 = CL_LoadImageOnce("Menus\\MarineEpisode1.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE2 = CL_LoadImageOnce("Menus\\MarineEpisode2.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE3 = CL_LoadImageOnce("Menus\\MarineEpisode3.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE4 = CL_LoadImageOnce("Menus\\MarineEpisode4.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE5 = CL_LoadImageOnce("Menus\\MarineEpisode5.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE6 = CL_LoadImageOnce("Menus\\MarineEpisode6.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_MARINE_EPISODE7 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE8 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE9 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE10 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_MARINE_EPISODE11 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_PREDATOR_EPISODE1 = CL_LoadImageOnce("Menus\\PredatorEpisode1.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE2 = CL_LoadImageOnce("Menus\\PredatorEpisode2.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE3 = CL_LoadImageOnce("Menus\\PredatorEpisode3.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE4 = CL_LoadImageOnce("Menus\\PredatorEpisode4.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE5 = CL_LoadImageOnce("Menus\\PredatorEpisode5.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE6 = CL_LoadImageOnce("Menus\\PredatorEpisode5.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_PREDATOR_EPISODE7 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE8 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE9 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE10 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_PREDATOR_EPISODE11 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	AVPMENUGFX_ALIEN_EPISODE1 = CL_LoadImageOnce("Menus\\AlienEpisode2.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE2 = CL_LoadImageOnce("Menus\\AlienEpisode4.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE3 = CL_LoadImageOnce("Menus\\AlienEpisode1.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE4 = CL_LoadImageOnce("Menus\\AlienEpisode3.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE5 = CL_LoadImageOnce("Menus\\AlienEpisode5.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE6 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE7 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE8 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE9 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	AVPMENUGFX_ALIEN_EPISODE10 = CL_LoadImageOnce("Menus\\bonus.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

	// Splash screens
	#if MARINE_DEMO
		AVPMENUGFX_WINNER_SCREEN = CL_LoadImageOnce("MarineSplash\\splash00.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

		AVPMENUGFX_SPLASH_SCREEN1 = CL_LoadImageOnce("MarineSplash\\splash01.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN2 = CL_LoadImageOnce("MarineSplash\\splash02.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN3 = CL_LoadImageOnce("MarineSplash\\splash03.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN4 = CL_LoadImageOnce("MarineSplash\\splash04.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN5 = CL_LoadImageOnce("MarineSplash\\splash05.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	#elif ALIEN_DEMO
		AVPMENUGFX_WINNER_SCREEN = CL_LoadImageOnce("AlienSplash\\splash00.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

		AVPMENUGFX_SPLASH_SCREEN1 = CL_LoadImageOnce("AlienSplash\\splash01.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN2 = CL_LoadImageOnce("AlienSplash\\splash02.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN3 = CL_LoadImageOnce("AlienSplash\\splash03.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN4 = CL_LoadImageOnce("AlienSplash\\splash04.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN5 = CL_LoadImageOnce("AlienSplash\\splash05.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	#else
		AVPMENUGFX_WINNER_SCREEN = CL_LoadImageOnce("PredatorSplash\\splash00.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);

		AVPMENUGFX_SPLASH_SCREEN1 = CL_LoadImageOnce("PredatorSplash\\splash01.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN2 = CL_LoadImageOnce("PredatorSplash\\splash02.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN3 = CL_LoadImageOnce("PredatorSplash\\splash03.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN4 = CL_LoadImageOnce("PredatorSplash\\splash04.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
		AVPMENUGFX_SPLASH_SCREEN5 = CL_LoadImageOnce("PredatorSplash\\splash05.rim", LIO_D3DTEXTURE | LIO_RELATIVEPATH | LIO_RESTORABLE);
	#endif
}

extern void LoadAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{
#if 0 // bjd - texture test

	AVPMENUGFX *gfxPtr;
	char buffer[100];
	
	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);

	gfxPtr = &AvPMenuGfxStorage[menuGfxID];

	CL_GetImageFileName(buffer, 100, gfxPtr->FilenamePtr, LIO_RELATIVEPATH);
	
	//see if graphic can be found in fast file
	size_t fastFileLength;
	void const * pFastFileData = ffreadbuf(buffer,&fastFileLength);

	if (pFastFileData)
	{
		//D3DTexture *ImagePtr; 

		/* bjd
		I think we use AwCreateSurface to tell the engine we'll be loading non-power of 2 
		or image files for "raw" pixel by pixel drawing.

		Eg, linux code:

			if (pixelFormat.texB) {
				Tex->buf = NULL; 
				CreateOGLTexture(Tex, buf); 
				free(buf);
			} else {
				Tex->buf = buf; 
				CreateIMGSurface(Tex, buf);
			}
			
		Only create a texture (d3d texture, ogl texture etc) IF pixelFormat.texB is true.
		.texB being true means its a TEXTURE, not surface creation function we called.
		We don't need to keep a copy of Tex->buf with this.

		If .textB is false and we're dealing with a surface, stick all the buffered image data 
		into tex->buf so we can do pixel by pixel drawing with it later
		*/

		//load from fast file
		gfxPtr->ImagePtr = AwCreateTexture
							(
								"pxfXYB",
								pFastFileData,
								fastFileLength,
								AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
								&(gfxPtr->Width),
								&(gfxPtr->Height)
							);
	}
	else
	{
		//load graphic from rim file
		gfxPtr->ImagePtr = AwCreateTexture
							(
								"sfXYB",
								buffer,
								AW_TLF_TRANSP|AW_TLF_CHROMAKEY,
								&(gfxPtr->Width),
								&(gfxPtr->Height)
							);

	}

	if (gfxPtr->ImagePtr) 
	{
		AvPMenuGfxStorage[menuGfxID].menuTexture = CreateD3DTexturePadded(gfxPtr->ImagePtr, (uint32_t*)&gfxPtr->newWidth, (uint32_t*)&gfxPtr->newHeight);

		if (AvPMenuGfxStorage[menuGfxID].menuTexture == NULL) {
			OutputDebugString("Texture in AvPMenuGfxStorage was NULL!");
		}
	}
	else
	{
		// bjd - bail out?
		gfxPtr->Height = 0;
		gfxPtr->Width = 0;
		return;
	}

	// remove the red outline grid from the font texture
	if (menuGfxID == AVPMENUGFX_SMALL_FONT)
	{
		DeRedTexture(AvPMenuGfxStorage[menuGfxID].menuTexture);
	}

	GLOBALASSERT(gfxPtr->ImagePtr);
	GLOBALASSERT(gfxPtr->Width  > 0);
	GLOBALASSERT(gfxPtr->Height > 0);

#endif
}

static void ReleaseAvPMenuGfx(enum AVPMENUGFX_ID menuGfxID)
{

#if 0 // bjd - texture test
	AVPMENUGFX *gfxPtr;
	
	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);
		
	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
	
	GLOBALASSERT(gfxPtr);
	GLOBALASSERT(gfxPtr->ImagePtr);

	ReleaseAvPTexture(gfxPtr->ImagePtr);
	gfxPtr->ImagePtr = NULL;

	/* release d3d texture */
	SAFE_RELEASE(gfxPtr->menuTexture);
#endif
}

extern AVPMENU_ELEMENT AvPMenu_SinglePlayer[];

extern void LoadAllAvPMenuGfx(void)
{
	int i = 0;

	LoadAllMenuTextures();
	LoadMenuFont();
	CalculateWidthsOfAAFont();

	// call a function to remove the red grid from the small font texture
	DeRedTexture(Tex_GetTexture(AVPMENUGFX_SMALL_FONT));

	/*
		AVPMENUGFX_ALIEN_LOGO and friends were originally constants that could be assigned at 
		compile time (done in AvP_MenuData.c). As the IDs are no longer constants, we set them to 0
		at compile time, then update them here once they've been loaded and assigned a valid textureID
	*/
	AvPMenu_SinglePlayer[0].textureID = AVPMENUGFX_ALIEN_LOGO;
	AvPMenu_SinglePlayer[1].textureID = AVPMENUGFX_MARINE_LOGO;
	AvPMenu_SinglePlayer[2].textureID = AVPMENUGFX_PREDATOR_LOGO;

#if 0 // bjd - texture test
	while (i < AVPMENUGFX_WINNER_SCREEN)
	{
		LoadAvPMenuGfx((enum AVPMENUGFX_ID)i++);
	}

	LoadMenuFont();
	
	uint8_t *srcPtr = 0;

	AVPMENUGFX *gfxPtr = &AvPMenuGfxStorage[AVPMENUGFX_CLOUDY];
	
	AVPTEXTURE *image = gfxPtr->ImagePtr;

	if (image != NULL)
	{
		srcPtr = image->buffer;

		for (uint32_t y = 0; y < gfxPtr->Height; y++)
		{
			for (uint32_t x = 0; x < gfxPtr->Width; x++)
			{
				int r = srcPtr[0];

				r = DIV_FIXED(r, 0xFF);
				CloudTable[x][y] = r;
		
				srcPtr += 4;
			}
		}
	}

	CalculateWidthsOfAAFont();
#endif
}

extern void LoadAllSplashScreenGfx(void)
{
#if 0 // bjd - texture test
	int i = AVPMENUGFX_SPLASH_SCREEN1;
	while(i<MAX_NO_OF_AVPMENUGFXS)
	{
		LoadAvPMenuGfx((enum AVPMENUGFX_ID)i++);
	}
#endif
}

extern void InitialiseMenuGfx(void)
{
#if 0 // bjd - texture test
	int i=0;
	while(i<MAX_NO_OF_AVPMENUGFXS)
	{
		AvPMenuGfxStorage[i++].ImagePtr = 0;
	}
#endif
}

extern void ReleaseAllAvPMenuGfx(void)
{
	// sigh..find a better way to do this..
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

/*
	for (int i = AVPMENUGFX_CLOUDY; i < AVPMENUGFX_SPLASH_SCREEN5; i++)
	{
		if (i == -1) return;
		Tex_Release(i);
	}
*/
#if 0 // bjd - texture test
	int i=0;
	while(i<MAX_NO_OF_AVPMENUGFXS)
	{
		if (AvPMenuGfxStorage[i].ImagePtr)
		{
			ReleaseAvPMenuGfx((enum AVPMENUGFX_ID)i);
		}
		i++;
	}	
	UnloadMenuFont();
#endif
}

extern void DrawAvPMenuGfx(/*enum AVPMENUGFX_ID menuGfxID*/uint32_t textureID, int topleftX, int topleftY, int alpha, enum AVPMENUFORMAT_ID format)
{
	// bjd - texture tset
//	AVPMENUGFX *gfxPtr;
	
//	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);

//	gfxPtr = &AvPMenuGfxStorage[menuGfxID];
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
			topleftX -= texWidth;//gfxPtr->Width;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			topleftX -= /*gfxPtr->Width*/texWidth / 2;
			break;
		}
	}

	int32_t width = /*gfxPtr->Width*/texWidth;
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

	DrawAlphaMenuQuad(topleftX, topleftY, /*menuGfxID*/textureID, alpha);
}

extern void DrawAvPMenuGfx_Faded(/*enum AVPMENUGFX_ID menuGfxID*/uint32_t textureID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format)
{
	// bjd - texture test
//	AVPMENUGFX *gfxPtr;
//	gfxPtr = &AvPMenuGfxStorage[menuGfxID];

	// bjd - temp
	if (textureID == 0)
		return;

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
			topleftX -= texWidth;//gfxPtr->Width;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			topleftX -= /*gfxPtr->Width*/texWidth / 2;
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

extern void DrawAvPMenuGfx_Clipped(/*enum AVPMENUGFX_ID menuGfxID*/uint32_t textureID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format, int topY, int bottomY)
{	
	// bjd - texture test
//	GLOBALASSERT(menuGfxID < MAX_NO_OF_AVPMENUGFXS);

//	AVPMENUGFX *gfxPtr;
//	gfxPtr = &AvPMenuGfxStorage[menuGfxID];

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
			topleftX -= texWidth;//gfxPtr->Width;
			break;
		}
		case AVPMENUFORMAT_CENTREJUSTIFIED:
		{
			topleftX -= /*gfxPtr->Width*/texWidth / 2;
			break;
		}
	}

	int32_t length = texWidth;//gfxPtr->Width;

	if (/*ScreenDescriptorBlock.SDB_Width*/640 - topleftX < length) 
	{
		length = /*ScreenDescriptorBlock.SDB_Width*/640 - topleftX;
	}
	if (length <= 0)
	{	
		return;
	}

//	DrawMenuQuad(topleftX, topleftY, topleftX, topleftY, menuGfxID, alpha);

	if (alpha > ONE_FIXED) // ONE_FIXED = 65536
		alpha = ONE_FIXED;

	DrawAlphaMenuQuad(topleftX, topleftY, /*menuGfxID*/textureID, alpha);
}

extern int HeightOfMenuGfx(/*enum AVPMENUGFX_ID menuGfxID*/uint32_t textureID)
{
	// bjd - texture test
	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	return texHeight;
//	return AvPMenuGfxStorage[menuGfxID].Height; 
}

static void CalculateWidthsOfAAFont(void)
{
	// bjd - texture test
	int c;

	uint32_t textureWidth = 0;
	uint32_t textureHeight = 0;

	Tex_GetDimensions(AVPMENUGFX_SMALL_FONT, textureWidth, textureHeight);

	uint8_t *originalSrcPtr = NULL;
	uint32_t pitch = 0;

	if (!Tex_Lock(AVPMENUGFX_SMALL_FONT, &originalSrcPtr, &pitch))
	{
		Con_PrintError("CalculateWidthsOfAAFont() failed - Can't lock texture");
		return;
	}

	uint8_t *srcPtr = originalSrcPtr;

	AAFontWidths[32] = 3;

	for (c = 33; c < 255; c++) 
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
