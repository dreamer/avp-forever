#include "3dc.h"
#include "module.h"
#include "stratdef.h"
#include "gamedef.h"
#include "bh_types.h"
#include "hudgfx.h"
#include "huddefs.h"
#include "kshape.h"
#include "chnktexi.h"
#include "HUD_layout.h"
#include "language.h"
#include "tables.h"
#include "renderer.h"
#include "r2base.h"
#include "chnkload.hpp" // c++ header which ignores class definitions/member functions if __cplusplus is not defined ?
#include "d3d_hud.h"
#define UseLocalAssert FALSE
#include "ourasert.h"
#include "vision.h"
#include "kshape.h"

extern void D3D_RenderHUDString_Centred(char *stringPtr, uint32_t centreX, uint32_t y, uint32_t colour);
extern void D3D_RenderHUDNumber_Centred(uint32_t number, uint32_t x, uint32_t y, uint32_t colour);

#define RGBLIGHT_MAKE(rr,gg,bb) \
( \
	LCCM_NORMAL == d3d_light_ctrl.ctrl ? \
		RGB_MAKE(rr, gg, bb) \
	: LCCM_CONSTCOLOUR == d3d_light_ctrl.ctrl ? \
		RGB_MAKE(MUL_FIXED(rr, d3d_light_ctrl.r), MUL_FIXED(gg, d3d_light_ctrl.g), MUL_FIXED(bb, d3d_light_ctrl.b)) \
	: \
		RGB_MAKE(d3d_light_ctrl.GetR(rr), d3d_light_ctrl.GetG(gg), d3d_light_ctrl.GetB(bb)) \
)
#define RGBALIGHT_MAKE(rr,gg,bb,aa) \
( \
		RGBA_MAKE(rr,gg,bb,aa) \
)

void D3D_DrawHUDFontCharacter(HUDCharDesc *charDescPtr);
void D3D_DrawHUDDigit(HUDCharDesc *charDescPtr);

extern void YClipMotionTrackerVertices(struct VertexTag *v1, struct VertexTag *v2);
/* HUD globals */
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern char LevelName[];
signed int HUDTranslucencyLevel = 64;
static int MotionTrackerHalfWidth;
static int MotionTrackerTextureSize;
static int MotionTrackerCentreY;
static int MotionTrackerCentreX;
static int MT_BlipHeight;
static int MT_BlipWidth;
static HUDImageDesc BlueBar;

texID_t HUDImageNumber;
texID_t SpecialFXImageNumber;
texID_t SmokyImageNumber;
texID_t ChromeImageNumber;
texID_t CloudyImageNumber;
texID_t BurningImageNumber;
texID_t HUDFontsImageNumber;
texID_t PredatorVisionChangeImageNumber;
texID_t PredatorNumbersImageNumber;
texID_t StaticImageNumber;
texID_t AlienTongueImageNumber;
texID_t AAFontImageNumber;
texID_t WaterShaftImageNumber;

fixed_t HUDScaleFactor;
fixed_t MotionTrackerScale;

float HUDScaleFactorf = 1.0f;
float MotionTrackerScalef = 1.0f;

const char *cl_pszGameMode = NULL;

static struct HUDFontDescTag HUDFontDesc[] =
{
	//MARINE_HUD_FONT_BLUE,
	{
		225,//XOffset
		24,//Height
		16,//Width
	},
	//MARINE_HUD_FONT_RED,
	{
		242,//XOffset
		24,//Height
		14,//Width
	},
	//MARINE_HUD_FONT_MT_SMALL,
	{
		232,//XOffset
		12,//Height
		8,//Width
	},
	//MARINE_HUD_FONT_MT_BIG,
	{
		241,//XOffset
		24,//Height
		14,//Width
	},
};

#define BLUE_BAR_WIDTH ((203-0)+1)
#define BLUE_BAR_HEIGHT ((226-195)+1)

void D3D_BLTDigitToHUD(char digit, int x, int y, int font);

void LoadCommonTextures(void);
void D3D_InitialiseMarineHUD(void);

void PlatformSpecificInitMarineHUD()
{
	D3D_InitialiseMarineHUD();
	LoadCommonTextures();
}

void PlatformSpecificInitAlienHUD()
{
	/* set game mode: different, though for multiplayer game */
	if (AvP.Network == I_No_Network)
	{
		cl_pszGameMode = "alien";
		LoadCommonTextures();
	}
	else
	{
		cl_pszGameMode = "multip";
		/* load in sfx */
		LoadCommonTextures();
		// load marine stuff as well
		D3D_InitialiseMarineHUD();
	}
}

