#ifndef _included_AvP_MenuGfx_h_
#define _included_AvP_MenuGfx_h_

/* KJL 12:27:18 26/06/98 - AvP_MenuGfx.hpp */

extern texID_t AVPMENUGFX_CLOUDY;
extern texID_t AVPMENUGFX_SMALL_FONT;
extern texID_t AVPMENUGFX_COPYRIGHT_SCREEN;

extern texID_t AVPMENUGFX_PRESENTS;
extern texID_t AVPMENUGFX_AREBELLIONGAME;
extern texID_t AVPMENUGFX_ALIENSVPREDATOR;

extern texID_t AVPMENUGFX_SLIDERBAR;
extern texID_t AVPMENUGFX_SLIDER;

extern texID_t AVPMENUGFX_BACKDROP;
extern texID_t AVPMENUGFX_ALIENS_LOGO;
extern texID_t AVPMENUGFX_ALIEN_LOGO;
extern texID_t AVPMENUGFX_MARINE_LOGO;
extern texID_t AVPMENUGFX_PREDATOR_LOGO;

extern texID_t AVPMENUGFX_GLOWY_LEFT;
extern texID_t AVPMENUGFX_GLOWY_MIDDLE;
extern texID_t AVPMENUGFX_GLOWY_RIGHT;

extern texID_t AVPMENUGFX_MARINE_EPISODE1;
extern texID_t AVPMENUGFX_MARINE_EPISODE2;
extern texID_t AVPMENUGFX_MARINE_EPISODE3;
extern texID_t AVPMENUGFX_MARINE_EPISODE4;
extern texID_t AVPMENUGFX_MARINE_EPISODE5;
extern texID_t AVPMENUGFX_MARINE_EPISODE6;

extern texID_t AVPMENUGFX_MARINE_EPISODE7;
extern texID_t AVPMENUGFX_MARINE_EPISODE8;
extern texID_t AVPMENUGFX_MARINE_EPISODE9;
extern texID_t AVPMENUGFX_MARINE_EPISODE10;
extern texID_t AVPMENUGFX_MARINE_EPISODE11;

extern texID_t AVPMENUGFX_PREDATOR_EPISODE1;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE2;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE3;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE4;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE5;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE6;

extern texID_t AVPMENUGFX_PREDATOR_EPISODE7;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE8;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE9;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE10;
extern texID_t AVPMENUGFX_PREDATOR_EPISODE11;

extern texID_t AVPMENUGFX_ALIEN_EPISODE1;
extern texID_t AVPMENUGFX_ALIEN_EPISODE2;
extern texID_t AVPMENUGFX_ALIEN_EPISODE3;
extern texID_t AVPMENUGFX_ALIEN_EPISODE4;
extern texID_t AVPMENUGFX_ALIEN_EPISODE5;
extern texID_t AVPMENUGFX_ALIEN_EPISODE6;
extern texID_t AVPMENUGFX_ALIEN_EPISODE7;
extern texID_t AVPMENUGFX_ALIEN_EPISODE8;
extern texID_t AVPMENUGFX_ALIEN_EPISODE9;
extern texID_t AVPMENUGFX_ALIEN_EPISODE10;

extern texID_t AVPMENUGFX_WINNER_SCREEN;

extern texID_t AVPMENUGFX_SPLASH_SCREEN1;
extern texID_t AVPMENUGFX_SPLASH_SCREEN2;
extern texID_t AVPMENUGFX_SPLASH_SCREEN3;
extern texID_t AVPMENUGFX_SPLASH_SCREEN4;
extern texID_t AVPMENUGFX_SPLASH_SCREEN5;

typedef struct AVPIndexedFont
{
	texID_t textureID;
	int swidth;			// width for space
	int ascii;			// ascii code for initial character
	uint8_t charHeight;	// height per character
	uint8_t charWidth;

	uint8_t nRows;
	uint8_t nColumns;

	int numchars;
	int FontWidth[256];
} AVPIndexedFont;

enum AVPMENUFORMAT_ID
{
	AVPMENUFORMAT_LEFTJUSTIFIED,
	AVPMENUFORMAT_RIGHTJUSTIFIED,
	AVPMENUFORMAT_CENTREJUSTIFIED,
};

extern void LoadAllAvPMenuGfx(void);
extern void ReleaseAllAvPMenuGfx(void);

extern int RenderMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format);

extern int RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format);
extern int RenderSmallMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue);

extern int Hardware_RenderSmallMenuText(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format);
extern int Hardware_RenderSmallMenuText_Coloured(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int red, int green, int blue);

extern int RenderSmallChar(char c, int x, int y, int alpha, int red, int green, int blue);
extern int RenderMenuText_Clipped(char *textPtr, int x, int y, int alpha, enum AVPMENUFORMAT_ID format, int topY, int bottomY);
extern void RenderSmallFontString_Wrapped(char *textPtr, RECT* area, int alpha, int* output_x, int* output_y);

extern void DrawAvPMenuGfx(texID_t textureID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format);
extern void DrawAvPMenuGfx_Clipped(texID_t textureID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format, int topY, int bottomY);
extern void DrawAvPMenuGfx_Faded(texID_t textureID, int topleftX, int topleftY, int alpha,enum AVPMENUFORMAT_ID format);
extern int HeightOfMenuGfx(texID_t textureID);

#endif
