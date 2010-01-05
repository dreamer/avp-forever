#ifndef _included_font2_h_
#define _included_font2_h_

enum
{
	FONT_SMALL,
	FONT_BIG,
	NUM_FONT_TYPES
};

void Font_Init();
int Font_DrawText(const char* text, int x, int y, int colour, int fontType);
void Font_Release();

#endif