void PlatformSpecificInitPredatorHUD()
{
	/* set game mode: different, though for multiplayer game */
	if (AvP.Network == I_No_Network)
	{
		cl_pszGameMode = "predator";
		/* load in sfx */
		LoadCommonTextures();
	}
	else
	{
		cl_pszGameMode = "multip";
		/* load in sfx */
		LoadCommonTextures();
		//load marine stuff as well
		D3D_InitialiseMarineHUD();
	}
}

void Draw_HUDImage(HUDImageDesc *imageDescPtr)
{
	int scaledWidth;
	int scaledHeight;

	if (imageDescPtr->Scale == ONE_FIXED)
	{
		scaledWidth = imageDescPtr->Width;
		scaledHeight = imageDescPtr->Height;
	}
	else
	{
		scaledWidth = MUL_FIXED(imageDescPtr->Scale, imageDescPtr->Width);
		scaledHeight = MUL_FIXED(imageDescPtr->Scale, imageDescPtr->Height);
	}

	VertexTag quadVertices[4];

	quadVertices[0].U = imageDescPtr->TopLeftU;
	quadVertices[0].V = imageDescPtr->TopLeftV;
	quadVertices[1].U = imageDescPtr->TopLeftU + imageDescPtr->Width;
	quadVertices[1].V = imageDescPtr->TopLeftV;
	quadVertices[2].U = imageDescPtr->TopLeftU + imageDescPtr->Width;
	quadVertices[2].V = imageDescPtr->TopLeftV + imageDescPtr->Height;
	quadVertices[3].U = imageDescPtr->TopLeftU;
	quadVertices[3].V = imageDescPtr->TopLeftV + imageDescPtr->Height;
	
	quadVertices[0].X = imageDescPtr->TopLeftX;
	quadVertices[0].Y = imageDescPtr->TopLeftY;
	quadVertices[1].X = imageDescPtr->TopLeftX + scaledWidth;
	quadVertices[1].Y = imageDescPtr->TopLeftY;
	quadVertices[2].X = imageDescPtr->TopLeftX + scaledWidth;
	quadVertices[2].Y = imageDescPtr->TopLeftY + scaledHeight;
	quadVertices[3].X = imageDescPtr->TopLeftX;
	quadVertices[3].Y = imageDescPtr->TopLeftY + scaledHeight;

	D3D_HUDQuad_Output(imageDescPtr->ImageNumber, quadVertices, RGBALIGHT_MAKE
		(
			imageDescPtr->Red,
			imageDescPtr->Green,
			imageDescPtr->Blue,
			imageDescPtr->Translucency
		),
		imageDescPtr->filterTexture	? FILTERING_BILINEAR_ON : FILTERING_BILINEAR_OFF);
}


void D3D_InitialiseMarineHUD(void)
{
	/* set game mode: different though for multiplayer game */
	if (AvP.Network == I_No_Network)
	{
		cl_pszGameMode = "marine";
	}
	else
	{
		cl_pszGameMode = "multip";
	}

	/* load HUD gfx of correct resolution */
	{
		HUDImageNumber = Tex_CreateFromRIM("graphics/HUDs/Marine/MarineHUD.RIM");

		MotionTrackerHalfWidth   = 127/2;
		MotionTrackerTextureSize = 128;

		BlueBar.ImageNumber = HUDImageNumber;
		BlueBar.TopLeftX = ScreenDescriptorBlock.SDB_SafeZoneWidthOffset;//0; bjd
		BlueBar.TopLeftY = (ScreenDescriptorBlock.SDB_Height-40) - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset;
		BlueBar.TopLeftU = 1;
		BlueBar.TopLeftV = 223;
		BlueBar.Red   = 255;
		BlueBar.Green = 255;
		BlueBar.Blue  = 255;

		BlueBar.Height = BLUE_BAR_HEIGHT;
		BlueBar.Width  = BLUE_BAR_WIDTH;

		/* motion tracker blips */
		MT_BlipHeight = 12;
		MT_BlipWidth  = 12;

		/* load in sfx */
		SpecialFXImageNumber = Tex_CreateFromRIM("graphics/Common/partclfx.RIM");
	}

	/* centre of motion tracker */
	MotionTrackerCentreY = BlueBar.TopLeftY;
	MotionTrackerCentreX = BlueBar.TopLeftX + (BlueBar.Width/2);
	MotionTrackerScale  = ONE_FIXED;
	MotionTrackerScalef = 1.0f;

	HUDScaleFactor = DIV_FIXED(ScreenDescriptorBlock.SDB_Width, 640);
	HUDScaleFactorf = (float)ScreenDescriptorBlock.SDB_Width / 640.0f;

	#if UseGadgets
//	MotionTrackerGadget::SetCentre(r2pos(100,100));
	#endif
}

