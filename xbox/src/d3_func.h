#ifndef _included_d3_func_h_
#define _included_d3_func_h_

#ifdef __cplusplus
	extern "C" {
#endif

#include <xtl.h>
#include "aw.h"
#include "stdint.h"

/*
  Direct3D globals
*/

/* 
  Maximum number of Direct3D drivers ever
  expected to be resident on the system.
*/
#define MAX_D3D_DRIVERS 2
/*
  Maximum number of texture formats ever
  expected to be reported by a Direct3D
  driver.
*/
#define MAX_TEXTURE_FORMATS 10

/*
  Description of a D3D driver.
*/

typedef struct D3DDriverInfo {
	D3DFORMAT				Formats[20];
	D3DADAPTER_IDENTIFIER8	AdapterInfo;
	D3DDISPLAYMODE			DisplayMode[100];
	int						NumModes;
} D3DDRIVERINFO;


typedef struct D3DInfo {
    LPDIRECT3D8				lpD3D;
    LPDIRECT3DDEVICE8		lpD3DDevice; 
    D3DVIEWPORT8			D3DViewport; 
	D3DPRESENT_PARAMETERS	d3dpp;

	LPDIRECT3DVERTEXBUFFER8 lpD3DVertexBuffer;
	LPDIRECT3DINDEXBUFFER8	lpD3DIndexBuffer;

	LPDIRECT3DVERTEXBUFFER8 lpD3DOrthoVertexBuffer;

    int						NumDrivers;
    int						CurrentDriver;
	D3DDRIVERINFO			Driver[MAX_D3D_DRIVERS];
    int						CurrentTextureFormat;
    int						NumTextureFormats;
} D3DINFO;

extern D3DINFO d3d;

/* KJL 14:24:45 12/4/97 - render state information */
enum TRANSLUCENCY_TYPE
{
	TRANSLUCENCY_OFF,
	TRANSLUCENCY_NORMAL,
	TRANSLUCENCY_INVCOLOUR,
	TRANSLUCENCY_COLOUR,
	TRANSLUCENCY_GLOWING,
	TRANSLUCENCY_DARKENINGCOLOUR,
	TRANSLUCENCY_JUSTSETZ,
	TRANSLUCENCY_NOT_SET
};

enum FILTERING_MODE_ID
{
	FILTERING_BILINEAR_OFF,
	FILTERING_BILINEAR_ON,
	FILTERING_NOT_SET
};

enum TEXTURE_ADDRESS_MODE
{
	TEXTURE_WRAP,
	TEXTURE_CLAMP
};

typedef struct
{
	enum TRANSLUCENCY_TYPE TranslucencyMode;
	enum FILTERING_MODE_ID FilteringMode;
	enum TEXTURE_ADDRESS_MODE TextureAddressMode;
	int FogDistance;
	unsigned int FogIsOn :1;
	unsigned int WireFrameModeIsOn :1;

} RENDERSTATES;

LPDIRECT3DTEXTURE8 CreateD3DTexture(AVPTEXTURE *tex, uint8_t *buf, int usage, D3DPOOL poolType);
LPDIRECT3DTEXTURE8 CreateD3DTexturePadded(AVPTEXTURE *tex, int *real_height, int *real_width);
LPDIRECT3DTEXTURE8 CreateD3DTallFontTexture(AVPTEXTURE *tex);

BOOL ReleaseVolatileResources();
BOOL CreateVolatileResources();
BOOL ChangeGameResolution(int width, int height, int colour_depth);

void DrawAlphaMenuQuad(int topX, int topY, int image_num, int alpha);
void DrawTallFontCharacter(int topX, int topY, int texU, int texV, int char_width, int alpha);
void DrawCloudTable(int topX, int topY, int word_length, int alpha);
void DrawFadeQuad(int topX, int topY, int alpha);
void DrawQuad(int x, int y, int width, int height, int textureID, int colour, enum TRANSLUCENCY_TYPE translucencyType);
void DrawSmallMenuCharacter(int topX, int topY, int texU, int texV, int red, int green, int blue, int alpha);
void DrawProgressBar(RECT src_rect, RECT dest_rect, D3DTEXTURE bar_texture, int original_width, int original_height, int new_width, int new_height);
void SetFilteringMode(enum FILTERING_MODE_ID filteringRequired);
void ReleaseD3DTexture(D3DTEXTURE *d3dTexture);
void DrawFmvFrame(int frameWidth, int frameHeight, int textureWidth, int textureHeight, D3DTEXTURE fmvTexture);
void CreateScreenShotImage();
void DeRedTexture(D3DTEXTURE texture);
D3DTEXTURE CheckAndLoadUserTexture(const char *fileName, int *width, int *height);
D3DTEXTURE CreateFmvTexture(int *width, int *height, int usage, int pool);

D3DINFO GetD3DInfo();
void InGameFlipBuffers(void);
char* GetDeviceName();
#define RGB_MAKE	D3DCOLOR_XRGB

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#ifdef __cplusplus
}
#endif

#endif /* ! _included_d3_func_h_ */
