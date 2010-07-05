#include "langenum.h"
extern void D3D_HUD_Setup(void);
extern void D3D_HUDQuad_Output(int32_t textureID, struct VertexTag *quadVerticesPtr, uint32_t colour, enum FILTERING_MODE_ID filteringType);

extern int TransparencyLevelOfHUD;
/* takes values of 0 (not drawn) to 255 (opaque) */


struct VertexTag
{
	int X;
	int Y;
	int U;
	int V;
};

typedef struct
{
	int ImageNumber;

	int TopLeftX;
	int TopLeftY;
	
	int TopLeftU;
	int TopLeftV;

	int Width;
	int Height;

	int Scale;

	BOOL filterTexture;

	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
	unsigned char Translucency; /* 0 == invisible, 255 == opaque */

} HUDImageDesc;


typedef struct
{
	char Character;
	int X;
	int Y;

	unsigned char Red;
	unsigned char Green;
	unsigned char Blue;
	unsigned char Alpha;

} HUDCharDesc;


extern void D3D_DrawHUDFontCharacter(HUDCharDesc *charDescPtr);
extern void D3D_DrawHUDDigit(HUDCharDesc *charDescPtr);
void D3D_BLTMotionTrackerBlipToHUD(int x, int y, int brightness);
void D3D_BLTGunSightToHUD(int screenX, int screenY, enum GUNSIGHT_SHAPE gunsightShape);
void D3D_DrawHUDPredatorDigit(HUDCharDesc *charDescPtr, int scale);
void D3D_BLTMotionTrackerToHUD(int scanLineSize);
void Draw_HUDImage(HUDImageDesc *imageDescPtr);
extern void D3D_BlitWhiteChar(int x, int y, unsigned char c);

/* KJL 17:55:48 18/04/98 - called by HUD.c */
extern void Render_HealthAndArmour(unsigned int health, unsigned int armour);
extern void Render_MarineAmmo(enum TEXTSTRING_ID ammoText, enum TEXTSTRING_ID magazinesText, unsigned int magazines, enum TEXTSTRING_ID roundsText, unsigned int rounds, int primaryAmmo);