void LoadCommonTextures(void)
{
	if (AvP.Network == I_No_Network)
	{
		switch (AvP.PlayerType)
		{
			case I_Predator:
			{
				PredatorNumbersImageNumber = Tex_CreateFromRIM("graphics/HUDs/Predator/prednumbers.RIM");
				StaticImageNumber = Tex_CreateFromRIM("graphics/Common/static.RIM");
				break;
			}
			case I_Alien:
			{
				AlienTongueImageNumber = Tex_CreateFromRIM("graphics/HUDs/Alien/AlienTongue.RIM");
				break;
			}
			case I_Marine:
			{
				StaticImageNumber = Tex_CreateFromRIM("graphics/Common/static.RIM");
				break;
			}
			default:
				break;
		}
	}
	else
	{
		PredatorNumbersImageNumber = Tex_CreateFromRIM("graphics/HUDs/Predator/prednumbers.RIM");
		StaticImageNumber          = Tex_CreateFromRIM("graphics/Common/static.RIM");
		AlienTongueImageNumber     = Tex_CreateFromRIM("graphics/HUDs/Alien/AlienTongue.RIM");
	}
	
	HUDFontsImageNumber  = Tex_CreateFromRIM("graphics/Common/HUDfonts.RIM");
	SpecialFXImageNumber = Tex_CreateFromRIM("graphics/Common/partclfx.RIM");
	CloudyImageNumber    = Tex_CreateFromRIM("graphics/Common/cloudy.RIM");
	BurningImageNumber   = Tex_CreateFromRIM("graphics/Common/burn.RIM");

	if (!strcmp(LevelName,"invasion_a"))
	{
		ChromeImageNumber = Tex_CreateFromRIM("graphics/Envrnmts/Invasion/water2.RIM");
		WaterShaftImageNumber = Tex_CreateFromRIM("graphics/Envrnmts/Invasion/water-shaft.RIM");
	}
	else if (!strcmp(LevelName,"genshd1"))
	{
		WaterShaftImageNumber = Tex_CreateFromRIM("graphics/Envrnmts/GenShd1/colonywater.RIM");
	}
	else if (!strcmp(LevelName,"fall") || !strcmp(LevelName,"fall_m"))
	{
		ChromeImageNumber = Tex_CreateFromRIM("graphics/Envrnmts/fall/stream_water.RIM");
	}
	else if (!strcmp(LevelName,"derelict"))
	{
		ChromeImageNumber = Tex_CreateFromRIM("graphics/Envrnmts/Derelict/water.RIM");
	}
}

void YClipMotionTrackerVertices(struct VertexTag *v1, struct VertexTag *v2)
{
	char vertex1Inside=0,vertex2Inside=0;

	if (v1->Y<0) vertex1Inside = 1;
	if (v2->Y<0) vertex2Inside = 1;

	/* if both vertices inside clip region no clipping required */
	if (vertex1Inside && vertex2Inside) return;

	/* if both vertices outside clip region no action required 
	(the other lines will be clipped) */
	if (!vertex1Inside && !vertex2Inside) return;

	/* okay, let's clip */
	if (vertex1Inside)
	{
		int lambda = DIV_FIXED(v1->Y,v2->Y - v1->Y);

		v2->X = v1->X - MUL_FIXED(v2->X - v1->X,lambda);
		v2->Y=0;

		v2->U = v1->U - MUL_FIXED(v2->U - v1->U,lambda);
		v2->V = v1->V - MUL_FIXED(v2->V - v1->V,lambda);
	}
	else
	{
		int lambda = DIV_FIXED(v2->Y,v1->Y - v2->Y);

		v1->X = v2->X - MUL_FIXED(v1->X - v2->X,lambda);
		v1->Y=0;

		v1->U = v2->U - MUL_FIXED(v1->U - v2->U,lambda);
		v1->V = v2->V - MUL_FIXED(v1->V - v2->V,lambda);
	}
}

