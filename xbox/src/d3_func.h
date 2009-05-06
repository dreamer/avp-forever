#ifndef _included_d3_func_h_
#define _included_d3_func_h_

#ifdef __cplusplus

extern "C" {

#endif

#ifdef WIN32
	#include <d3d9.h>
	#include "Dxerr9.h"

	#pragma comment(lib, "Dxerr9.lib")
#endif

#ifdef _XBOX
	#include <xtl.h>
#endif

#include "aw.h"
/*
  Direct3D globals
*/

/* 
  Maximum number of Direct3D drivers ever
  expected to be resident on the system.
*/
#define MAX_D3D_DRIVERS 5
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
    char Name[30]; /* short name of driver */
	char About[50]; /* string about driver */
	BOOL Hardware; /* accelerated driver? */
	BOOL Textures; /* Texture mapping available? */
	BOOL ZBuffer; /* Z Buffering available? */
} D3DDRIVERINFO;

/*
  Description of a D3D driver texture 
  format.
*/

typedef struct D3DTextureFormat {
    BOOL Palette;   /* is Palettized? */
    int RedBPP;         /* #red bits per pixel */
    int BlueBPP;        /* #blue bits per pixel */
    int GreenBPP;       /* #green bits per pixel */
    int IndexBPP;       /* number of bits in palette index */
} D3DTEXTUREFORMAT;


typedef struct D3DInfo {
    LPDIRECT3D8				lpD3D;
    LPDIRECT3DDEVICE8		lpD3DDevice; 
    D3DVIEWPORT8			lpD3DViewport; 
	LPDIRECT3DSURFACE8		lpD3DBackSurface;// back buffer surface
	D3DSURFACE_DESC			BackSurface_desc; // back buffer surface description
	D3DPRESENT_PARAMETERS	d3dpp;

	LPDIRECT3DVERTEXBUFFER8 lpD3DVertexBuffer;
	LPDIRECT3DINDEXBUFFER8	lpD3DIndexBuffer;
    int						NumDrivers;
    int						CurrentDriver;
	D3DADAPTER_IDENTIFIER8	AdapterInfo;
	int						NumModes;
	D3DDISPLAYMODE			DisplayMode[100];
	D3DFORMAT				Formats[20];
    int						CurrentTextureFormat;
    int						NumTextureFormats;
} D3DINFO;

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

typedef struct
{
	enum TRANSLUCENCY_TYPE TranslucencyMode;
	enum FILTERING_MODE_ID FilteringMode;
	int FogDistance;
	unsigned int FogIsOn :1;
	unsigned int WireFrameModeIsOn :1;

} RENDERSTATES;

LPDIRECT3DTEXTURE8 CreateD3DTexture(AvPTexture *tex, unsigned char *buf, D3DPOOL poolType);
//LPDIRECT3DSURFACE8 CreateD3DSurface(DDSurface *tex, int width, int height);
LPDIRECT3DTEXTURE8 CreateD3DTexturePadded(AvPTexture *tex, int *real_height, int *real_width);
LPDIRECT3DTEXTURE8 CreateD3DTallFontTexture(AvPTexture *tex);

BOOL ReleaseVolatileResources();
BOOL CreateVolatileResources();
BOOL ChangeGameResolution(int width, int height, int colour_depth);

//void DrawMenuQuad(int topX, int topY, int bottomX, int bottomY, int image_num, BOOL alpha);
void DrawAlphaMenuQuad(int topX, int topY, int bottomX, int bottomY, int image_num, int alpha);
void DrawTallFontCharacter(int topX, int topY, int texU, int texV, int char_width, int alpha);
void DrawCloudTable(int topX, int topY, int word_length, int alpha);
void DrawFadeQuad(int topX, int topY, int alpha);
void DrawSmallMenuCharacter(int topX, int topY, int texU, int texV, int red, int green, int blue, int alpha);
//void DrawTexturedFadedQuad(int topX, int topY, int image_num, int alpha);
void DrawProgressBar(RECT src_rect, RECT dest_rect, D3DTEXTURE bar_texture, int original_width, int original_height, int new_width, int new_height);
void SetFilteringMode(enum FILTERING_MODE_ID filteringRequired);
//void LogDxError(HRESULT hr);
//void LogDebugValue(int value);
void ReleaseD3DTexture(D3DTEXTURE d3dTexture);
void DrawBinkFmv(int topX, int topY, int height, int width, D3DTEXTURE fmvTexture);
void CreateScreenShotImage();
D3DTEXTURE CheckAndLoadUserTexture(const char *fileName, int *width, int *height);
D3DTEXTURE CreateFmvTexture(int width, int height, int usage, int pool);

D3DINFO GetD3DInfo();
void InGameFlipBuffers(void);
char* GetDeviceName();
#define RGB_MAKE	D3DCOLOR_XRGB

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

#ifdef __cplusplus

}
#endif

#endif /* ! _included_d3_func_h_ */