void D3D_BLTMotionTrackerToHUD(int scanLineSize)
{
	BlueBar.TopLeftY = (ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_SafeZoneHeightOffset) - MUL_FIXED(MotionTrackerScale, 40);
	MotionTrackerCentreY = BlueBar.TopLeftY;
	MotionTrackerCentreX = BlueBar.TopLeftX+MUL_FIXED(MotionTrackerScale, (BlueBar.Width/2));
	BlueBar.Scale = MotionTrackerScale;

	int motionTrackerScaledHalfWidth = MUL_FIXED(MotionTrackerScale*3, MotionTrackerHalfWidth/2);

	int angle = 4095 - Player->ObEuler.EulerY;

	int widthCos = MUL_FIXED(motionTrackerScaledHalfWidth, GetCos(angle));
	int widthSin = MUL_FIXED(motionTrackerScaledHalfWidth, GetSin(angle));

	/* I've put these -1s in here to help clipping 45 degree cases,
	where two vertices can end up around the clipping line of Y=0 */

	VertexTag quadVertices[4];

	// top left
	quadVertices[0].X = (-widthCos - (-widthSin));
	quadVertices[0].Y = (-widthSin + (-widthCos)) -1;
	quadVertices[0].U = 1;
	quadVertices[0].V = 1;
	
	// top right
	quadVertices[1].X = (widthCos - (-widthSin));
	quadVertices[1].Y = (widthSin + (-widthCos)) -1;
	quadVertices[1].U = 1 + MotionTrackerTextureSize;
	quadVertices[1].V = 1;

	// bottom right
	quadVertices[2].X = (widthCos - widthSin);
	quadVertices[2].Y = (widthSin + widthCos) -1;
	quadVertices[2].U = 1 + MotionTrackerTextureSize;
	quadVertices[2].V = 1 + MotionTrackerTextureSize;

	// bottom left
	quadVertices[3].X = ((-widthCos) - widthSin);
	quadVertices[3].Y = ((-widthSin) + widthCos) -1;
	quadVertices[3].U = 1;
	quadVertices[3].V = 1 + MotionTrackerTextureSize;

	/* clip to Y<=0 */
	YClipMotionTrackerVertices(&quadVertices[0], &quadVertices[1]);
	YClipMotionTrackerVertices(&quadVertices[1], &quadVertices[2]);
	YClipMotionTrackerVertices(&quadVertices[2], &quadVertices[3]);
	YClipMotionTrackerVertices(&quadVertices[3], &quadVertices[0]);

	/* translate into screen coords */
	quadVertices[0].X += MotionTrackerCentreX;
	quadVertices[1].X += MotionTrackerCentreX;
	quadVertices[2].X += MotionTrackerCentreX;
	quadVertices[3].X += MotionTrackerCentreX;
	quadVertices[0].Y += MotionTrackerCentreY;
	quadVertices[1].Y += MotionTrackerCentreY;
	quadVertices[2].Y += MotionTrackerCentreY;
	quadVertices[3].Y += MotionTrackerCentreY;
	
	/* dodgy offset 'cos I'm not x clipping */
	if (quadVertices[0].X==-1) quadVertices[0].X = 0;
	if (quadVertices[1].X==-1) quadVertices[1].X = 0;
	if (quadVertices[2].X==-1) quadVertices[2].X = 0;
	if (quadVertices[3].X==-1) quadVertices[3].X = 0;

	/* check u & v are >0 */
	if (quadVertices[0].V<0) quadVertices[0].V = 0;
	if (quadVertices[1].V<0) quadVertices[1].V = 0;
	if (quadVertices[2].V<0) quadVertices[2].V = 0;
	if (quadVertices[3].V<0) quadVertices[3].V = 0;

	if (quadVertices[0].U<0) quadVertices[0].U = 0;
	if (quadVertices[1].U<0) quadVertices[1].U = 0;
	if (quadVertices[2].U<0) quadVertices[2].U = 0;
	if (quadVertices[3].U<0) quadVertices[3].U = 0;

	D3D_HUDQuad_Output
	(
		HUDImageNumber, 
		quadVertices,
		RGBALIGHT_MAKE(255,255,255,HUDTranslucencyLevel),
		FILTERING_BILINEAR_ON
	);

	{
		HUDImageDesc imageDesc;

		imageDesc.ImageNumber = HUDImageNumber;
		imageDesc.Scale = MUL_FIXED(MotionTrackerScale*3, scanLineSize/2);
		imageDesc.TopLeftX = MotionTrackerCentreX - MUL_FIXED(motionTrackerScaledHalfWidth, scanLineSize);
		imageDesc.TopLeftY = MotionTrackerCentreY - MUL_FIXED(motionTrackerScaledHalfWidth, scanLineSize);
		imageDesc.TopLeftU = 1;
		imageDesc.TopLeftV = 132;
		imageDesc.Height = 64;
		imageDesc.Width  = 128;
		imageDesc.Red    = 255;
		imageDesc.Green  = 255;
		imageDesc.Blue   = 255;
		imageDesc.Translucency  = HUDTranslucencyLevel;
		imageDesc.filterTexture = TRUE;

		Draw_HUDImage(&imageDesc);
	}

	/* KJL 16:14:29 30/01/98 - draw bottom bar of MT */
	BlueBar.Translucency = HUDTranslucencyLevel;
	Draw_HUDImage(&BlueBar);
	
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_UNITS],      17, -4, MARINE_HUD_FONT_MT_SMALL);
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_TENS],        9, -4, MARINE_HUD_FONT_MT_SMALL);
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_HUNDREDS],   -9, -4, MARINE_HUD_FONT_MT_BIG);
	D3D_BLTDigitToHUD(ValueOfHUDDigit[MARINE_HUD_MOTIONTRACKER_THOUSANDS], -25, -4, MARINE_HUD_FONT_MT_BIG);
}


void D3D_BLTMotionTrackerBlipToHUD(int x, int y, int brightness)
{
	HUDImageDesc imageDesc;

	int motionTrackerScaledHalfWidth = MUL_FIXED(MotionTrackerScale*3, MotionTrackerHalfWidth/2);

	GLOBALASSERT(brightness<=65536);
	
	int frame = (brightness*5)/65537;
	GLOBALASSERT(frame>=0 && frame<5);
	
	frame = 4 - frame; // frames bloody wrong way round
	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.Scale = MUL_FIXED(MotionTrackerScale*3,(brightness+ONE_FIXED)/4);
	imageDesc.TopLeftX = MotionTrackerCentreX - MUL_FIXED(MT_BlipWidth/2,imageDesc.Scale) + MUL_FIXED(x,motionTrackerScaledHalfWidth);
	imageDesc.TopLeftY = MotionTrackerCentreY - MUL_FIXED(MT_BlipHeight/2,imageDesc.Scale) - MUL_FIXED(y,motionTrackerScaledHalfWidth);
	imageDesc.TopLeftU = 227;
	imageDesc.TopLeftV = 187;
	imageDesc.Height = MT_BlipHeight;
	imageDesc.Width = MT_BlipWidth;

	int trans = MUL_FIXED(brightness*2, HUDTranslucencyLevel);
	if (trans > 255)
		trans = 255;
	imageDesc.Translucency = trans;

	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;
	imageDesc.filterTexture = TRUE;

	if (imageDesc.TopLeftX < 0) /* then we need to clip */
	{
		imageDesc.Width += imageDesc.TopLeftX;
		imageDesc.TopLeftU -= imageDesc.TopLeftX;
		imageDesc.TopLeftX = 0;
	}

	Draw_HUDImage(&imageDesc);
}

extern void D3D_BlitWhiteChar(int x, int y, unsigned char c)
{
	HUDImageDesc imageDesc;
	
//	if (c>='a' && c<='z') c-='a'-'A';

//	if (c<' ' || c>'_') return;
	if (c==' ') return;

	imageDesc.ImageNumber = AAFontImageNumber;

	imageDesc.TopLeftX = x;
	imageDesc.TopLeftY = y;
	imageDesc.TopLeftU = 1+((c-32)&15)*16;
	imageDesc.TopLeftV = 1+((c-32)>>4)*16;
	imageDesc.Height = 15;
	imageDesc.Width = 15;

	imageDesc.Scale = ONE_FIXED; //DIV_FIXED(640, ScreenDescriptorBlock.SDB_Width);//ONE_FIXED;
	imageDesc.Translucency = 255;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;
	imageDesc.filterTexture = FALSE;

	Draw_HUDImage(&imageDesc);
}

void D3D_DrawHUDFontCharacter(HUDCharDesc *charDescPtr)
{
	HUDImageDesc imageDesc;

//	if (charDescPtr->Character<' ' || charDescPtr->Character>'_') return;
	if (charDescPtr->Character == ' ') return;

	imageDesc.ImageNumber = AAFontImageNumber;

	imageDesc.TopLeftX = charDescPtr->X-1;
	imageDesc.TopLeftY = charDescPtr->Y-1;
	imageDesc.TopLeftU = 0+((charDescPtr->Character-32)&15)*16;
	imageDesc.TopLeftV = 0+((charDescPtr->Character-32)>>4)*16;
	imageDesc.Height = HUD_FONT_HEIGHT+2;
	imageDesc.Width = HUD_FONT_WIDTH+2;

	imageDesc.Scale = ONE_FIXED;
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;
	imageDesc.filterTexture = FALSE;

	Draw_HUDImage(&imageDesc);
}

void D3D_DrawHUDDigit(HUDCharDesc *charDescPtr)
{
	HUDImageDesc imageDesc;

	imageDesc.ImageNumber = HUDFontsImageNumber;

	imageDesc.TopLeftX = charDescPtr->X;
	imageDesc.TopLeftY = charDescPtr->Y;

	if (charDescPtr->Character<8)
	{
		imageDesc.TopLeftU = 1+(charDescPtr->Character)*16;
		imageDesc.TopLeftV = 81;
	}
	else
	{
		imageDesc.TopLeftU = 1+(charDescPtr->Character-8)*16;
		imageDesc.TopLeftV = 81+24;
	}

	imageDesc.Height = HUD_DIGITAL_NUMBERS_HEIGHT;
	imageDesc.Width = HUD_DIGITAL_NUMBERS_WIDTH;
	imageDesc.Scale = ONE_FIXED;
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;
	imageDesc.filterTexture = FALSE;

	Draw_HUDImage(&imageDesc);
}

void D3D_DrawHUDPredatorDigit(HUDCharDesc *charDescPtr, int scale)
{
	HUDImageDesc imageDesc;

	imageDesc.ImageNumber = PredatorNumbersImageNumber;

	/* bjd - 24/11/08 - this function draws the red and blue predator energy bars
	** the red bar is on the left side of the screen, the blue on the right
	** both bars are the same distance down from the top of the screen (same y co-ord)
	** for tvs, need to add some distance from top to move into tv safe zone and then 
	** need to add or remove space depending on which side of screen the energy bar is on */
	
	/* if x is in the first half of screen, its the red bar..offset towards centre of screen */
	if (charDescPtr->X <= ScreenDescriptorBlock.SDB_Width / 2)
	{
		imageDesc.TopLeftX = charDescPtr->X + ScreenDescriptorBlock.SDB_SafeZoneWidthOffset;
	}
	/* we're on the right side of the screen, minus some offset to move us back left a bit */
	else
	{
		imageDesc.TopLeftX = charDescPtr->X - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset;
	}

	/* y will be the same for both bars, need to add some offset */
	imageDesc.TopLeftY = charDescPtr->Y + ScreenDescriptorBlock.SDB_SafeZoneWidthOffset;

//	imageDesc.TopLeftX = charDescPtr->X;
//	imageDesc.TopLeftY = charDescPtr->Y;

	if (charDescPtr->Character<5)
	{
		imageDesc.TopLeftU = (charDescPtr->Character)*51;
		imageDesc.TopLeftV = 1;
	}
	else
	{
		imageDesc.TopLeftU = (charDescPtr->Character-5)*51;
		imageDesc.TopLeftV = 52;
	}

	imageDesc.Height = 50;
	imageDesc.Width = 50;
	imageDesc.Scale = scale;
	imageDesc.Translucency = charDescPtr->Alpha;
	imageDesc.Red = charDescPtr->Red;
	imageDesc.Green = charDescPtr->Green;
	imageDesc.Blue = charDescPtr->Blue;
	imageDesc.filterTexture = TRUE;

	Draw_HUDImage(&imageDesc);
}

void D3D_BLTDigitToHUD(char digit, int x, int y, int font)
{
	HUDImageDesc imageDesc;
	struct HUDFontDescTag *FontDescPtr;
	int gfxID;

	switch (font)
	{
		case MARINE_HUD_FONT_MT_SMALL:
		case MARINE_HUD_FONT_MT_BIG:
		{
			gfxID = MARINE_HUD_GFX_TRACKERFONT;
			imageDesc.Scale = MotionTrackerScale;
			x = MUL_FIXED(x, MotionTrackerScale) + MotionTrackerCentreX;
			y = MUL_FIXED(y, MotionTrackerScale) + MotionTrackerCentreY;
			break;
		}
		case MARINE_HUD_FONT_RED:
		case MARINE_HUD_FONT_BLUE:
		{
			if (x<0) x+=ScreenDescriptorBlock.SDB_Width;
			gfxID = MARINE_HUD_GFX_NUMERALS;
			imageDesc.Scale=ONE_FIXED;
			break;
		}
		case ALIEN_HUD_FONT:
		{
			gfxID = ALIEN_HUD_GFX_NUMBERS;
			imageDesc.Scale=ONE_FIXED;
			break;
		}
		default:
			LOCALASSERT(0);
			break;
	}

	FontDescPtr = &HUDFontDesc[font];

	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.TopLeftX = x;
	imageDesc.TopLeftY = y;
	imageDesc.TopLeftU = FontDescPtr->XOffset;
	imageDesc.TopLeftV = digit*(FontDescPtr->Height+1)+1;
	
	imageDesc.Height = FontDescPtr->Height;
	imageDesc.Width = FontDescPtr->Width;
	imageDesc.Translucency = HUDTranslucencyLevel;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;
	imageDesc.filterTexture = FALSE;

	Draw_HUDImage(&imageDesc);
}

void D3D_BLTGunSightToHUD(int screenX, int screenY, enum GUNSIGHT_SHAPE gunsightShape)
{
	HUDImageDesc imageDesc;
	int gunsightSize=13;

	screenX = (screenX-(gunsightSize/2));
	screenY = (screenY-(gunsightSize/2));
	
	imageDesc.ImageNumber = HUDImageNumber;
	imageDesc.TopLeftX = screenX;
	imageDesc.TopLeftY = screenY;
	imageDesc.TopLeftU = 227;
	imageDesc.TopLeftV = 131+(gunsightShape*(gunsightSize+1));
	imageDesc.Height = gunsightSize;
	imageDesc.Width  = gunsightSize;
	imageDesc.Scale  = ONE_FIXED;
	imageDesc.Translucency = 128;
	imageDesc.Red = 255;
	imageDesc.Green = 255;
	imageDesc.Blue = 255;
	imageDesc.filterTexture = FALSE;

	Draw_HUDImage(&imageDesc);
}

void Render_HealthAndArmour(unsigned int health, unsigned int armour)
{
	unsigned int healthColour;
	unsigned int armourColour;

	if (AvP.PlayerType == I_Marine)
	{
		int xCentre = MUL_FIXED(HUDLayout_RightmostTextCentre, HUDScaleFactor)+ScreenDescriptorBlock.SDB_Width - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset;
		healthColour = HUDLayout_Colour_MarineGreen;
		armourColour = HUDLayout_Colour_MarineGreen;

		D3D_RenderHUDString_Centred
		(
			GetTextString(TEXTSTRING_INGAME_HEALTH),
			xCentre,
			MUL_FIXED(HUDLayout_Health_TopY,HUDScaleFactor) + ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
			HUDLayout_Colour_BrightWhite
		);
		D3D_RenderHUDNumber_Centred
		(
			health,
			xCentre,
			MUL_FIXED(HUDLayout_Health_TopY+HUDLayout_Linespacing,HUDScaleFactor) + ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
			healthColour
		);
		D3D_RenderHUDString_Centred
		(
			GetTextString(TEXTSTRING_INGAME_ARMOUR),
			xCentre,
			MUL_FIXED(HUDLayout_Armour_TopY,HUDScaleFactor) + ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
			HUDLayout_Colour_BrightWhite
		);
		D3D_RenderHUDNumber_Centred
		(
			armour,
			xCentre,
			MUL_FIXED(HUDLayout_Armour_TopY+HUDLayout_Linespacing,HUDScaleFactor) + ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
			armourColour
		);
	}
	else
	{
		if (health > 100)
		{
			healthColour = HUDLayout_Colour_BrightWhite;
		}
		else
		{
			int r = ((health)*128)/100;
			healthColour = 0xff000000 + ((128-r)<<16) + (r<<8);
		}
		if (armour>100)
		{
			armourColour = HUDLayout_Colour_BrightWhite;
		}
		else
		{
			int r = ((armour)*128)/100;
			armourColour = 0xff000000 + ((128-r)<<16) + (r<<8);
		}

		{
			struct VertexTag quadVertices[4];
			int scaledWidth;
			int scaledHeight;
			int x,y;

			if (health<100)
			{
				scaledWidth = WideMulNarrowDiv(ScreenDescriptorBlock.SDB_Width,health,100);
				scaledHeight = scaledWidth/32;
			}
			else
			{
				scaledWidth = ScreenDescriptorBlock.SDB_Width;
				scaledHeight = scaledWidth/32;
			}
			x = (ScreenDescriptorBlock.SDB_Width - scaledWidth)/2;
			y = (ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_Width/32 + x/32) - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset;

			quadVertices[0].U = 8;
			quadVertices[0].V = 5;

			quadVertices[1].U = 57;//255;
			quadVertices[1].V = 5;

			quadVertices[2].U = 57;//255;
			quadVertices[2].V = 55;//255;

			quadVertices[3].U = 8;
			quadVertices[3].V = 55;//255;
			
			quadVertices[0].X = x;
			quadVertices[0].Y = y;
			quadVertices[1].X = x + scaledWidth;
			quadVertices[1].Y = y;
			quadVertices[2].X = x + scaledWidth;
			quadVertices[2].Y = y + scaledHeight;
			quadVertices[3].X = x;
			quadVertices[3].Y = y + scaledHeight;

			D3D_HUDQuad_Output(SpecialFXImageNumber, quadVertices, 0xff003fff, FILTERING_BILINEAR_ON);

			health = (health / 2);
/*
			if (health < 0) 
				health = 0;
*/
			if (health<100)
			{
				scaledWidth = WideMulNarrowDiv(ScreenDescriptorBlock.SDB_Width,health,100);
				scaledHeight = scaledWidth/32;
			}
			else
			{
				scaledWidth = ScreenDescriptorBlock.SDB_Width;
				scaledHeight = scaledWidth/32;
			}

			x = (ScreenDescriptorBlock.SDB_Width - scaledWidth)/2;
			y = (ScreenDescriptorBlock.SDB_Height - ScreenDescriptorBlock.SDB_Width/32 + x/32) - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset;

			quadVertices[0].X = x;
			quadVertices[0].Y = y;
			quadVertices[1].X = x + scaledWidth;
			quadVertices[1].Y = y;
			quadVertices[2].X = x + scaledWidth;
			quadVertices[2].Y = y + scaledHeight;
			quadVertices[3].X = x;
			quadVertices[3].Y = y + scaledHeight;

			D3D_HUDQuad_Output(SpecialFXImageNumber, quadVertices, 0xffffffff, FILTERING_BILINEAR_ON);
		}
	}
}

void Render_MarineAmmo(enum TEXTSTRING_ID ammoText, enum TEXTSTRING_ID magazinesText, unsigned int magazines, enum TEXTSTRING_ID roundsText, unsigned int rounds, int primaryAmmo)
{
	int xCentre = (MUL_FIXED(HUDLayout_RightmostTextCentre,HUDScaleFactor)+ScreenDescriptorBlock.SDB_Width) - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset;

	if (!primaryAmmo)
		xCentre += MUL_FIXED(HUDScaleFactor, HUDLayout_RightmostTextCentre * 2);

	D3D_RenderHUDString_Centred
	(
		GetTextString(ammoText),
		xCentre,
		(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_AmmoDesc_TopY))  - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
		HUDLayout_Colour_BrightWhite
	);
	D3D_RenderHUDString_Centred
	(
		GetTextString(magazinesText),
		xCentre,
		(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor, HUDLayout_Magazines_TopY)) - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
		HUDLayout_Colour_BrightWhite
	);
	D3D_RenderHUDNumber_Centred
	(
		magazines,
		xCentre,
		(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_Magazines_TopY - HUDLayout_Linespacing)) - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
		HUDLayout_Colour_MarineRed
	);
	D3D_RenderHUDString_Centred
	(
		GetTextString(roundsText),
		xCentre,
		(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY)) - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
		HUDLayout_Colour_BrightWhite
	);
	D3D_RenderHUDNumber_Centred
	(
		rounds,
		xCentre,
		(ScreenDescriptorBlock.SDB_Height - MUL_FIXED(HUDScaleFactor,HUDLayout_Rounds_TopY - HUDLayout_Linespacing)) - ScreenDescriptorBlock.SDB_SafeZoneWidthOffset,
		HUDLayout_Colour_MarineRed
	);
}

void DrawPredatorEnergyBar(void)
{
	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);
	PLAYER_WEAPON_DATA *weaponPtr = &(playerStatusPtr->WeaponSlot[playerStatusPtr->SelectedWeaponSlot]);
	int maxHeight = ScreenDescriptorBlock.SDB_Height*3/4;
	int h;
	{
		h = MUL_FIXED(DIV_FIXED(playerStatusPtr->FieldCharge,PLAYERCLOAK_MAXENERGY),maxHeight);
		
		r2rect rectangle
		(
			ScreenDescriptorBlock.SDB_Width+HUDLayout_RightmostTextCentre*3/2,
			ScreenDescriptorBlock.SDB_Height-h,
			ScreenDescriptorBlock.SDB_Width+HUDLayout_RightmostTextCentre/2,
			ScreenDescriptorBlock.SDB_Height
			
		);

		rectangle.AlphaFill
		(
			0xff, // unsigned char R,
			0x00,// unsigned char G,
			0x00,// unsigned char B,
			128 // unsigned char translucency
		);
	}
	if (weaponPtr->WeaponIDNumber == WEAPON_PRED_SHOULDERCANNON)
	{
		h = MUL_FIXED(playerStatusPtr->PlasmaCasterCharge,maxHeight);

		r2rect rectangle
		(
			ScreenDescriptorBlock.SDB_Width+HUDLayout_RightmostTextCentre*3,
			ScreenDescriptorBlock.SDB_Height-h,
			ScreenDescriptorBlock.SDB_Width+HUDLayout_RightmostTextCentre*2,
			ScreenDescriptorBlock.SDB_Height
			
		);

		rectangle.AlphaFill
		(
			0x00, // unsigned char R,
			0xff,// unsigned char G,
			0xff,// unsigned char B,
			128 // unsigned char translucency
		);
	}
}

