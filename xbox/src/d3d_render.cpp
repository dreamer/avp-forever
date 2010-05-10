#include "console.h"
#include "onscreenKeyboard.h"
#include "textureManager.h"

#include "r2base.h"
#include "font2.h"

// STL stuff
#include <vector>
#include <algorithm>

#include "logString.h"
#include "fmvCutscenes.h"

#include "logString.h"

DWORD mainDecl[] = 
{
	D3DVSD_STREAM(0),
		D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),    // position
		D3DVSD_REG( 1, D3DVSDT_D3DCOLOR ),  // diffuse color
		D3DVSD_REG( 2, D3DVSDT_D3DCOLOR ),  // specular color
		D3DVSD_REG( 3, D3DVSDT_FLOAT2 ),    // texture coordiantes
	D3DVSD_END()	
};

DWORD orthoDecl[] = 
{
	D3DVSD_STREAM(0),
		D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),    // position
		D3DVSD_REG( 1, D3DVSDT_D3DCOLOR ),  // diffuse color
		D3DVSD_REG( 2, D3DVSDT_FLOAT2 ),    // texture coordiantes
	D3DVSD_END()	
};

DWORD fmvDecl[] = 
{
	D3DVSD_STREAM(0),
		D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),    // position
		D3DVSD_REG( 1, D3DVSDT_FLOAT2 ),    // texture coordiantes 1
		D3DVSD_REG( 2, D3DVSDT_FLOAT2 ),    // texture coordiantes 2
		D3DVSD_REG( 3, D3DVSDT_FLOAT2 ),    // texture coordiantes 3
	D3DVSD_END()	
};


extern "C" {

#include "3dc.h"
#include "inline.h"
#include "gamedef.h"
#include "dxlog.h"
#include "d3_func.h"
#include "d3d_hud.h"
#include "particle.h"
#include "avp_menus.h"

#define UseLocalAssert FALSE
#include "ourasert.h"
#include <assert.h>

extern D3DINFO d3d;
extern uint32_t fov;

#include "HUD_layout.h"
#define HAVE_VISION_H 1
#include "lighting.h"
#include "showcmds.h"
#include "frustrum.h"
#include "d3d_render.h"
#include "avp_userprofile.h"
#include "bh_types.h"

const uint32_t MAX_VERTEXES = 4096;
const uint32_t MAX_INDICES = 9216;

D3DLVERTEX *mainVertex = NULL;
WORD *mainIndex = NULL;

ORTHOVERTEX *orthoVerts = NULL;
WORD *orthoIndex = NULL;

//POINTSPRITEVERTEX *testPS = NULL;
//uint32_t psIndex = 0;

static uint32_t orthoVBOffset = 0;
static uint32_t orthoIBOffset = 0;
static uint32_t orthoListCount = 0;

#pragma pack(push, 8)       // Make sure structure packing is set properly
#include <d3d8perf.h>
#pragma pack(pop)

struct RENDER_STATES
{
	int32_t		textureID;
	uint32_t	vertStart;
	uint32_t	vertEnd;
	uint32_t	indexStart;
	uint32_t	indexEnd;

	enum TRANSLUCENCY_TYPE translucencyType;
	enum FILTERING_MODE_ID filteringType;

	bool operator<(const RENDER_STATES& rhs) const {return textureID < rhs.textureID;}
//	bool operator<(const RENDER_STATES& rhs) const {return translucency_type < rhs.translucency_type;}
};

RENDER_STATES *renderList = new RENDER_STATES[MAX_VERTEXES];
std::vector<RENDER_STATES> renderTest;

struct ORTHO_OBJECTS
{
	int32_t		textureID;
	uint32_t	vertStart;
	uint32_t	vertEnd;

	uint32_t	indexStart;
	uint32_t	indexEnd;

	enum TRANSLUCENCY_TYPE translucencyType;
	enum FILTERING_MODE_ID filteringType;
	enum TEXTURE_ADDRESS_MODE textureAddressMode;
};

// array of 2d objects
ORTHO_OBJECTS *orthoList = new ORTHO_OBJECTS[MAX_VERTEXES]; // lower me!

struct renderParticle
{
	PARTICLE		particle;
	RENDERVERTEX	vertices[9];
	uint32_t		numVerts;
	int				translucency;

	inline bool operator<(const renderParticle& rhs) const {return translucency < rhs.translucency;}
};

std::vector<renderParticle> particleArray;

D3DXMATRIX viewMatrix;

void UpdateViewMatrix(float *viewMat)
{
	
	D3DXVECTOR3 vecRight	(viewMat[0], viewMat[1], viewMat[2]);
	D3DXVECTOR3 vecUp		(viewMat[4], viewMat[5], viewMat[6]);
	D3DXVECTOR3 vecFront	(viewMat[8], -viewMat[9], viewMat[10]);
	D3DXVECTOR3 vecPosition (viewMat[3], -viewMat[7], viewMat[11]);

	D3DXVec3Normalize(&vecFront, &vecFront);

	D3DXVec3Cross(&vecUp, &vecFront, &vecRight);
	D3DXVec3Normalize(&vecUp, &vecUp);

	D3DXVec3Cross(&vecRight, &vecUp, &vecFront);
	D3DXVec3Normalize(&vecRight, &vecRight);

	// right
	viewMatrix._11 = vecRight.x;
	viewMatrix._21 = vecRight.y;
	viewMatrix._31 = vecRight.z;

	// up
	viewMatrix._12 = vecUp.x;
	viewMatrix._22 = vecUp.y;
	viewMatrix._32 = vecUp.z;

	// front
	viewMatrix._13 = vecFront.x;
	viewMatrix._23 = vecFront.y;
	viewMatrix._33 = vecFront.z;

	// 4th
	viewMatrix._14 = 0.0f;
	viewMatrix._24 = 0.0f;
	viewMatrix._34 = 0.0f;
	viewMatrix._44 = 1.0f;

	viewMatrix._41 = -D3DXVec3Dot(&vecPosition, &vecRight);
	viewMatrix._42 = -D3DXVec3Dot(&vecPosition, &vecUp);
	viewMatrix._43 = -D3DXVec3Dot(&vecPosition, &vecFront);

/*
	viewMatrix._41 = vecPosition.x;
	viewMatrix._42 = vecPosition.y;
	viewMatrix._43 = vecPosition.z;
*/
}

void DrawParticles()
{
	if (particleArray.size() == 0)
		return;

	int backup = RenderPolygon.NumberOfVertices;

	// sort particle array
	std::sort(particleArray.begin(), particleArray.end());

	// loop particles and add them to vertex buffer
	for (size_t i = 0; i < particleArray.size(); i++)
	{
		RenderPolygon.NumberOfVertices = particleArray[i].numVerts;
		D3D_Particle_Output(&particleArray[i].particle, &particleArray[i].vertices[0]);
	}

	particleArray.resize(0);

	// restore RenderPolygon.NumberOfVertices value...
	RenderPolygon.NumberOfVertices = backup;
}

const int32_t  NO_TEXTURE   = -1;
const uint32_t TALLFONT_TEX = 999;

// set them to 'null' texture initially
int32_t	currentTextureID	= NO_TEXTURE;
int32_t	currentWaterTexture = NO_TEXTURE;

uint32_t	NumVertices = 0;
uint32_t	NumIndicies = 0;
uint32_t	vb = 0;
uint32_t	particleIndex = 0;
static uint32_t	renderCount = 0;

extern AVPIndexedFont IntroFont_Light;

const float Zoffset = 2.0f;

// menu graphics arrat
extern AVPMENUGFX AvPMenuGfxStorage[];

#define RGBLIGHT_MAKE(r,g,b) RGB_MAKE(r,g,b)
#define RGBALIGHT_MAKE(r,g,b,a) RGBA_MAKE(r,g,b,a)

#define FMV_ON 0
#define FMV_EVERYWHERE 0
#define FMV_SIZE 128

extern int SpecialFXImageNumber;
extern int SmokyImageNumber;
extern int ChromeImageNumber;
extern int HUDFontsImageNumber;
extern int BurningImageNumber;
extern int PredatorVisionChangeImageNumber;
extern int PredatorNumbersImageNumber;
extern int StaticImageNumber;
extern int AAFontImageNumber;
extern int WaterShaftImageNumber;
extern int HUDImageNumber;
extern int HUDFontsImageNumber;
extern int AAFontImageNumber;

AVPTEXTURE FMVTextureHandle[4];
AVPTEXTURE NoiseTextureHandle;

int WaterXOrigin;
int WaterZOrigin;
float WaterUScale;
float WaterVScale;

int MeshZScale;
int MeshXScale;
int WaterFallBase;

int LightIntensityAtPoint(VECTORCH *pointPtr);

VECTORCH MeshWorldVertex[256];
uint32_t MeshVertexColour[256];
char MeshVertexOutcode[256];

// Externs

extern int VideoMode;
extern int WindowMode;
extern int ZBufferRequestMode;
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern VIEWDESCRIPTORBLOCK* Global_VDB_Ptr;

extern IMAGEHEADER ImageHeaderArray[];
extern int NumImagesArray[];

extern int NormalFrameTime;
int WireFrameMode;

extern int HUDScaleFactor;
extern MODULE *playerPherModule;
extern int NumOnScreenBlocks;
extern DISPLAYBLOCK *OnScreenBlockList[];
extern char LevelName[];

extern int CloakingPhase;
extern int NumActiveBlocks;
extern DISPLAYBLOCK *ActiveBlockList[];

extern unsigned char GammaValues[256];
extern int GlobalAmbience;
extern char CloakedPredatorIsMoving;

uint32_t FMVParticleColour;
VECTORCH MeshVertex[256];

extern BOOL LevelHasStars;

extern void HandleRainShaft(MODULE *modulePtr, int bottomY, int topY, int numberOfRaindrops);
void D3D_DrawWaterPatch(int xOrigin, int yOrigin, int zOrigin);
extern void RenderMirrorSurface(void);
extern void RenderMirrorSurface2(void);
extern void RenderParticlesInMirror(void);
extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
extern void HandleRain(int numberOfRaindrops);
void D3D_DrawWaterFall(int xOrigin, int yOrigin, int zOrigin);
extern void RenderSky(void);
extern void RenderStarfield(void);
void D3D_DrawMoltenMetalMesh_Unclipped(void);
static void D3D_OutputTriangles(void);
BOOL LockExecuteBuffer();
BOOL UnlockExecuteBufferAndPrepareForUse();

//Globals

// keep track of set render states
static bool	D3DAlphaBlendEnable;
D3DBLEND D3DSrcBlend;
D3DBLEND D3DDestBlend;

// for tallfont
bool D3DAlphaTestEnable = FALSE;
static bool D3DStencilEnable;
D3DCMPFUNC D3DStencilFunc;
static unsigned int D3DStencilRef;
static unsigned char D3DZFunc;
static unsigned char D3DDitherEnable;
static bool D3DZWriteEnable;

static unsigned char DefaultD3DTextureFilterMin;
static unsigned char DefaultD3DTextureFilterMax;

extern void ScanImagesForFMVs();

static int NumberOfRenderedTriangles = 0;
int NumberOfLandscapePolygons;
RENDERSTATES CurrentRenderStates;

static HRESULT LastError;

BOOL LockExecuteBuffer();
BOOL UnlockExecuteBufferAndPrepareForUse();

void ChangeTranslucencyMode(enum TRANSLUCENCY_TYPE translucencyRequired);
void ChangeFilteringMode(enum FILTERING_MODE_ID filteringRequired);
void ChangeTextureAddressMode(enum TEXTURE_ADDRESS_MODE textureAddressMode);
void CheckOrthoBuffer(uint32_t numVerts, int32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum TEXTURE_ADDRESS_MODE textureAddressMode, enum FILTERING_MODE_ID filteringMode);

static inline void OUTPUT_TRIANGLE(int a, int b, int c, int n) 
{
	mainIndex[NumIndicies]   = (NumVertices - (n) + (a));
	mainIndex[NumIndicies+1] = (NumVertices - (n) + (b));
	mainIndex[NumIndicies+2] = (NumVertices - (n) + (c));
	NumIndicies+=3;
}

#if 0
static void SetNewTexture(const int32_t textureID)
{
	if (textureID != currentTextureID)
	{
		LastError = d3d.lpD3DDevice->SetTexture(0, ImageHeaderArray[textureID].Direct3DTexture);
		if (FAILED(LastError))
		{
			OutputDebugString("Couldn't set menu quad texture\n");
		}
		currentTextureID = textureID;
	}
}

static inline void PushVerts()
{
	if ((NumVertices * 7) > 2047)
	{
		OutputDebugString("too much data for BeginPush\n");
		NumVertices = 0;
		return;
	}

	DWORD *pPush;
	DWORD dwordCount = 7 * NumVertices;

	// The "+5" is to reserve enough overhead for the encoding
	// parameters. It can safely be more, but no less.
	d3d.lpD3DDevice->BeginPush( dwordCount + 5, &pPush );

	pPush[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
	pPush[1] = D3DPT_TRIANGLELIST;

	pPush[2] = D3DPUSH_ENCODE( D3DPUSH_NOINCREMENT_FLAG | D3DPUSH_INLINE_ARRAY, dwordCount );
	pPush += 3;

	memcpy(pPush, (DWORD*) &testVertex, NumVertices * sizeof(D3DLVERTEX));
/*
	for (int i = 0; i < testCount; i++)
	{
		pPush[0] = *((DWORD*) &testVertex[i].sx);
		pPush[1] = *((DWORD*) &testVertex[i].sy);
		pPush[2] = *((DWORD*) &testVertex[i].sz);
		pPush[3] = *((DWORD*) &testVertex[i].rhw);
		pPush[4] = *((DWORD*) &testVertex[i].color);
		pPush[5] = *((DWORD*) &testVertex[i].specular);
		pPush[6] = *((DWORD*) &testVertex[i].tu);
		pPush[7] = *((DWORD*) &testVertex[i].tv);

		pPush += 8;
	}
*/
	pPush+= dwordCount;

	pPush[0] = D3DPUSH_ENCODE( D3DPUSH_SET_BEGIN_END, 1 );
	pPush[1] = 0;
	pPush += 2;

	d3d.lpD3DDevice->EndPush( pPush );

	NumVertices = 0;
}
#endif

static inline void D3D_OutputTriangles()
{
	switch (RenderPolygon.NumberOfVertices)
	{
		default:
			OutputDebugString("unexpected number of verts to render\n");
			break;
		case 0:
			OutputDebugString("Asked to render 0 verts\n");
			break;
		case 3:
		{
			OUTPUT_TRIANGLE(0,2,1, 3);
			break;
		}
		case 4:
		{
			OUTPUT_TRIANGLE(0,1,2, 4);
			OUTPUT_TRIANGLE(0,2,3, 4);
			break;
		}
		case 5:
		{
			OUTPUT_TRIANGLE(0,1,4, 5);
		    OUTPUT_TRIANGLE(1,3,4, 5);
		    OUTPUT_TRIANGLE(1,2,3, 5);

			break;
		}
		case 6:
		{
			OUTPUT_TRIANGLE(0,4,5, 6);
		    OUTPUT_TRIANGLE(0,3,4, 6);
		    OUTPUT_TRIANGLE(0,2,3, 6);
		    OUTPUT_TRIANGLE(0,1,2, 6);

			break;
		}
		case 7:
		{
			OUTPUT_TRIANGLE(0,5,6, 7);
		    OUTPUT_TRIANGLE(0,4,5, 7);
		    OUTPUT_TRIANGLE(0,3,4, 7);
		    OUTPUT_TRIANGLE(0,2,3, 7);
		    OUTPUT_TRIANGLE(0,1,2, 7);

			break;
		}
		case 8:
		{
			OUTPUT_TRIANGLE(0,6,7, 8);
		    OUTPUT_TRIANGLE(0,5,6, 8);
		    OUTPUT_TRIANGLE(0,4,5, 8);
		    OUTPUT_TRIANGLE(0,3,4, 8);
		    OUTPUT_TRIANGLE(0,2,3, 8);
		    OUTPUT_TRIANGLE(0,1,2, 8);

			break;
		}
	}

	if (NumVertices > (MAX_VERTEXES-12))
	{
		UnlockExecuteBufferAndPrepareForUse();
		ExecuteBuffer();
		LockExecuteBuffer();
	}
}

BOOL SetExecuteBufferDefaults()
{
    NumVertices = 0;
/*
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 8);
*/
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_LINEAR);
//	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAXANISOTROPY, 8);

	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLOROP,	D3DTOP_MODULATE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1,	D3DTA_TEXTURE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2,	D3DTA_DIFFUSE);

	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP,	D3DTOP_MODULATE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1,	D3DTA_TEXTURE);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2,	D3DTA_DIFFUSE);

	d3d.lpD3DDevice->SetTextureStageState(1, D3DTSS_COLOROP,	D3DTOP_DISABLE);
	d3d.lpD3DDevice->SetTextureStageState(1, D3DTSS_ALPHAOP,	D3DTOP_DISABLE);

	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
	d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);

//	float alphaRef = 0.5f;
//	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAREF,	 *((DWORD*)&alphaRef));//(DWORD)0.5);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAREF,	 (DWORD)0.5);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);

	d3d.lpD3DDevice->SetRenderState(D3DRS_OCCLUSIONCULLENABLE, FALSE);

//	ChangeFilteringMode(FILTERING_BILINEAR_OFF);
	ChangeTranslucencyMode(TRANSLUCENCY_OFF);

	d3d.lpD3DDevice->SetRenderState(D3DRS_CULLMODE,			D3DCULL_NONE);
//	d3d.lpD3DDevice->SetRenderState(D3DRS_CLIPPING,			TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_LIGHTING,			FALSE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_SPECULARENABLE,	TRUE);
	d3d.lpD3DDevice->SetRenderState(D3DRS_DITHERENABLE,		FALSE);
	D3DDitherEnable = TRUE;

	// enable z-buffer
	d3d.lpD3DDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

	// enable z writes (already on by default)
	d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
	D3DZWriteEnable = TRUE;

	// set less + equal z buffer test
	d3d.lpD3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
	D3DZFunc = D3DCMP_LESSEQUAL;

	// fill out first render list struct
	memset(&renderList[0], 0, sizeof(RENDER_STATES));
	renderList[0].textureID = NO_TEXTURE;

	// fill out first ortho render list struct
	memset(&orthoList[0], 0, sizeof(ORTHO_OBJECTS));
	orthoList[0].textureID = NO_TEXTURE;

	renderTest.push_back(renderList[0]);

	// Enable fog blending.
//    d3d.lpD3DDevice->SetRenderState(D3DRS_FOGENABLE, TRUE);

    // Set the fog color.
//    d3d.lpD3DDevice->SetRenderState(D3DRS_FOGCOLOR, FOG_COLOUR);

    return TRUE;
}

void CheckOrthoBuffer(uint32_t numVerts, int32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum TEXTURE_ADDRESS_MODE textureAddressMode, enum FILTERING_MODE_ID filteringMode = FILTERING_BILINEAR_ON)
{
	assert (numVerts == 4);

	// check if we've got enough room. if not, flush
		// TODO

	assert (orthoListCount < MAX_VERTEXES);

	// create a new list item for it
	orthoList[orthoListCount].textureID = textureID;
	orthoList[orthoListCount].vertStart = 0;
	orthoList[orthoListCount].vertEnd = 0;
	orthoList[orthoListCount].translucencyType = translucencyMode;
	orthoList[orthoListCount].textureAddressMode = textureAddressMode;
	orthoList[orthoListCount].filteringType = filteringMode;

	// check if current vertexes use the same texture and render states as the previous. if they do, we can 'merge' the two together
	if (orthoListCount != 0 &&
		textureID == orthoList[orthoListCount-1].textureID && 
		translucencyMode == orthoList[orthoListCount-1].translucencyType &&
		textureAddressMode == orthoList[orthoListCount-1].textureAddressMode &&
		filteringMode == orthoList[orthoListCount-1].filteringType)
	{
		// ok, drop back to the previous data
		orthoListCount--;
	}
	else
	{
		// new unique entry
		orthoList[orthoListCount].indexStart = orthoIBOffset;
	}

	// create the indicies for each triangle
	for (uint32_t i = 0; i < numVerts; i+=4)
	{
		orthoIndex[orthoIBOffset]   = orthoVBOffset+i+0;
		orthoIndex[orthoIBOffset+1] = orthoVBOffset+i+1;
		orthoIndex[orthoIBOffset+2] = orthoVBOffset+i+2;
		orthoIBOffset+=3;

		orthoIndex[orthoIBOffset]   = orthoVBOffset+i+2;
		orthoIndex[orthoIBOffset+1] = orthoVBOffset+i+1;
		orthoIndex[orthoIBOffset+2] = orthoVBOffset+i+3;
		orthoIBOffset+=3;
	}

	orthoList[orthoListCount].vertEnd = orthoVBOffset + numVerts;
	orthoList[orthoListCount].indexEnd = orthoIBOffset;

	orthoListCount++;
}

void CheckVertexBuffer(uint32_t numVerts, int32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum FILTERING_MODE_ID filteringMode = FILTERING_BILINEAR_ON) 
{
	uint32_t realNumVerts = 0;

	switch (numVerts) 
	{
		case 3:
			realNumVerts = 3; 
			break;
		case 4:
			realNumVerts = 6; 
			break;
		case 5:
			realNumVerts = 9; 
			break;
		case 6:
			realNumVerts = 12; 
			break;
		case 7:
			realNumVerts = 15; 
			break;
		case 8:
			realNumVerts = 18; 
			break;
		case 0:
			return;
		case 256:
			realNumVerts = 1350; 
			break;
		default:
			// need to check this is ok!!
			realNumVerts = numVerts;
	}

	// check if we've got enough room. if not, flush
	if (NumVertices + numVerts > (MAX_VERTEXES-12))
	{
		UnlockExecuteBufferAndPrepareForUse();
		ExecuteBuffer();
		LockExecuteBuffer();
	}

	renderList[renderCount].textureID = textureID;
	renderList[renderCount].vertStart = 0;
	renderList[renderCount].vertEnd = 0;

	renderList[renderCount].indexStart = 0;
	renderList[renderCount].indexEnd = 0;

	renderList[renderCount].translucencyType = translucencyMode;
	renderList[renderCount].filteringType = filteringMode;

	// check if current vertexes use the same texture and render states as the previous
	// if they do, we can 'merge' the two together
	if (renderCount != 0 &&
		textureID == renderList[renderCount-1].textureID && 
		translucencyMode == renderList[renderCount-1].translucencyType && 
		filteringMode	 == renderList[renderCount-1].filteringType)
	{
		// ok, drop back to the previous data
		renderTest.pop_back();
		renderCount--;
	}
	else
	{
		renderList[renderCount].vertStart = NumVertices;
		renderList[renderCount].indexStart = NumIndicies;
	}

	renderList[renderCount].vertEnd = NumVertices + numVerts;
	renderList[renderCount].indexEnd = NumIndicies + realNumVerts;

	renderTest.push_back(renderList[renderCount]);
	renderCount++;

	NumVertices += numVerts;
}

BOOL LockExecuteBuffer()
{
	// lock 2D ortho vertex buffer
	LastError = d3d.lpD3DOrthoVertexBuffer->Lock(0, 0, (byte**)&orthoVerts, D3DLOCK_DISCARD);
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	// lock 2D ortho index buffer
	LastError = d3d.lpD3DOrthoIndexBuffer->Lock(0, 0, (byte**)&orthoIndex, D3DLOCK_DISCARD);
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}
	
//	d3d.lpD3DDevice->SetVertexShader(D3DFVF_LVERTEX);

	NumVertices = 0;
	NumIndicies = 0;
	renderCount = 0;
	vb = 0;
	
	orthoVBOffset = 0;
	orthoIBOffset = 0;
	orthoListCount = 0;

	renderTest.clear();

    return TRUE;
}

BOOL UnlockExecuteBufferAndPrepareForUse()
{
	// unlock orthographic quad vertex buffer
	LastError = d3d.lpD3DOrthoVertexBuffer->Unlock();
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	// unlock orthographic index buffer
	LastError = d3d.lpD3DOrthoIndexBuffer->Unlock();
	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}

	return TRUE;
}

BOOL BeginD3DScene()
{
	NumberOfRenderedTriangles = 0;

	LastError = d3d.lpD3DDevice->BeginScene();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}
/*
	LastError = d3d.lpD3DDevice->SetVertexShader(d3d.vertexShader);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	LastError = d3d.lpD3DDevice->SetPixelShader(d3d.pixelShader);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}
*/
	LastError = d3d.lpD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_ARGB(0,0,0,0), 1.0f, 0);

	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	return TRUE;
}

void D3D_SetupSceneDefaults()
{
	/* force translucency state to be reset */
	CurrentRenderStates.TranslucencyMode = TRANSLUCENCY_NOT_SET;
	CurrentRenderStates.FilteringMode = FILTERING_NOT_SET;
	ChangeFilteringMode(FILTERING_BILINEAR_ON);

	CheckWireFrameMode(0);

	// this defaults to FALSE
	if (D3DDitherEnable != TRUE)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_DITHERENABLE, TRUE);
		D3DDitherEnable = TRUE;
	}
}

BOOL EndD3DScene()
{
    LastError = d3d.lpD3DDevice->EndScene();

	if (ShowDebuggingText.PolyCount)
	{
		ReleasePrintDebuggingText("NumberOfLandscapePolygons: %d\n",NumberOfLandscapePolygons);
		ReleasePrintDebuggingText("NumberOfRenderedTriangles: %d\n",NumberOfRenderedTriangles);
	}
//	textprint ("NumberOfRenderedTrianglesPerSecond: %d\n",DIV_FIXED(NumberOfRenderedTriangles,NormalFrameTime));
	NumberOfLandscapePolygons = 0;
	NumberOfRenderedTriangles = 0;

	if (FAILED(LastError)) 
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return FALSE;
	}
	
	return TRUE;
}

static void ChangeTexture(const int32_t textureID)
{
	if (textureID == currentTextureID) 
		return;

	if (textureID >= texIDoffset)
	{
		LastError = d3d.lpD3DDevice->SetTexture(0, Tex_GetTexture(textureID));
		if (!FAILED(LastError)) currentTextureID = textureID;
			return;
	}

	// menu large font
	else if (textureID == TALLFONT_TEX)
	{
		LastError = d3d.lpD3DDevice->SetTexture(0, IntroFont_Light.info.menuTexture);
		if (!FAILED(LastError)) currentTextureID = TALLFONT_TEX;
			return;
	}

	// if texture was specified as 'null'
	else if (textureID == NO_TEXTURE)
	{
		LastError = d3d.lpD3DDevice->SetTexture(0, NULL);
		if (!FAILED(LastError)) currentTextureID = NO_TEXTURE;
		return;
	}

	// if in menus (outside game)
	if (mainMenu)
	{
		LastError = d3d.lpD3DDevice->SetTexture(0, AvPMenuGfxStorage[textureID].menuTexture);
		if (!FAILED(LastError)) currentTextureID = textureID;
		return;
	}
	else
	{
		LastError = d3d.lpD3DDevice->SetTexture(0, ImageHeaderArray[textureID].Direct3DTexture);
		if (!FAILED(LastError)) currentTextureID = textureID;
		return;
	}
}

BOOL ExecuteBuffer()
{
	// sort the list of render objects
	std::sort(renderTest.begin(), renderTest.end());

	D3DXMATRIX matProjection;
	D3DXMatrixPerspectiveFovLH(&matProjection, D3DXToRadian(fov), (float)ScreenDescriptorBlock.SDB_Width / (float)ScreenDescriptorBlock.SDB_Height, 64.0f, 1000000.0f);

	#ifndef USE_D3DVIEWTRANSFORM
		D3DXMatrixIdentity(&viewMatrix); // we want to use the identity matrix in this case
	#endif

//	d3d.lpD3DDevice->SetTransform(D3DTS_VIEW,		&viewMatrix);
//	d3d.lpD3DDevice->SetTransform(D3DTS_PROJECTION, &matProjection);

	LastError = d3d.lpD3DDevice->SetVertexShader(d3d.vertexShader);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	LastError = d3d.lpD3DDevice->SetPixelShader(d3d.pixelShader);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	D3DXMATRIX matWorld; 
	D3DXMatrixIdentity(&matWorld);

	D3DXMATRIX matWorldViewProj = matWorld * viewMatrix * matProjection;
	
	D3DXMatrixTranspose(&matWorldViewProj, &matWorldViewProj);
	d3d.lpD3DDevice->SetVertexShaderConstant(0, &matWorldViewProj, 4);

	ChangeTextureAddressMode(TEXTURE_WRAP);

	for (uint32_t i = 0; i < renderCount; i++)
	{
		if (renderTest[i].translucencyType != TRANSLUCENCY_OFF) 
			continue;

		// change render states if required
		ChangeTexture(renderTest[i].textureID);
		ChangeTranslucencyMode(renderTest[i].translucencyType);
		ChangeFilteringMode(renderTest[i].filteringType);

		uint32_t numPrimitives = (renderTest[i].indexEnd - renderTest[i].indexStart) / 3;

		if (numPrimitives > 0) 
		{
			LastError = d3d.lpD3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
				0, // min index (ignored)
				0, // num vertices (ignored)
				numPrimitives,
				mainIndex,
				D3DFMT_INDEX16,
				mainVertex,
				sizeof(D3DLVERTEX));

			if (FAILED(LastError))
			{
				LogDxError(LastError, __LINE__, __FILE__);
			}
		}
		NumberOfRenderedTriangles += numPrimitives / 3;
	}

	// do transparents here..
	for (uint32_t i = 0; i < renderCount; i++)
	{
		if (renderTest[i].translucencyType == TRANSLUCENCY_OFF) 
			continue;

		// change render states if required
		ChangeTexture(renderTest[i].textureID);
		ChangeTranslucencyMode(renderTest[i].translucencyType);
		ChangeFilteringMode(renderTest[i].filteringType);

		uint32_t numPrimitives = (renderTest[i].indexEnd - renderTest[i].indexStart) / 3;

		if (numPrimitives > 0) 
		{
			LastError = d3d.lpD3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,
				0, // min index (ignored)
				0, // num vertices (ignored)
				numPrimitives,
				mainIndex,
				D3DFMT_INDEX16,
				mainVertex,
				sizeof(D3DLVERTEX));

			if (FAILED(LastError))
			{
				LogDxError(LastError, __LINE__, __FILE__);
			}
		}
		NumberOfRenderedTriangles += numPrimitives / 3;
	}
#if 1
	// render any orthographic quads
	if (orthoListCount)
	{
		LastError = d3d.lpD3DDevice->SetStreamSource(0, d3d.lpD3DOrthoVertexBuffer, sizeof(ORTHOVERTEX));
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		LastError = d3d.lpD3DDevice->SetIndices(d3d.lpD3DOrthoIndexBuffer, 0);
		if (FAILED(LastError)) 
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		LastError = d3d.lpD3DDevice->SetVertexShader(d3d.orthoVertexShader);
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		LastError = d3d.lpD3DDevice->SetPixelShader(d3d.pixelShader);
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		D3DXMATRIX matView; 
		D3DXMatrixIdentity(&matView);

		D3DXMATRIX matWorld; 
		D3DXMatrixIdentity(&matWorld);

		D3DXMATRIX matOrtho;
		D3DXMatrixOrthoLH(&matOrtho, 2.0f, -2.0f, 1.0f, 10.0f);

		D3DXMATRIX matWorldViewProj = matWorld * matView * matOrtho;

		D3DXMatrixTranspose(&matWorldViewProj, &matWorldViewProj);
		d3d.lpD3DDevice->SetVertexShaderConstant(0, &matWorldViewProj, 4);
/*
		d3d.lpD3DDevice->SetTransform(D3DTS_WORLD, &matWorld);
		d3d.lpD3DDevice->SetTransform(D3DTS_VIEW, &matView);
		d3d.lpD3DDevice->SetTransform(D3DTS_PROJECTION, &matOrtho);
*/
		// loop through list drawing the quads
		for (uint32_t i = 0; i < orthoListCount; i++)
		{
			ChangeTexture(orthoList[i].textureID);
			ChangeTranslucencyMode(orthoList[i].translucencyType);
			ChangeTextureAddressMode(orthoList[i].textureAddressMode);
			ChangeFilteringMode(orthoList[i].filteringType);

			uint32_t primitiveCount = (orthoList[i].indexEnd - orthoList[i].indexStart) / 3;

			LastError = d3d.lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 
			   0,
			   0,
			   orthoList[i].indexStart,
			   primitiveCount);

			if (FAILED(LastError))
			{
				LogDxError(LastError, __LINE__, __FILE__);
			}
		}
	}
#endif
	return TRUE;
}

void SetFogDistance(int fogDistance)
{
	if (fogDistance>10000) fogDistance = 10000;
	fogDistance+=5000;
	fogDistance=2000;
	CurrentRenderStates.FogDistance = fogDistance;
//	textprint("fog distance %d\n",fogDistance);

}

void ChangeTextureAddressMode(enum TEXTURE_ADDRESS_MODE textureAddressMode)
{
	if (CurrentRenderStates.TextureAddressMode == textureAddressMode)
		return;

	CurrentRenderStates.TextureAddressMode = textureAddressMode;

	if (textureAddressMode == TEXTURE_WRAP)
	{
		// wrap texture addresses
		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSU Wrap fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSV Wrap fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_WRAP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSW Wrap fail");
		}
	}
	else if (textureAddressMode == TEXTURE_CLAMP)
	{
		// clamp texture addresses
		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSU Clamp fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSV Clamp fail");
		}

		LastError = d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);
		if (FAILED(LastError))
		{
			OutputDebugString("D3DSAMP_ADDRESSW Clamp fail");
		}
	}
}

void ChangeTranslucencyMode(enum TRANSLUCENCY_TYPE translucencyRequired)
{
	if (CurrentRenderStates.TranslucencyMode == translucencyRequired) 
		return;
	{
		CurrentRenderStates.TranslucencyMode = translucencyRequired;
		switch(CurrentRenderStates.TranslucencyMode)
		{
		 	case TRANSLUCENCY_OFF:
			{
				if (TRIPTASTIC_CHEATMODE||MOTIONBLUR_CHEATMODE)
				{
					if (D3DAlphaBlendEnable != TRUE)
					{
						d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
						D3DAlphaBlendEnable = TRUE;
					}
					if (D3DSrcBlend != D3DBLEND_INVSRCALPHA)
					{
						d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_INVSRCALPHA);
						D3DSrcBlend = D3DBLEND_INVSRCALPHA;
					}
					if (D3DDestBlend != D3DBLEND_SRCALPHA)
					{
						d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_SRCALPHA);
						D3DDestBlend = D3DBLEND_SRCALPHA;
					}
				}
				else
				{
					if (D3DAlphaBlendEnable != FALSE)
					{
						d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
						D3DAlphaBlendEnable = FALSE;
					}
					if (D3DSrcBlend != D3DBLEND_ONE)
					{
						d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
						D3DSrcBlend = D3DBLEND_ONE;
					}
					if (D3DDestBlend != D3DBLEND_ZERO)
					{
						d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);
						D3DDestBlend = D3DBLEND_ZERO;
					}
				}
				break;
			}
		 	case TRANSLUCENCY_NORMAL:
			{
				if (D3DAlphaBlendEnable != TRUE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					D3DAlphaBlendEnable = TRUE;
				}
				if (D3DSrcBlend != D3DBLEND_SRCALPHA)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
					D3DSrcBlend = D3DBLEND_SRCALPHA;
				}
				if (D3DDestBlend != D3DBLEND_INVSRCALPHA)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
					D3DDestBlend = D3DBLEND_INVSRCALPHA;
				}
				break;
			}
		 	case TRANSLUCENCY_COLOUR:
			{
				if (D3DAlphaBlendEnable != TRUE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					D3DAlphaBlendEnable = TRUE;
				}
				if (D3DSrcBlend != D3DBLEND_ZERO)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
					D3DSrcBlend = D3DBLEND_ZERO;
				}
				if (D3DDestBlend != D3DBLEND_SRCCOLOR)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_SRCCOLOR);
					D3DDestBlend = D3DBLEND_SRCCOLOR;
				}
				break;
			}
		 	case TRANSLUCENCY_INVCOLOUR:
			{
				if (D3DAlphaBlendEnable != TRUE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					D3DAlphaBlendEnable = TRUE;
				}
				if (D3DSrcBlend != D3DBLEND_ZERO)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
					D3DSrcBlend = D3DBLEND_ZERO;
				}
				if (D3DDestBlend != D3DBLEND_INVSRCCOLOR)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCCOLOR);
					D3DDestBlend = D3DBLEND_INVSRCCOLOR;
				}
				break;
			}
	  		case TRANSLUCENCY_GLOWING:
			{
				if (D3DAlphaBlendEnable != TRUE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					D3DAlphaBlendEnable = TRUE;
				}
				if (D3DSrcBlend != D3DBLEND_SRCALPHA)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
					D3DSrcBlend = D3DBLEND_SRCALPHA;
				}
				if (D3DDestBlend != D3DBLEND_ONE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
					D3DDestBlend = D3DBLEND_ONE;
				}
				break;
			}
	  		case TRANSLUCENCY_DARKENINGCOLOUR:
			{
				if (D3DAlphaBlendEnable != TRUE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					D3DAlphaBlendEnable = TRUE;
				}
				if (D3DSrcBlend != D3DBLEND_INVDESTCOLOR)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_INVDESTCOLOR);
					D3DSrcBlend = D3DBLEND_INVDESTCOLOR;
				}

				if (D3DDestBlend != D3DBLEND_ZERO)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ZERO);
					D3DDestBlend = D3DBLEND_ZERO;
				}
				break;
			}
			case TRANSLUCENCY_JUSTSETZ:
			{
				if (D3DAlphaBlendEnable != TRUE)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
					D3DAlphaBlendEnable = TRUE;
				}
				if (D3DSrcBlend != D3DBLEND_ZERO)
				{
					d3d.lpD3DDevice->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ZERO);
					D3DSrcBlend = D3DBLEND_ZERO;
				}
				if (D3DDestBlend != D3DBLEND_ONE) {
					d3d.lpD3DDevice->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
					D3DDestBlend = D3DBLEND_ONE;
				}
			}
			default:
				break;
		}
	}
}

void ChangeFilteringMode(enum FILTERING_MODE_ID filteringRequired)
{
	if (CurrentRenderStates.FilteringMode == filteringRequired) return;

	CurrentRenderStates.FilteringMode = filteringRequired;

	switch(CurrentRenderStates.FilteringMode)
	{
		case FILTERING_BILINEAR_OFF:
		{
			d3d.lpD3DDevice->SetTextureStageState(0,D3DTSS_MAGFILTER,D3DTEXF_POINT);
			d3d.lpD3DDevice->SetTextureStageState(0,D3DTSS_MINFILTER,D3DTEXF_POINT);
			break;
		}
		case FILTERING_BILINEAR_ON:
		{
			d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
			d3d.lpD3DDevice->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
			break;
		}
		default:
		{
			LOCALASSERT("Unrecognized filtering mode"==0);
			OutputDebugString("Unrecognized filtering mode");
			break;
		}
	}
}

void ToggleWireframe()
{
	if (CurrentRenderStates.WireFrameModeIsOn)
		CheckWireFrameMode(0);
	else
		CheckWireFrameMode(1);
}

extern void CheckWireFrameMode(int shouldBeOn)
{
	if (shouldBeOn) 
		shouldBeOn = 1;

	if (CurrentRenderStates.WireFrameModeIsOn != shouldBeOn)
	{
		CurrentRenderStates.WireFrameModeIsOn = shouldBeOn;
		if (shouldBeOn)
		{
			d3d.lpD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		}
		else
		{
			d3d.lpD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
		}
	}
}

void D3D_BackdropPolygon_Output(POLYHEADER *inputPolyPtr,RENDERVERTEX *renderVerticesPtr)
{
	int flags;
	int texoffset;

	float ZNear;

    // Get ZNear
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	// Take header information
	flags = inputPolyPtr->PolyFlags;

	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	texoffset = (inputPolyPtr->PolyColour & ClrTxDefn);

	float RecipW = 1.0f / (float)ImageHeaderArray[texoffset].ImageWidth;
	float RecipH = 1.0f / (float)ImageHeaderArray[texoffset].ImageHeight;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, texoffset, TRANSLUCENCY_OFF);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = 1.0f;

		mainVertex[vb].color = RGBLIGHT_MAKE(vertices->R,vertices->G,vertices->B);
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);

		mainVertex[vb].tu = (float)(vertices->U) * RecipW;
		mainVertex[vb].tv = (float)(vertices->V) * RecipH;

		vb++;
	}

	D3D_OutputTriangles();
}

void D3D_ZBufferedGouraudTexturedPolygon_Output(POLYHEADER *inputPolyPtr,RENDERVERTEX *renderVerticesPtr)
{
	// bjd - This seems to be the function responsible for drawing level geometry
	// and the players weapon

	int texoffset;

	float ZNear;
	float RecipW, RecipH;

    // Get ZNear
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	texoffset = (inputPolyPtr->PolyColour & ClrTxDefn);

	if (!texoffset)
		texoffset = currentTextureID;

	RecipW = 1.0f / (float) ImageHeaderArray[texoffset].ImageWidth;
	RecipH = 1.0f / (float) ImageHeaderArray[texoffset].ImageHeight;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, texoffset, RenderPolygon.TranslucencyMode);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z;

		extern unsigned char GammaValues[256];

		/* need this so we can enable alpha test and not lose pred arms in green vision mode, and also not lose
		/* aliens in pred red alien vision mode */
		if (vertices->A == 0)
			vertices->A = 1;

		mainVertex[vb].color = RGBALIGHT_MAKE(GammaValues[vertices->R],GammaValues[vertices->G],GammaValues[vertices->B],vertices->A);
		mainVertex[vb].specular = RGBALIGHT_MAKE(GammaValues[vertices->SpecularR],GammaValues[vertices->SpecularG],GammaValues[vertices->SpecularB],255);

		mainVertex[vb].tu = (float)(vertices->U) * RecipW;
		mainVertex[vb].tv = (float)(vertices->V) * RecipH;

		vb++;
	}

	D3D_OutputTriangles();
}

void D3D_ZBufferedGouraudPolygon_Output(POLYHEADER *inputPolyPtr,RENDERVERTEX *renderVerticesPtr)
{
	/* responsible for drawing predators lock on triangle, for one */
	int flags;

	float ZNear;

    // Get ZNear
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	// Take header information
	flags = inputPolyPtr->PolyFlags;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, NO_TEXTURE, RenderPolygon.TranslucencyMode);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z;

		if (flags & iflag_transparent)
		{
			mainVertex[vb].color = RGBALIGHT_MAKE(vertices->R,vertices->G,vertices->B, vertices->A);
		}
		else
		{
			mainVertex[vb].color = RGBLIGHT_MAKE(vertices->R,vertices->G,vertices->B);
		}

		mainVertex[vb].specular = (D3DCOLOR)1.0f;
		mainVertex[vb].tu = 0.0f;
		mainVertex[vb].tv = 0.0f;

		vb++;
	}

	D3D_OutputTriangles();
}

void D3D_PredatorThermalVisionPolygon_Output(POLYHEADER *inputPolyPtr,RENDERVERTEX *renderVerticesPtr)
{
	float ZNear;

    // Get ZNear
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z;

		mainVertex[vb].color = RGBALIGHT_MAKE(vertices->R,vertices->G,vertices->B,vertices->A);//RGBALIGHT_MAKE(255,255,255,255);
		mainVertex[vb].specular = (D3DCOLOR)1.0f;
		mainVertex[vb].tu = 0.0f;
		mainVertex[vb].tv = 0.0f;

		vb++;
	}

	D3D_OutputTriangles();
}

void D3D_ZBufferedCloakedPolygon_Output(POLYHEADER *inputPolyPtr,RENDERVERTEX *renderVerticesPtr)
{
	extern int CloakingMode;
	extern char CloakedPredatorIsMoving;
	int uOffset = FastRandom()&255;
	int vOffset = FastRandom()&255;

	int flags;
	int texoffset;

	float ZNear;

    // Get ZNear
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	// Take header information
	flags = inputPolyPtr->PolyFlags;

	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	texoffset = (inputPolyPtr->PolyColour & ClrTxDefn);

	float RecipW = 1.0f / (float) ImageHeaderArray[texoffset].ImageWidth;
	float RecipH = 1.0f / (float) ImageHeaderArray[texoffset].ImageHeight;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, texoffset, TRANSLUCENCY_NORMAL);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z;

		if (CloakedPredatorIsMoving)
		{
			mainVertex[vb].color = RGBALIGHT_MAKE(vertices->R,vertices->G,vertices->B,vertices->A);
		}
		else
		{
			mainVertex[vb].color = RGBALIGHT_MAKE(vertices->R,vertices->G,vertices->B,vertices->A);
		}

		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].tu = (float)(vertices->U) * RecipW;
		mainVertex[vb].tv = (float)(vertices->V) * RecipH;

		vb++;
	}

	D3D_OutputTriangles();
}

void D3D_Rectangle(int x0, int y0, int x1, int y1, int r, int g, int b, int a) 
{

	float new_x1, new_y1, new_x2, new_y2;

	if (mainMenu)
	{
		new_x1 = (float(x0 / 640.0f) * 2) - 1;
		new_y1 = (float(y0 / 480.0f) * 2) - 1;

		new_x2 = ((float(x1) / 640.0f) * 2) - 1;
		new_y2 = ((float(y1) / 480.0f) * 2) - 1;
	}
	else
	{
		new_x1 = (float(x0 / (float)ScreenDescriptorBlock.SDB_Width) * 2) - 1;
		new_y1 = (float(y0 / (float)ScreenDescriptorBlock.SDB_Height) * 2) - 1;

		new_x2 = ((float(x1) / (float)ScreenDescriptorBlock.SDB_Width) * 2) - 1;
		new_y2 = ((float(y1) / (float)ScreenDescriptorBlock.SDB_Height) * 2) - 1;
	}

	CheckOrthoBuffer(4, NO_TEXTURE, TRANSLUCENCY_GLOWING, TEXTURE_CLAMP);

	// bottom left
	orthoVerts[orthoVBOffset].x = new_x1;
	orthoVerts[orthoVBOffset].y = new_y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(r,g,b,a);
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = new_x1;
	orthoVerts[orthoVBOffset].y = new_y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(r,g,b,a);
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = new_x2;
	orthoVerts[orthoVBOffset].y = new_y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(r,g,b,a);
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = new_x2;
	orthoVerts[orthoVBOffset].y = new_y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(r,g,b,a);
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;
}

void D3D_HUD_Setup(void)
{
	if (D3DZFunc != D3DCMP_LESSEQUAL)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		D3DZFunc = D3DCMP_LESSEQUAL;
	}
}

inline float WPos2DC(int pos)
{
	return (float(pos / (float)ScreenDescriptorBlock.SDB_Width) * 2) - 1;
}

inline float HPos2DC(int pos)
{
	return (float(pos / (float)ScreenDescriptorBlock.SDB_Height) * 2) - 1;
}

void D3D_HUDQuad_Output(int32_t textureID, struct VertexTag *quadVerticesPtr, uint32_t colour, enum FILTERING_MODE_ID filteringType)
{
	float RecipW, RecipH;

	// ugh..
	if (textureID >= texIDoffset)
	{
		uint32_t texWidth, texHeight;

		Tex_GetDimensions(textureID, texWidth, texHeight);

		RecipW = 1.0f / texWidth;
		RecipH = 1.0f / texHeight;
	}
	else
	{
		RecipW = 1.0f / ImageHeaderArray[textureID].ImageWidth;
		RecipH = 1.0f / ImageHeaderArray[textureID].ImageHeight;
	}

	CheckOrthoBuffer(4, textureID, TRANSLUCENCY_GLOWING, TEXTURE_CLAMP, filteringType);

	// bottom left
	orthoVerts[orthoVBOffset].x = WPos2DC(quadVerticesPtr[3].X);
	orthoVerts[orthoVBOffset].y = HPos2DC(quadVerticesPtr[3].Y);
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = quadVerticesPtr[3].U * RecipW;
	orthoVerts[orthoVBOffset].v = quadVerticesPtr[3].V * RecipH;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = WPos2DC(quadVerticesPtr[0].X);
	orthoVerts[orthoVBOffset].y = HPos2DC(quadVerticesPtr[0].Y);
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = quadVerticesPtr[0].U * RecipW;
	orthoVerts[orthoVBOffset].v = quadVerticesPtr[0].V * RecipH;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = WPos2DC(quadVerticesPtr[2].X);
	orthoVerts[orthoVBOffset].y = HPos2DC(quadVerticesPtr[2].Y);
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = quadVerticesPtr[2].U * RecipW;
	orthoVerts[orthoVBOffset].v = quadVerticesPtr[2].V * RecipH;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = WPos2DC(quadVerticesPtr[1].X);
	orthoVerts[orthoVBOffset].y = HPos2DC(quadVerticesPtr[1].Y);
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = quadVerticesPtr[1].U * RecipW;
	orthoVerts[orthoVBOffset].v = quadVerticesPtr[1].V * RecipH;
	orthoVBOffset++;
}

void D3D_DrawParticle_Rain(PARTICLE *particlePtr, VECTORCH *prevPositionPtr)
{
	VECTORCH vertices[3];
	vertices[0] = *prevPositionPtr;

	/* translate second vertex into view space */
	TranslatePointIntoViewspace(&vertices[0]);

	/* is particle within normal view frustrum ? */
	if ((-vertices[0].vx <= vertices[0].vz)
	&& (vertices[0].vx <= vertices[0].vz)
	&& (-vertices[0].vy <= vertices[0].vz)
	&& (vertices[0].vy <= vertices[0].vz))
	{

		vertices[1] = particlePtr->Position;
		vertices[2] = particlePtr->Position;
		vertices[1].vx += particlePtr->Offset.vx;
		vertices[2].vx -= particlePtr->Offset.vx;
		vertices[1].vz += particlePtr->Offset.vz;
		vertices[2].vz -= particlePtr->Offset.vz;

		/* translate particle into view space */
		TranslatePointIntoViewspace(&vertices[1]);
		TranslatePointIntoViewspace(&vertices[2]);

//		float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

		CheckVertexBuffer(3, NO_TEXTURE, TRANSLUCENCY_NORMAL);

		VECTORCH *verticesPtr = vertices;

		for (uint32_t i = 0; i < 3; i++)
		{
			mainVertex[vb].sx = (float)verticesPtr->vx;
			mainVertex[vb].sy = (float)-verticesPtr->vy;
			mainVertex[vb].sz = (float)verticesPtr->vz; // bjd - CHECK

			if (i==3) 
				mainVertex[vb].color = RGBALIGHT_MAKE(0, 255, 255, 32);
			else 
				mainVertex[vb].color = RGBALIGHT_MAKE(255, 255, 255, 32);

			mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
			mainVertex[vb].tu = 0.0f;
			mainVertex[vb].tv = 0.0f;

			vb++;
			verticesPtr++;
		}

		OUTPUT_TRIANGLE(0,2,1, 3);
	}
}

void D3D_DrawParticle_Smoke(PARTICLE *particlePtr)
{
	VECTORCH vertices[3];
	vertices[0] = particlePtr->Position;

	/* translate second vertex into view space */
	TranslatePointIntoViewspace(&vertices[0]);

	/* is particle within normal view frustrum ? */
	int inView = 0;

	if (AvP.PlayerType == I_Alien)
	{
		if ((-vertices[0].vx <= vertices[0].vz*2)
		&&(vertices[0].vx <= vertices[0].vz*2)
		&&(-vertices[0].vy <= vertices[0].vz*2)
		&&(vertices[0].vy <= vertices[0].vz*2))
		{
			inView = 1;
		}
	}
	else
	{
		if ((-vertices[0].vx <= vertices[0].vz)
		&&(vertices[0].vx <= vertices[0].vz)
		&&(-vertices[0].vy <= vertices[0].vz)
		&&(vertices[0].vy <= vertices[0].vz))
		{
			inView = 1;
		}
	}

	if (inView)
	{
		vertices[1] = particlePtr->Position;
		vertices[2] = particlePtr->Position;
		vertices[1].vx += ((FastRandom()&15)-8)*2;
		vertices[1].vy += ((FastRandom()&15)-8)*2;
		vertices[1].vz += ((FastRandom()&15)-8)*2;
		vertices[2].vx += ((FastRandom()&15)-8)*2;
		vertices[2].vy += ((FastRandom()&15)-8)*2;
		vertices[2].vz += ((FastRandom()&15)-8)*2;

		/* translate particle into view space */
		TranslatePointIntoViewspace(&vertices[1]);
		TranslatePointIntoViewspace(&vertices[2]);

		float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

		CheckVertexBuffer(3, NO_TEXTURE, TRANSLUCENCY_NORMAL);

		{
			VECTORCH *verticesPtr = vertices;

			for (uint32_t i = 0; i < 3; i++)
			{
				mainVertex[vb].sx = (float)verticesPtr->vx;
				mainVertex[vb].sy = (float)-verticesPtr->vy;
				mainVertex[vb].sz = (float)vertices->vz;

				mainVertex[vb].color = RGBALIGHT_MAKE((particlePtr->LifeTime>>8),(particlePtr->LifeTime>>8),0,(particlePtr->LifeTime>>7)+64);

				mainVertex[vb].specular = (D3DCOLOR)1.0f;
				mainVertex[vb].tu = 0.0f;
				mainVertex[vb].tv = 0.0f;

				vb++;
				verticesPtr++;
			}
		}
		OUTPUT_TRIANGLE(0,2,1, 3);
	}
}

void D3D_DecalSystem_Setup(void)
{
/*
	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	LockExecuteBuffer();
*/
	if (D3DDitherEnable != FALSE)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_DITHERENABLE,FALSE);
		D3DDitherEnable = FALSE;
	}

	if (D3DZWriteEnable != FALSE)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
		D3DZWriteEnable = FALSE;
	}
}

void D3D_DecalSystem_End(void)
{
	DrawParticles();
/*
	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	LockExecuteBuffer();
*/
	if (D3DDitherEnable != TRUE)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_DITHERENABLE,TRUE);
		D3DDitherEnable = TRUE;
	}

	if (D3DZWriteEnable != TRUE)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
		D3DZWriteEnable = TRUE;
	}
}

void D3D_Decal_Output(DECAL *decalPtr,RENDERVERTEX *renderVerticesPtr)
{
	// function responsible for bullet marks on walls, etc
	DECAL_DESC *decalDescPtr = &DecalDescription[decalPtr->DecalID];

	int32_t textureID;

	AVPTEXTURE *textureHandle = NULL;

	float ZNear;
	float RecipW, RecipH;
	int colour;
	int specular = RGBALIGHT_MAKE(0, 0, 0, 0);

    // Get ZNear
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	if (decalPtr->DecalID == DECAL_FMV)
	{
		#if !FMV_ON
		return;
		#endif

//		textureHandle = FMVTextureHandle[decalPtr->Centre.vx];

		RecipW = 1.0f / 128.0f;
		RecipH = 1.0f / 128.0f;
/*
	    if (TextureHandle != CurrTextureHandle)
		{
			d3d.lpD3DDevice->SetTexture(0,TextureHandle);
		   	CurrTextureHandle = TextureHandle;
		}
*/
		textureID = NO_TEXTURE;
	}

	else if (decalPtr->DecalID == DECAL_SHAFTOFLIGHT || decalPtr->DecalID == DECAL_SHAFTOFLIGHT_OUTER)
	{
		textureID = NO_TEXTURE;
	}
	else
	{
		RecipW = 1.0f / (float) ImageHeaderArray[SpecialFXImageNumber].ImageWidth;
		RecipH = 1.0f / (float) ImageHeaderArray[SpecialFXImageNumber].ImageHeight;

		textureID = SpecialFXImageNumber;
	}

	if (decalDescPtr->IsLit)
	{
		int intensity = LightIntensityAtPoint(decalPtr->Vertices);
		colour = RGBALIGHT_MAKE
	  		  	(
	  		   		MUL_FIXED(intensity,decalDescPtr->RedScale[CurrentVisionMode]),
	  		   		MUL_FIXED(intensity,decalDescPtr->GreenScale[CurrentVisionMode]),
	  		   		MUL_FIXED(intensity,decalDescPtr->BlueScale[CurrentVisionMode]),
	  		   		decalDescPtr->Alpha
	  		   	);
	}
	else
	{
		colour = RGBALIGHT_MAKE
			  	(
			   		decalDescPtr->RedScale[CurrentVisionMode],
			   		decalDescPtr->GreenScale[CurrentVisionMode],
			   		decalDescPtr->BlueScale[CurrentVisionMode],
			   		decalDescPtr->Alpha
			   	);
	}

	if (RAINBOWBLOOD_CHEATMODE)
	{
		colour = RGBALIGHT_MAKE
				(
					FastRandom()&255,
					FastRandom()&255,
					FastRandom()&255,
					decalDescPtr->Alpha
				);
	}

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, textureID, decalDescPtr->TranslucencyType);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z + HeadUpDisplayZOffset;// - 50;

		mainVertex[vb].color = colour;
		mainVertex[vb].specular = specular;

		mainVertex[vb].tu = (float)(vertices->U) * RecipW;
		mainVertex[vb].tv = (float)(vertices->V) * RecipH;

		vb++;
	}

	D3D_OutputTriangles();
}

void AddParticle(PARTICLE *particlePtr, RENDERVERTEX *renderVerticesPtr)
{
	renderParticle tempParticle;

	tempParticle.numVerts = RenderPolygon.NumberOfVertices;
	tempParticle.particle = *particlePtr;

	memcpy(&tempParticle.vertices[0], renderVerticesPtr, tempParticle.numVerts * sizeof(RENDERVERTEX));
	tempParticle.translucency = ParticleDescription[particlePtr->ParticleID].TranslucencyType;

	particleArray.push_back(tempParticle);
}

void D3D_PointSpriteTest(PARTICLE *particlePtr, RENDERVERTEX *renderVerticesPtr)
{
/*
	testPS[psIndex].x = particlePtr->Position.vx;
	testPS[psIndex].y = particlePtr->Position.vy;
	testPS[psIndex].z = particlePtr->Position.vz;

	testPS[psIndex].size = 64.0f;

	testPS[psIndex].colour = D3DCOLOR_XRGB(128, 0, 128);
	testPS[psIndex].u = 0.0f;
	testPS[psIndex].v = 0.0f;

	psIndex++;
*/
}

void D3D_Particle_Output(PARTICLE *particlePtr, RENDERVERTEX *renderVerticesPtr)
{
	// steam jets, wall lights, fire (inc aliens on fire) etc
	PARTICLE_DESC *particleDescPtr = &ParticleDescription[particlePtr->ParticleID];

	float RecipW = 1.0f / (float) ImageHeaderArray[SpecialFXImageNumber].ImageWidth;
	float RecipH = 1.0f / (float) ImageHeaderArray[SpecialFXImageNumber].ImageHeight;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, SpecialFXImageNumber, particleDescPtr->TranslucencyType);
/*
	char buf[100];
	sprintf(buf, "trans type: %d\n", particleDescPtr->TranslucencyType);
	OutputDebugString(buf);
*/
	int colour;

	if (particleDescPtr->IsLit && !(particlePtr->ParticleID == PARTICLE_ALIEN_BLOOD && CurrentVisionMode == VISION_MODE_PRED_SEEALIENS))
	{
		int intensity = LightIntensityAtPoint(&particlePtr->Position);
		if (particlePtr->ParticleID == PARTICLE_SMOKECLOUD || particlePtr->ParticleID == PARTICLE_ANDROID_BLOOD)
		{
			colour = RGBALIGHT_MAKE
				  	(
				   		MUL_FIXED(intensity,particlePtr->ColourComponents.Red),
				   		MUL_FIXED(intensity,particlePtr->ColourComponents.Green),
				   		MUL_FIXED(intensity,particlePtr->ColourComponents.Blue),
				   		particlePtr->ColourComponents.Alpha
				   	);
		}
		else
		{
			colour = RGBALIGHT_MAKE
				  	(
				   		MUL_FIXED(intensity,particleDescPtr->RedScale[CurrentVisionMode]),
				   		MUL_FIXED(intensity,particleDescPtr->GreenScale[CurrentVisionMode]),
				   		MUL_FIXED(intensity,particleDescPtr->BlueScale[CurrentVisionMode]),
				   		particleDescPtr->Alpha
				   	);
		}
	}
	else
	{
		colour = particlePtr->Colour;
	}

	if (RAINBOWBLOOD_CHEATMODE)
	{
		colour = RGBALIGHT_MAKE
					(
						FastRandom()&255,
						FastRandom()&255,
						FastRandom()&255,
						particleDescPtr->Alpha
					);
	}
		float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

		{
			for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
			{
				RENDERVERTEX *vertices = &renderVerticesPtr[i];
			
				float zvalue;

				mainVertex[vb].sx = (float)vertices->X;
				mainVertex[vb].sy = (float)-vertices->Y;
				mainVertex[vb].sz = (float)vertices->Z;

				mainVertex[vb].color = colour;
	 			mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);

				mainVertex[vb].tu = (float)(vertices->U) * RecipW;
				mainVertex[vb].tv = (float)(vertices->V) * RecipH;

				vb++;
			}

		D3D_OutputTriangles();
	}
}

void D3D_FMVParticle_Output(RENDERVERTEX *renderVerticesPtr)
{
#if 0 // bjd
	D3DTEXTUREHANDLE TextureHandle = FMVTextureHandle[0];
	float RecipW, RecipH;

	RecipW = 1.0 /128.0;
	RecipH = 1.0 /128.0;

	int colour = FMVParticleColour&0xffffff;

//	D3DTLVERTEX *vertexPtr = new D3DTLVERTEX;// = &((LPD3DTLVERTEX)ExecuteBufferDataArea)[NumVertices];

	{
		float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);
		{
			int i = RenderPolygon.NumberOfVertices;
			RENDERVERTEX *vertices = renderVerticesPtr;

			do
			{
				//D3DTLVERTEX *vertexPtr = &((LPD3DTLVERTEX)ExecuteBufferDataArea)[NumVertices];
			  	float oneOverZ = (1.0)/vertices->Z;
				float zvalue;

				{
					int x = (vertices->X*(Global_VDB_Ptr->VDB_ProjX+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreX;

					if (x<Global_VDB_Ptr->VDB_ClipLeft)
					{
						x=Global_VDB_Ptr->VDB_ClipLeft;
					}
					else if (x>Global_VDB_Ptr->VDB_ClipRight)
					{
						x=Global_VDB_Ptr->VDB_ClipRight;
					}

					mainVertex->sx=x;
				}
				{
					int y = (vertices->Y*(Global_VDB_Ptr->VDB_ProjY+1))/vertices->Z+Global_VDB_Ptr->VDB_CentreY;

					if (y<Global_VDB_Ptr->VDB_ClipUp)
					{
						y=Global_VDB_Ptr->VDB_ClipUp;
					}
					else if (y>Global_VDB_Ptr->VDB_ClipDown)
					{
						y=Global_VDB_Ptr->VDB_ClipDown;
					}
					mainVertex->sy=y;

				}
				mainVertex->tu = ((float)(vertices->U>>16)) * RecipW;
				mainVertex->tv = ((float)(vertices->V>>16)) * RecipH;
				mainVertex->rhw = oneOverZ;
				zvalue = 1.0 - ZNear*oneOverZ;

//				vertexPtr->color = colour;
				mainVertex->color = (colour)+(vertices->A<<24);
				mainVertex->sz = zvalue;
	 		   	mainVertex->specular=RGBALIGHT_MAKE(0,0,0,255);//RGBALIGHT_MAKE(vertices->SpecularR,vertices->SpecularG,vertices->SpecularB,fog);

	 			NumVertices++;
				vertices++;

//				vertexPtr++; // increment
				mainVertex++;
			}
		  	while(--i);
		}

		// set correct texture handle
	    if (TextureHandle != CurrTextureHandle)
		{
			d3d.lpD3DDevice->SetTexture(0,TextureHandle);
			CurrTextureHandle = TextureHandle;
		}

		CheckTranslucencyModeIsCorrect(TRANSLUCENCY_NORMAL);

	    if (D3DTexturePerspective != TRUE)
	    {
			D3DTexturePerspective = TRUE;
			//OP_STATE_RENDER(1, ExecBufInstPtr);
			//STATE_DATA(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE, ExecBufInstPtr);
		}

//		d3d.lpD3DDevice->SetVertexShader(D3DFVF_TLVERTEX);
//		d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST,NumVertices/3,vertexPtr,sizeof(D3DTLVERTEX));

		D3D_OutputTriangles();

	}
#endif
}

void PostLandscapeRendering()
{
	int numOfObjects = NumOnScreenBlocks;

  	CurrentRenderStates.FogIsOn = 1;

	if (!strcmp(LevelName,"fall")||!strcmp(LevelName,"fall_m"))
	{
		char drawWaterFall = 0;
		char drawStream = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if( (!strcmp(modulePtr->name,"fall01"))
				  ||(!strcmp(modulePtr->name,"well01"))
				  ||(!strcmp(modulePtr->name,"well02"))
				  ||(!strcmp(modulePtr->name,"well03"))
				  ||(!strcmp(modulePtr->name,"well04"))
				  ||(!strcmp(modulePtr->name,"well05"))
				  ||(!strcmp(modulePtr->name,"well06"))
				  ||(!strcmp(modulePtr->name,"well07"))
				  ||(!strcmp(modulePtr->name,"well08"))
				  ||(!strcmp(modulePtr->name,"well")))
				{
					drawWaterFall = 1;
				}
				else if( (!strcmp(modulePtr->name,"stream02"))
				       ||(!strcmp(modulePtr->name,"stream03"))
				       ||(!strcmp(modulePtr->name,"watergate")))
				{
		   			drawStream = 1;
				}
			}
		}

		if (drawWaterFall)
		{
	   		//UpdateWaterFall();
			WaterFallBase = 109952;

			MeshZScale = (66572-51026)/15;
			MeshXScale = (109952+3039)/45;

	   		D3D_DrawWaterFall(175545,-3039,51026);
		}
		if (drawStream)
		{
			int x = 68581;
			int y = 12925;
			int z = 93696;
			MeshXScale = (87869-68581);
			MeshZScale = (105385-93696);
			{
				CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;

			currentWaterTexture = ChromeImageNumber;

		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
	}
	else if (!_stricmp(LevelName,"hangar"))
	{
		// unused code removed
	}
	else if (!_stricmp(LevelName,"invasion_a"))
	{
		char drawWater = 0;
		char drawEndWater = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if( (!strcmp(modulePtr->name,"hivepool"))
				  ||(!strcmp(modulePtr->name,"hivepool04")))
				{
					drawWater = 1;
					break;
				}
				else
				{
					if (!strcmp(modulePtr->name,"shaftbot"))
					{
						drawEndWater = 1;
					}
					if ((!_stricmp(modulePtr->name,"shaft01"))
					 ||(!_stricmp(modulePtr->name,"shaft02"))
					 ||(!_stricmp(modulePtr->name,"shaft03"))
					 ||(!_stricmp(modulePtr->name,"shaft04"))
					 ||(!_stricmp(modulePtr->name,"shaft05"))
					 ||(!_stricmp(modulePtr->name,"shaft06")))
					{
						HandleRainShaft(modulePtr, -11726,-107080,10);
						drawEndWater = 1;
						break;
					}
				}
			}
		}

		if (drawWater)
		{
			int x = 20767;
			int y = -36000+200;
			int z = 30238;
			MeshXScale = (36353-20767);
			MeshZScale = (41927-30238);
			{
				CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;

			currentWaterTexture = ChromeImageNumber;

		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
		else if (drawEndWater)
		{
			int x = -15471;
			int y = -11720-500;
			int z = -55875;
			MeshXScale = (15471-1800);
			MeshZScale = (55875-36392);
			{
				CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
			}
			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)(MeshXScale+1800-3782);
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=4;
		 	MeshZScale/=2;

			currentWaterTexture = WaterShaftImageNumber;

		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*2, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale*3, y, z+MeshZScale);
		}
	}
	else if (!_stricmp(LevelName,"derelict"))
	{
		char drawMirrorSurfaces = 0;
		char drawWater = 0;

		while(numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
			  	if ((!_stricmp(modulePtr->name,"start-en01"))
			  	  ||(!_stricmp(modulePtr->name,"start")))
				{
					drawMirrorSurfaces = 1;
				}
				else if (!_stricmp(modulePtr->name,"water-01"))
				{
					drawWater = 1;
					HandleRainShaft(modulePtr, 32000, 0, 16);
				}
			}
		}

		if (drawMirrorSurfaces)
		{
			RenderParticlesInMirror();
			RenderMirrorSurface();
			RenderMirrorSurface2();
		}
		if (drawWater)
		{
			int x = -102799;
			int y = 32000;
			int z = -200964;
			MeshXScale = (102799-87216);
			MeshZScale = (200964-180986);
			{
				CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
			}

			WaterXOrigin=x;
			WaterZOrigin=z;
			WaterUScale = 4.0f/(float)MeshXScale;
			WaterVScale = 4.0f/(float)MeshZScale;
		 	MeshXScale/=2;
		 	MeshZScale/=2;

			currentWaterTexture = ChromeImageNumber;

		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		}

	}
	else if (!_stricmp(LevelName,"genshd1"))
	{
		char drawWater = 0;

		while (numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
			if (modulePtr && modulePtr->name)
			{
				if ((!_stricmp(modulePtr->name,"largespace"))
				  ||(!_stricmp(modulePtr->name,"proc13"))
				  ||(!_stricmp(modulePtr->name,"trench01"))
				  ||(!_stricmp(modulePtr->name,"trench02"))
				  ||(!_stricmp(modulePtr->name,"trench03"))
				  ||(!_stricmp(modulePtr->name,"trench04"))
				  ||(!_stricmp(modulePtr->name,"trench05"))
				  ||(!_stricmp(modulePtr->name,"trench06"))
				  ||(!_stricmp(modulePtr->name,"trench07"))
				  ||(!_stricmp(modulePtr->name,"trench08"))
				  ||(!_stricmp(modulePtr->name,"trench09")))
				{
					HandleRain(999);
					break;
				}
			}
		}
	}
}

void D3D_DrawWaterTest(MODULE *testModulePtr)
{
	extern char LevelName[];
	if (!strcmp(LevelName,"genshd1"))
	{
		extern DISPLAYBLOCK *Player;

		MODULE *modulePtr = testModulePtr;

		if (modulePtr && modulePtr->name)
		{
			if (!strcmp(modulePtr->name,"05"))
			{
				int y = modulePtr->m_maxy+modulePtr->m_world.vy-500;
		   		int x = modulePtr->m_minx+modulePtr->m_world.vx;
		   		int z = modulePtr->m_minz+modulePtr->m_world.vz;
				MeshXScale = (7791 - -7794);
				MeshZScale = (23378 - 7793);
				{
					CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);
				}

				currentWaterTexture = WaterShaftImageNumber;

				WaterXOrigin=x;
				WaterZOrigin=z;
				WaterUScale = 4.0f/(float)(MeshXScale);
				WaterVScale = 4.0f/(float)MeshZScale;

				MeshXScale/=2;
				MeshZScale/=2;
				D3D_DrawWaterPatch(x, y, z);
				D3D_DrawWaterPatch(x+MeshXScale, y, z);
				D3D_DrawWaterPatch(x, y, z+MeshZScale);
				D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);

				HandleRainShaft(modulePtr, y,-21000,1);
			}
		}
	}
}

void D3D_DrawWaterPatch(int xOrigin, int yOrigin, int zOrigin)
{
	int i = 0;

	for (int x = 0; x < 16; x++)
	{
		for (int z = 0; z < 16; z++)
		{
			VECTORCH *point = &MeshVertex[i];
			
			point->vx = xOrigin+(x*MeshXScale)/15;
			point->vz = zOrigin+(z*MeshZScale)/15;


			int offset = 0;

			/* basic noise ripples */
//		 	offset = MUL_FIXED(32,GetSin(  (point->vx+point->vz+CloakingPhase)&4095 ) );
//		 	offset += MUL_FIXED(16,GetSin(  (point->vx-point->vz*2+CloakingPhase/2)&4095 ) );

			{
 				offset += EffectOfRipples(point);
			}
	//		if (offset>450) offset = 450;
	//		if (offset<-450) offset = -450;
			point->vy = yOrigin+offset;

			{
				int alpha = 128-offset/4;
		//		if (alpha>255) alpha = 255;
		//		if (alpha<128) alpha = 128;
				switch (CurrentVisionMode)
				{
					default:
					case VISION_MODE_NORMAL:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(255,255,255,alpha);
						break;
					}
					case VISION_MODE_IMAGEINTENSIFIER:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(0,51,0,alpha);
						break;
					}
					case VISION_MODE_PRED_THERMAL:
					case VISION_MODE_PRED_SEEALIENS:
					case VISION_MODE_PRED_SEEPREDTECH:
					{
						MeshVertexColour[i] = RGBALIGHT_MAKE(0,0,28,alpha);
					  	break;
					}
				}
			}

			MeshWorldVertex[i].vx = ((point->vx-WaterXOrigin)/4+MUL_FIXED(GetSin((point->vy*16)&4095),128));			
			MeshWorldVertex[i].vy = ((point->vz-WaterZOrigin)/4+MUL_FIXED(GetSin((point->vy*16+200)&4095),128));			
			
			TranslatePointIntoViewspace(point);

			/* is particle within normal view frustrum ? */
			if(AvP.PlayerType==I_Alien)	/* wide frustrum */
			{
				if(( (-point->vx <= point->vz*2)
		   			&&(point->vx <= point->vz*2)
					&&(-point->vy <= point->vz*2)
					&&(point->vy <= point->vz*2) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}
			else
			{
				if(( (-point->vx <= point->vz)
		   			&&(point->vx <= point->vz)
					&&(-point->vy <= point->vz)
					&&(point->vy <= point->vz) ))
				{
					MeshVertexOutcode[i]=1;
				}
				else
				{
					MeshVertexOutcode[i]=0;
				}
			}

			i++;
		}
	}

	D3D_DrawMoltenMetalMesh_Unclipped();	
}

int LightSourceWaterPoint(VECTORCH *pointPtr, int offset)
{
	// this needs a rewrite...
	// make a list of lights which will affect some part of the mesh
	// and go throught that for each point.

   	int redI=0,greenI=0,blueI=0;

	DISPLAYBLOCK **activeBlockListPtr = ActiveBlockList;
	for (int i = NumActiveBlocks; i!=0; i--)
	{
		DISPLAYBLOCK *dispPtr = *activeBlockListPtr++;

		if(dispPtr->ObNumLights)
		{
			for(int j = 0; j < dispPtr->ObNumLights; j++)
			{
				LIGHTBLOCK *lptr = dispPtr->ObLights[j];

				VECTORCH disp = lptr->LightWorld;
				disp.vx -= pointPtr->vx;
				disp.vy -= pointPtr->vy;
				disp.vz -= pointPtr->vz;

				int dist = Approximate3dMagnitude(&disp);

				if (dist<lptr->LightRange)
				{
					int brightness = MUL_FIXED(lptr->BrightnessOverRange,lptr->LightRange-dist);
					redI += MUL_FIXED(brightness,lptr->RedScale);
					greenI += MUL_FIXED(brightness,lptr->GreenScale);
					blueI += MUL_FIXED(brightness,lptr->BlueScale);
				}
			}
		}
	}

	if (redI>ONE_FIXED) redI=ONE_FIXED;
	else if (redI<GlobalAmbience) redI=GlobalAmbience;
	if (greenI>ONE_FIXED) greenI=ONE_FIXED;
	else if (greenI<GlobalAmbience) greenI=GlobalAmbience;
 	if (blueI>ONE_FIXED) blueI=ONE_FIXED;
	else if (blueI<GlobalAmbience) blueI=GlobalAmbience;

	int alpha = 192-offset/4;
	if (alpha>255) alpha = 255;
	if (alpha<128) alpha = 128;

	return RGBALIGHT_MAKE(MUL_FIXED(10,redI),MUL_FIXED(51,greenI),MUL_FIXED(28,blueI),alpha);
}

int LightIntensityAtPoint(VECTORCH *pointPtr)
{
	int intensity=0;

	DISPLAYBLOCK **activeBlockListPtr = ActiveBlockList;
	for (int i = NumActiveBlocks; i!=0; i--)
	{
		DISPLAYBLOCK *dispPtr = *activeBlockListPtr++;

		if (dispPtr->ObNumLights)
		{
			for (int j = 0; j < dispPtr->ObNumLights; j++)
			{
				LIGHTBLOCK *lptr = dispPtr->ObLights[j];

				VECTORCH disp = lptr->LightWorld;
				disp.vx -= pointPtr->vx;
				disp.vy -= pointPtr->vy;
				disp.vz -= pointPtr->vz;

				int dist = Approximate3dMagnitude(&disp);

				if (dist<lptr->LightRange)
				{
					intensity += WideMulNarrowDiv(lptr->LightBright,lptr->LightRange-dist,lptr->LightRange);
				}
			}
		}
	}
	if (intensity>ONE_FIXED) intensity=ONE_FIXED;
	else if (intensity<GlobalAmbience) intensity=GlobalAmbience;

	/* KJL 20:31:39 12/1/97 - limit how dark things can be so blood doesn't go green */
	if (intensity<10*256) intensity = 10*256;

	return intensity;
}

signed int ForceFieldPointDisplacement[15*3+1][16];
signed int ForceFieldPointDisplacement2[15*3+1][16];
signed int ForceFieldPointVelocity[15*3+1][16];
unsigned char ForceFieldPointColour1[15*3+1][16];
unsigned char ForceFieldPointColour2[15*3+1][16];

int Phase = 0;
int ForceFieldPhase = 0;

void InitForceField(void)
{
	for (int x=0; x<15*3+1; x++)
		for (int y=0; y<16; y++)
		{
			ForceFieldPointDisplacement[x][y]=0;
			ForceFieldPointDisplacement2[x][y]=0;
			ForceFieldPointVelocity[x][y]=0;
		}
	ForceFieldPhase=0;
}

void UpdateWaterFall(void)
{
	int x;
	int y;
	for (y=0; y<=15; y++)
	{
		ForceFieldPointDisplacement[0][y] += (FastRandom()&127)-64;
		if(ForceFieldPointDisplacement[0][y]>512) ForceFieldPointDisplacement[0][y]=512;
		if(ForceFieldPointDisplacement[0][y]<-512) ForceFieldPointDisplacement[0][y]=-512;
		ForceFieldPointVelocity[0][y] = (FastRandom()&16383)-8192;
	}
	for (x=15*3-1; x>0; x--)
	{
		for (y=0; y<=15; y++)
		{
			ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
			ForceFieldPointVelocity[x][y] = ForceFieldPointVelocity[x-1][y];
		}

	}
	for (x=15*3-1; x>1; x--)
	{
		y = FastRandom()&15;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
		y = (FastRandom()&15)-1;
	 	ForceFieldPointDisplacement[x][y] = ForceFieldPointDisplacement[x-1][y];
	}
}

void D3D_DrawWaterFall(int xOrigin, int yOrigin, int zOrigin)
{
	uint32_t noRequired = MUL_FIXED(250, NormalFrameTime);

	for (uint32_t i = 0; i < noRequired; i++)
	{
		VECTORCH velocity;
		VECTORCH position;
		position.vx = xOrigin;
		position.vy = yOrigin-(FastRandom()&511);//+45*MeshXScale;
		position.vz = zOrigin+(FastRandom()%(15*MeshZScale));

		velocity.vy = (FastRandom()&511)+512;//-((FastRandom()&1023)+2048)*8;
		velocity.vx = ((FastRandom()&511)+256)*2;
		velocity.vz = 0;//-((FastRandom()&511))*8;
		MakeParticle(&(position), &velocity, PARTICLE_WATERFALLSPRAY);
	}
}

void DrawFrameRateBar(void)
{
	extern int NormalFrameTime;

	{
		int width = DIV_FIXED(ScreenDescriptorBlock.SDB_Width/120,NormalFrameTime);
		if (width>ScreenDescriptorBlock.SDB_Width) width=ScreenDescriptorBlock.SDB_Width;

		r2rect rectangle
		(
			0,0,
			width,
			24
		);
//		textprint("width %d\n",width);

		rectangle . AlphaFill
		(
			0xff, // unsigned char R,
			0x00,// unsigned char G,
			0x00,// unsigned char B,
		   	128 // unsigned char translucency
		);
	}

}

void D3D_DrawAlienRedBlipIndicatingJawAttack(void)
{
	return;
	r2rect rectangle
	(
		16,16,
		32,
		32
	);

	rectangle . AlphaFill
	(
		0xff, // unsigned char R,
		0x00,// unsigned char G,
		0x00,// unsigned char B,
	   	128 // unsigned char translucency
	);
}

void D3D_DrawBackdrop(void)
{
	if (TRIPTASTIC_CHEATMODE||MOTIONBLUR_CHEATMODE) 
		return;

	if (WireFrameMode)
	{
   		ColourFillBackBuffer(0);
		return;
	}
	else if(ShowDebuggingText.Tears)
	{
		ColourFillBackBuffer((63<<5));
		return;
	}

	{
		int needToDrawBackdrop=0;
		extern int NumActiveBlocks;
		extern DISPLAYBLOCK *ActiveBlockList[];

		int numOfObjects = NumActiveBlocks;
		while (numOfObjects--)
		{
			DISPLAYBLOCK *objectPtr = ActiveBlockList[numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			if (modulePtr && (ModuleCurrVisArray[modulePtr->m_index] == 2) && modulePtr->m_flags&MODULEFLAG_SKY)
			{
				needToDrawBackdrop=1;
				break;
			}
		}
		if (needToDrawBackdrop)
		{
			ColourFillBackBuffer(0);

			if (LevelHasStars)
			{
				RenderStarfield();
			}
			else
			{
		  		RenderSky();
			}
			return;
		}
	}

	/* if the player is outside the environment, clear the screen! */
	{
 		if (!playerPherModule)
 		{
 			ColourFillBackBuffer(0);
			return;
		}
	}
	{
		PLAYER_STATUS *playerStatusPtr= (PLAYER_STATUS *) (Player->ObStrategyBlock->SBdataptr);

		if (!playerStatusPtr->IsAlive || FREEFALL_CHEATMODE)
		{
			// minimise effects of camera glitches
			ColourFillBackBuffer(0);
			return;
		}
	}
}

void DrawNoiseOverlay(int t)
{
	// t == 64 for image intensifier
	float u = (float)(FastRandom()&255);
	float v = (float)(FastRandom()&255);
	int c = 255;
	int size = 256;//*CameraZoomScale;

	CheckOrthoBuffer(4, StaticImageNumber, TRANSLUCENCY_GLOWING, TEXTURE_WRAP);

	// bottom left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(c,c,c,t);
	orthoVerts[orthoVBOffset].u = u/256.0f;
	orthoVerts[orthoVBOffset].v = (v+size)/256.0f;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(c,c,c,t);
	orthoVerts[orthoVBOffset].u = u/256.0f;
	orthoVerts[orthoVBOffset].v = v/256.0f;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = 1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(c,c,c,t);
	orthoVerts[orthoVBOffset].u = (u+size)/256.0f;
	orthoVerts[orthoVBOffset].v = (v+size)/256.0f;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = 1.0f;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(c,c,c,t);
	orthoVerts[orthoVBOffset].u = (u+size)/256.0f;
	orthoVerts[orthoVBOffset].v = v/256.0f;
	orthoVBOffset++;
}

void DrawScanlinesOverlay(float level)
{
	float u = 0.0f;//FastRandom()&255;
	float v = 128.0f;//FastRandom()&255;
	int c = 255;
	int t;
   	f2i(t,64.0f+level*64.0f);

	float size = 128.0f*(1.0f-level*0.8f);//*CameraZoomScale;

	CheckOrthoBuffer(4, PredatorNumbersImageNumber, TRANSLUCENCY_NORMAL, TEXTURE_WRAP);

	// bottom left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(c,c,c,t);
	orthoVerts[orthoVBOffset].u = (v+size)/256.0f;
	orthoVerts[orthoVBOffset].v = 1.0f;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(c,c,c,t);
	orthoVerts[orthoVBOffset].u = (v-size)/256.0f;
	orthoVerts[orthoVBOffset].v = 1.0f;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = 1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(c,c,c,t);
	orthoVerts[orthoVBOffset].u = (v+size)/256.0f;
	orthoVerts[orthoVBOffset].v = 1.0f;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = 1.0f;;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = RGBALIGHT_MAKE(c,c,c,t);
	orthoVerts[orthoVBOffset].u = (v-size)/256.0f;
	orthoVerts[orthoVBOffset].v = 1.0f;
	orthoVBOffset++;
}


void D3D_SkyPolygon_Output(POLYHEADER *inputPolyPtr,RENDERVERTEX *renderVerticesPtr)
{
	int flags;
	int texoffset;

	float ZNear;

    // Get ZNear
	ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	// Take header information
	flags = inputPolyPtr->PolyFlags;

	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	texoffset = (inputPolyPtr->PolyColour & ClrTxDefn);

	float RecipW = 1.0f / (float) ImageHeaderArray[texoffset].ImageWidth;
	float RecipH = 1.0f / (float) ImageHeaderArray[texoffset].ImageHeight;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, texoffset, RenderPolygon.TranslucencyMode);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = 1.0f;

  		mainVertex[vb].color = RGBALIGHT_MAKE(vertices->R,vertices->G,vertices->B,vertices->A);
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);

		mainVertex[vb].tu = (float)(vertices->U) * RecipW;
		mainVertex[vb].tv = (float)(vertices->V) * RecipH;

		vb++;
	}

	#if FMV_EVERYWHERE
	TextureHandle = FMVTextureHandle[0];
	#endif

	D3D_OutputTriangles();
}

void D3D_DrawMoltenMetalMesh_Unclipped(void)
{
	float ZNear = (float) (Global_VDB_Ptr->VDB_ClipZ * GlobalScale);

	VECTORCH *point = MeshVertex;
	VECTORCH *pointWS = MeshWorldVertex;

	// outputs 450 triangles with each run of the loop
	// 450 triangles has 3 * 450 vertices which = 1350
	CheckVertexBuffer(256, currentWaterTexture, TRANSLUCENCY_NORMAL);

	for (uint32_t i=0; i < 256; i++)
	{
		if (point->vz <= 1) 
			point->vz = 1;

		mainVertex[vb].sx = (float)point->vx;
		mainVertex[vb].sy = (float)-point->vy;
		mainVertex[vb].sz = (float)point->vz;

	   	mainVertex[vb].color = MeshVertexColour[i];
		mainVertex[vb].specular = 0;

		mainVertex[vb].tu = pointWS->vx*WaterUScale+(1.0f/256.0f);
		mainVertex[vb].tv =	pointWS->vy*WaterVScale+(1.0f/256.0f);
		
		vb++;
		point++;
		pointWS++;
	}

 //	textprint("numvertices %d\n",NumVertices);

	/* CONSTRUCT POLYS */
	int count = 0;
	{
		int x = 0;
		int y = 0;

		for (x=0; x<15; x++)
		{
			for (y=0; y<15; y++)
			{
				OUTPUT_TRIANGLE(0+x+(16*y),1+x+(16*y),16+x+(16*y), 256);
				OUTPUT_TRIANGLE(1+x+(16*y),17+x+(16*y),16+x+(16*y), 256);
				count+=6;
			}
		}
	}
}

void ThisFramesRenderingHasBegun(void)
{
	BeginD3DScene(); // calls a function to perform d3d_device->Begin();
	LockExecuteBuffer(); // lock vertex buffer
}

size_t lastMem = 0;

void ThisFramesRenderingHasFinished(void)
{
	Osk_Draw();

	Con_Draw();

	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	EndD3DScene();

#if 0 // output how much memory is free
	#define MB	(1024*1024)
	MEMORYSTATUS stat;
	char buf[100];

	// Get the memory status.
	GlobalMemoryStatus( &stat );

	if ((stat.dwAvailPhys / MB) < 13)
	{
//		OutputDebugString("break here plz\n");
	}

	if (stat.dwAvailPhys != lastMem)
	{

	sprintf(buf, "%4d  free bytes of physical memory.\n", stat.dwAvailPhys);
	OutputDebugString( buf );

	sprintf(buf, "%4d  free MB of physical memory.\n", stat.dwAvailPhys / MB );
	OutputDebugString( buf );

	lastMem = stat.dwAvailPhys;
	}
#endif

/*
	sprintf(buf,  "%4d total MB of virtual memory.\n", stat.dwTotalVirtual / MB );
	OutputDebugString( buf );

    sprintf(buf, "%4d  free MB of virtual memory.\n", stat.dwAvailVirtual / MB );
	OutputDebugString( buf );
*/

/*
    AddStr( "%4d total MB of virtual memory.\n", stat.dwTotalVirtual / MB );
    AddStr( "%4d  free MB of virtual memory.\n", stat.dwAvailVirtual / MB );
    AddStr( "%4d total MB of physical memory.\n", stat.dwTotalPhys / MB );
    AddStr( "%4d  free MB of physical memory.\n", stat.dwAvailPhys / MB );
    AddStr( "%4d total MB of paging file.\n", stat.dwTotalPageFile / MB );
    AddStr( "%4d  free MB of paging file.\n", stat.dwAvailPageFile / MB );
    AddStr( "%4d  percent of memory is in use.\n", stat.dwMemoryLoad );
*/
    // Output the string.
//    OutputDebugString( buf );

 	/* KJL 11:46:56 01/16/97 - kill off any lights which are fated to be removed */
	LightBlockDeallocation();
}

extern void D3D_DrawSliderBar(int x, int y, int alpha)
{
	struct VertexTag quadVertices[4];
	int sliderHeight = 11;
	unsigned int colour = alpha>>8;

	if (colour>255) colour = 255;
	colour = (colour<<24)+0xffffff;

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + sliderHeight;
	quadVertices[3].Y = y + sliderHeight;

	{
		int topLeftU = 1;
		int topLeftV = 68;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU;// + 2;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU;// + 2;
		quadVertices[2].V = topLeftV + sliderHeight;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + sliderHeight;

		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + 2;
		quadVertices[2].X = x + 2;

		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour,
			FILTERING_BILINEAR_ON
		);

	}
	{
		int topLeftU = 7;
		int topLeftV = 68;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 2;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 2;
		quadVertices[2].V = topLeftV + sliderHeight;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + sliderHeight;

		quadVertices[0].X = x+213+2;
		quadVertices[3].X = x+213+2;
		quadVertices[1].X = x+2 +213+2;
		quadVertices[2].X = x+2 +213+2;

		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour,
			FILTERING_BILINEAR_ON
		);
	}
	quadVertices[2].Y = y + 2;
	quadVertices[3].Y = y + 2;

	{
		int topLeftU = 5;
		int topLeftV = 77;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU;
		quadVertices[2].V = topLeftV + 2;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 2;

		quadVertices[0].X = x + 2;
		quadVertices[3].X = x + 2;
		quadVertices[1].X = x + 215;
		quadVertices[2].X = x + 215;

		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour,
			FILTERING_BILINEAR_ON
		);
	}
	quadVertices[0].Y = y + 9;
	quadVertices[1].Y = y + 9;
	quadVertices[2].Y = y + 11;
	quadVertices[3].Y = y + 11;

	{
		int topLeftU = 5;
		int topLeftV = 77;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU;
		quadVertices[2].V = topLeftV + 2;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + 2;

		quadVertices[0].X = x + 2;
		quadVertices[3].X = x + 2;
		quadVertices[1].X = x + 215;
		quadVertices[2].X = x + 215;

		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour,
			FILTERING_BILINEAR_ON
		);
	}
}

extern void D3D_DrawSlider(int x, int y, int alpha)
{
	// the little thingy that slides through the rectangle
	struct VertexTag quadVertices[4];
	int sliderHeight = 5;
	unsigned int colour = alpha>>8;

	if (colour>255) colour = 255;
	colour = (colour<<24)+0xffffff;

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + sliderHeight;
	quadVertices[3].Y = y + sliderHeight;

	{
		int topLeftU = 11;
		int topLeftV = 74;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 9;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 9;
		quadVertices[2].V = topLeftV + sliderHeight;
		quadVertices[3].U = topLeftU;
		quadVertices[3].V = topLeftV + sliderHeight;

		quadVertices[0].X = x;
		quadVertices[3].X = x;
		quadVertices[1].X = x + 9;
		quadVertices[2].X = x + 9;

		D3D_HUDQuad_Output
		(
			HUDFontsImageNumber,
			quadVertices,
			colour,
			FILTERING_BILINEAR_ON
		);
	}
}

extern void D3D_DrawColourBar(int yTop, int yBottom, int rScale, int gScale, int bScale)
{
#if 0 // TODO

	extern unsigned char GammaValues[256];

	SetNewTexture(NO_TEXTURE);
	ChangeTranslucencyMode(TRANSLUCENCY_OFF);

	for (int i=0; i<255; )
	{
		unsigned int colour = 0;
		unsigned int c = 0;

		c = GammaValues[i];

		// set alpha to 255 otherwise d3d alpha test stops pixels being rendered
		colour = RGBA_MAKE(MUL_FIXED(c,rScale),MUL_FIXED(c,gScale),MUL_FIXED(c,bScale),255);

	  	tempVertex[0].sx = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
	  	tempVertex[0].sy = (float)yTop;
		tempVertex[0].sz = 0.0f;
//		tempVertex[0].rhw = 1.0f;
		tempVertex[0].color = colour;
		tempVertex[0].specular = RGBALIGHT_MAKE(0,0,0,255);
		tempVertex[0].tu = 0.0f;
		tempVertex[0].tv = 0.0f;

	  	tempVertex[1].sx = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
	  	tempVertex[1].sy = (float)yBottom;
		tempVertex[1].sz = 0.0f;
//		tempVertex[1].rhw = 1.0f;
		tempVertex[1].color = colour;
		tempVertex[1].specular = RGBALIGHT_MAKE(0,0,0,255);
		tempVertex[1].tu = 0.0f;
		tempVertex[1].tv = 0.0f;

		i++;
		c = GammaValues[i];
		colour = RGBA_MAKE(MUL_FIXED(c,rScale),MUL_FIXED(c,gScale),MUL_FIXED(c,bScale),255);

		tempVertex[2].sx = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
	  	tempVertex[2].sy = (float)yBottom;
		tempVertex[2].sz = 0.0f;
//		tempVertex[2].rhw = 1.0f;
		tempVertex[2].color = colour;
		tempVertex[2].specular = RGBALIGHT_MAKE(0,0,0,255);
		tempVertex[2].tu = 0.0f;
		tempVertex[2].tv = 0.0f;

	  	tempVertex[3].sx = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
	  	tempVertex[3].sy = (float)yTop;
		tempVertex[3].sz = 0.0f;
//		tempVertex[3].rhw = 1.0f;
		tempVertex[3].color = colour;
		tempVertex[3].specular = RGBALIGHT_MAKE(0,0,0,255);
		tempVertex[3].tu = 0.0f;
		tempVertex[3].tv = 0.0f;

		OUTPUT_TRIANGLE(0,1,3, 4);
		OUTPUT_TRIANGLE(1,2,3, 4);

		PushVerts();
	}
#endif
}


extern void D3D_FadeDownScreen(int brightness, int colour)
{
	int t = 255 - (brightness>>8);
	if (t < 0) 
		t = 0;

	CheckOrthoBuffer(4, NO_TEXTURE, TRANSLUCENCY_NORMAL, TEXTURE_WRAP);

	// bottom left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = (t<<24)+colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = (t<<24)+colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = 1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = (t<<24)+colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = 1.0f;;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = (t<<24)+colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;
}

extern void ClearZBufferWithPolygon(void)
{
	OutputDebugString("ClearZBufferWithPolygon\n");
}

extern void D3D_PlayerOnFireOverlay(void)
{
	int t = 128;
	int colour = (FMVParticleColour&0xffffff)+(t<<24);

	float u = (FastRandom()&255)/256.0f;
	float v = (FastRandom()&255)/256.0f;

	CheckOrthoBuffer(4, BurningImageNumber, TRANSLUCENCY_GLOWING, TEXTURE_WRAP);

	// bottom left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = u;
	orthoVerts[orthoVBOffset].v = v+1.0f;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = u;
	orthoVerts[orthoVBOffset].v = v;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = 1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = u+1.0f;
	orthoVerts[orthoVBOffset].v = v+1.0f;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = 1.0f;;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = u+1.0f;
	orthoVerts[orthoVBOffset].v = v;
	orthoVBOffset++;
}

extern void D3D_ScreenInversionOverlay()
{
	// alien overlay
	int theta[2];
	int colour = 0xffffffff;
	int i;

	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;

	CheckOrthoBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_DARKENINGCOLOUR, TEXTURE_WRAP);

	for (i = 0; i < 2; i++)
	{
		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;
		{
			// bottom left
			orthoVerts[orthoVBOffset].x = -1.0f;
			orthoVerts[orthoVBOffset].y = 1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = 0.375f + (cos*(-1) - sin*(+1));
			orthoVerts[orthoVBOffset].v = 0.375f + (sin*(-1) + cos*(+1));
			orthoVBOffset++;

			// top left
			orthoVerts[orthoVBOffset].x = -1.0f;
			orthoVerts[orthoVBOffset].y = -1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = 0.375f + (cos*(-1) - sin*(-1));
			orthoVerts[orthoVBOffset].v = 0.375f + (sin*(-1) + cos*(-1));
			orthoVBOffset++;

			// bottom right
			orthoVerts[orthoVBOffset].x = 1.0f;
			orthoVerts[orthoVBOffset].y = 1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = 0.375f + (cos*(+1) - sin*(+1));
			orthoVerts[orthoVBOffset].v = 0.375f + (sin*(+1) + cos*(+1));
			orthoVBOffset++;

			// top right
			orthoVerts[orthoVBOffset].x = 1.0f;;
			orthoVerts[orthoVBOffset].y = -1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = 0.375f + (cos*(+1) - sin*(-1));
			orthoVerts[orthoVBOffset].v = 0.375f + (sin*(+1) + cos*(-1));
			orthoVBOffset++;
		}

		/* only do this when finishing first loop, otherwise we reserve space for 4 verts we never add */
		if (i == 0)
		{
//			CheckVertexBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_COLOUR);
			CheckOrthoBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_COLOUR, TEXTURE_WRAP);
		}
	}
}

extern void D3D_PredatorScreenInversionOverlay()
{
	int colour = 0xffffffff;

	CheckOrthoBuffer(4, NO_TEXTURE, TRANSLUCENCY_DARKENINGCOLOUR, TEXTURE_WRAP);

	// bottom left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = 1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = 1.0f;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;
}

extern void D3D_PlayerDamagedOverlay(int intensity)
{
	int theta[2];
	int colour, baseColour;
	int i;

	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;

	switch(AvP.PlayerType)
	{
		default:
			LOCALASSERT(0);
			/* if no debug then fall through to marine */
		case I_Marine:
			baseColour = 0xff0000;
			break;

		case I_Alien:
			baseColour = 0xffff00;
			break;

		case I_Predator:
			baseColour = 0x00ff00;
			break;
	}

	colour = 0xffffff - baseColour + (intensity<<24);

	CheckOrthoBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_INVCOLOUR, TEXTURE_WRAP);

	for (i = 0; i < 2; i++)
	{
		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;
		{
			// bottom left
			orthoVerts[orthoVBOffset].x = -1.0f;
			orthoVerts[orthoVBOffset].y = 1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = (float)(0.875 + (cos*(-1) - sin*(+1)));
			orthoVerts[orthoVBOffset].v = (float)(0.375 + (sin*(-1) + cos*(+1)));
			orthoVBOffset++;

			// top left
			orthoVerts[orthoVBOffset].x = -1.0f;
			orthoVerts[orthoVBOffset].y = -1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = (float)(0.875 + (cos*(-1) - sin*(-1)));
			orthoVerts[orthoVBOffset].v = (float)(0.375 + (sin*(-1) + cos*(-1)));
			orthoVBOffset++;

			// bottom right
			orthoVerts[orthoVBOffset].x = 1.0f;
			orthoVerts[orthoVBOffset].y = 1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = (float)(0.875 + (cos*(+1) - sin*(+1)));
			orthoVerts[orthoVBOffset].v = (float)(0.375 + (sin*(+1) + cos*(+1)));
			orthoVBOffset++;

			// top right
			orthoVerts[orthoVBOffset].x = 1.0f;
			orthoVerts[orthoVBOffset].y = -1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = (float)(0.875 + (cos*(+1) - sin*(-1)));
			orthoVerts[orthoVBOffset].v = (float)(0.375 + (sin*(+1) + cos*(-1)));
			orthoVBOffset++;
		}

		colour = baseColour +(intensity<<24);

		/* only do this when finishing first loop, otherwise we reserve space for 4 verts we never add */
		if (i == 0)
		{
//			CheckVertexBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_GLOWING);
			CheckOrthoBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_GLOWING, TEXTURE_WRAP);
		}
	}
}

/* D3D_DrawCable - draws predator grappling hook */
void D3D_DrawCable(VECTORCH *centrePtr, MATRIXCH *orientationPtr)
{	
	/* TODO - not disabling zwrites. probably need to do this but double check (be handy if we didn't have to) */

	currentWaterTexture = NO_TEXTURE;

	MeshXScale = 4096/16;
	MeshZScale = 4096/16;

	for (int field=0; field<3; field++)
	{
		int i=0;
		int x;
		for (x=(0+field*15); x<(16+field*15); x++)
		{
			int z;
			for(z=0; z<16; z++)
			{
				VECTORCH *point = &MeshVertex[i];
				{
					int innerRadius = 20;
					VECTORCH radius;
					int theta = ((4096*z)/15)&4095;
					int rOffset = GetSin((x*64+theta/32-CloakingPhase)&4095);
					rOffset = MUL_FIXED(rOffset,rOffset)/512;

					radius.vx = MUL_FIXED(innerRadius+rOffset/8,GetSin(theta));
					radius.vy = MUL_FIXED(innerRadius+rOffset/8,GetCos(theta));
					radius.vz = 0;

					RotateVector(&radius,orientationPtr);

					point->vx = centrePtr[x].vx+radius.vx;
					point->vy = centrePtr[x].vy+radius.vy;
					point->vz = centrePtr[x].vz+radius.vz;

					MeshVertexColour[i] = RGBALIGHT_MAKE(0,rOffset,255,128);
				}

				TranslatePointIntoViewspace(point);

				/* is particle within normal view frustrum ? */
				if(AvP.PlayerType==I_Alien)	/* wide frustrum */
				{
					if(( (-point->vx <= point->vz*2)
		   				&&(point->vx <= point->vz*2)
						&&(-point->vy <= point->vz*2)
						&&(point->vy <= point->vz*2) ))
					{
						MeshVertexOutcode[i]=1;
					}
					else
					{
						MeshVertexOutcode[i]=0;
					}
				}
				else
				{
					if(( (-point->vx <= point->vz)
		   				&&(point->vx <= point->vz)
						&&(-point->vy <= point->vz)
						&&(point->vy <= point->vz) ))
					{
						MeshVertexOutcode[i]=1;
					}
					else
					{
						MeshVertexOutcode[i]=0;
					}
				}
				i++;
			}
		}
		
		D3D_DrawMoltenMetalMesh_Unclipped();
	}
}

extern "C++" {

void SetupFMVTexture(FMVTEXTURE *ftPtr)
{
	ftPtr->RGBBuffer = new uint8_t[128 * 128 * 4];

	ftPtr->SoundVolume = 0;
}

void UpdateFMVTexture(FMVTEXTURE *ftPtr)
{
	assert(ftPtr);
	assert(ftPtr->ImagePtr);
	assert(ftPtr->ImagePtr->Direct3DTexture);

	if (!ftPtr)
		return;

	if (!NextFMVTextureFrame(ftPtr))
	{
	 	return;
	}

	// lock the d3d texture
	D3DLOCKED_RECT textureRect;
	LastError = ftPtr->ImagePtr->Direct3DTexture->LockRect(0, &textureRect, NULL, D3DLOCK_DISCARD);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return;
	}

	uint8_t *destPtr = NULL;
	uint8_t *srcPtr = &ftPtr->RGBBuffer[0];

	for (uint32_t y = 0; y < ftPtr->ImagePtr->ImageHeight; y++)
	{
		destPtr = static_cast<uint8_t*>(textureRect.pBits) + y * textureRect.Pitch;

		for (uint32_t x = 0; x < ftPtr->ImagePtr->ImageWidth; x++)
		{
/*
			destPtr[0] = srcPtr[0];
			destPtr[1] = srcPtr[1];
			destPtr[2] = srcPtr[2];
			destPtr[3] = srcPtr[3];
*/
			memcpy(destPtr, srcPtr, sizeof(uint32_t));

			destPtr += sizeof(uint32_t);
			srcPtr += sizeof(uint32_t);
		}
	}

	// unlock d3d texture
	LastError = ftPtr->ImagePtr->Direct3DTexture->UnlockRect(0);
	if (FAILED(LastError))
	{
		LogErrorString("Could not unlock Direct3D texture ftPtr->DestTexture", __LINE__, __FILE__);
		return;
	}
}

} // extern "C"

}; // ?

void r2rect :: AlphaFill
(
	unsigned char R,
	unsigned char G,
	unsigned char B,
	unsigned char translucency
) const
{
	GLOBALASSERT
	(
		bValidPhys()
	);

	if (y1<=y0) return;
	D3D_Rectangle(x0, y0, x1, y1, R, G, B, translucency);
}

extern void D3D_RenderHUDNumber_Centred(unsigned int number,int x,int y,int colour)
{
	// green and red ammo numbers
	struct VertexTag quadVertices[4];
	int noOfDigits = 3;
	int h = MUL_FIXED(HUDScaleFactor, HUD_DIGITAL_NUMBERS_HEIGHT);
	int w = MUL_FIXED(HUDScaleFactor, HUD_DIGITAL_NUMBERS_WIDTH);

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + h;
	quadVertices[3].Y = y + h;

	x += (3*w)/2;

	do
	{
		int digit = number%10;
		number/=10;
		{
			int topLeftU;
			int topLeftV;
			if (digit<8)
			{
				topLeftU = 1+(digit)*16;
				topLeftV = 1;
			}
			else
			{
				topLeftU = 1+(digit-8)*16;
				topLeftV = 1+24;
			}
			if (AvP.PlayerType == I_Marine) topLeftV+=80;

			quadVertices[0].U = topLeftU;
			quadVertices[0].V = topLeftV;
			quadVertices[1].U = topLeftU + HUD_DIGITAL_NUMBERS_WIDTH;
			quadVertices[1].V = topLeftV;
			quadVertices[2].U = topLeftU + HUD_DIGITAL_NUMBERS_WIDTH;
			quadVertices[2].V = topLeftV + HUD_DIGITAL_NUMBERS_HEIGHT;
			quadVertices[3].U = topLeftU;
			quadVertices[3].V = topLeftV + HUD_DIGITAL_NUMBERS_HEIGHT;

			x -= 1+w;
			quadVertices[0].X = x;
			quadVertices[3].X = x;
			quadVertices[1].X = x + w;
			quadVertices[2].X = x + w;

			D3D_HUDQuad_Output(HUDFontsImageNumber, quadVertices, colour, FILTERING_BILINEAR_OFF);
		}
	}
	while (--noOfDigits);
}

extern void D3D_RenderHUDString(char *stringPtr, int x, int y, int colour)
{
	// mission briefing text?
	if (stringPtr == NULL)
		return;

	struct VertexTag quadVertices[4];

	quadVertices[0].Y = y-1;
	quadVertices[1].Y = y-1;
	quadVertices[2].Y = y + HUD_FONT_HEIGHT + 1;
	quadVertices[3].Y = y + HUD_FONT_HEIGHT + 1;

	while( *stringPtr )
	{
		char c = *stringPtr++;
		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[1].V = topLeftV - 1;
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;
			
			quadVertices[0].X = x - 1;
			quadVertices[3].X = x - 1;
			quadVertices[1].X = x + HUD_FONT_WIDTH + 1;
			quadVertices[2].X = x + HUD_FONT_WIDTH + 1;

			D3D_HUDQuad_Output(AAFontImageNumber, quadVertices, colour, FILTERING_BILINEAR_OFF);
		}
		x += AAFontWidths[(unsigned char)c];
	}
}

extern void D3D_RenderHUDString_Clipped(char *stringPtr, int x, int y, int colour)
{
	if (stringPtr == NULL)
		return;

	struct VertexTag quadVertices[4];

 	LOCALASSERT(y<=0);

	quadVertices[2].Y = y + HUD_FONT_HEIGHT + 1;
	quadVertices[3].Y = y + HUD_FONT_HEIGHT + 1;

	quadVertices[0].Y = 0;
	quadVertices[1].Y = 0;

	while ( *stringPtr )
	{
		char c = *stringPtr++;

		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			// top left
			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - y;

			// top right
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[1].V = topLeftV - y;

			// bottom right
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;

			// bottom left
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;

			quadVertices[0].X = x - 1;
			quadVertices[3].X = x - 1;
			quadVertices[1].X = x + HUD_FONT_WIDTH + 1;
			quadVertices[2].X = x + HUD_FONT_WIDTH + 1;

			D3D_HUDQuad_Output(AAFontImageNumber, quadVertices, colour, FILTERING_BILINEAR_OFF);
		}
		x += AAFontWidths[(unsigned char)c];
	}
}

void D3D_RenderHUDString_Centred(char *stringPtr, int centreX, int y, int colour)
{
	// white text only of marine HUD ammo, health numbers etc
	if (stringPtr == NULL)
		return;

	int length = 0;
	char *ptr = stringPtr;

	while(*ptr)
	{
		length+=AAFontWidths[(unsigned char)*ptr++];
	}

	length = MUL_FIXED(HUDScaleFactor,length);
// 	D3D_RenderHUDString(stringPtr,centreX-length/2,y,colour);

	int x = centreX-length/2;

	struct VertexTag quadVertices[4];

	quadVertices[0].Y = y - MUL_FIXED(HUDScaleFactor,1);
	quadVertices[1].Y = y - MUL_FIXED(HUDScaleFactor,1);
	quadVertices[2].Y = y + MUL_FIXED(HUDScaleFactor,HUD_FONT_HEIGHT + 1);
	quadVertices[3].Y = y + MUL_FIXED(HUDScaleFactor,HUD_FONT_HEIGHT + 1);

	while (*stringPtr)
	{
		char c = *stringPtr++;
		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			// top left
			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;

			// top right
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[1].V = topLeftV - 1;

			// bottom right
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH + 1;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;

			// bottom left
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;

			quadVertices[0].X = x - MUL_FIXED(HUDScaleFactor,1);
			quadVertices[3].X = x - MUL_FIXED(HUDScaleFactor,1);
			quadVertices[1].X = x + MUL_FIXED(HUDScaleFactor,HUD_FONT_WIDTH + 1);
			quadVertices[2].X = x + MUL_FIXED(HUDScaleFactor,HUD_FONT_WIDTH + 1);

			D3D_HUDQuad_Output(AAFontImageNumber, quadVertices, colour, FILTERING_BILINEAR_OFF);

		}
		x += MUL_FIXED(HUDScaleFactor,AAFontWidths[(unsigned char)c]);
	}
}

extern "C"
{

extern void RenderString(char *stringPtr, int x, int y, int colour)
{
	D3D_RenderHUDString(stringPtr, x, y, colour);
}

extern void RenderStringCentred(char *stringPtr, int centreX, int y, int colour)
{
	int length = 0;
	char *ptr = stringPtr;

	while(*ptr)
	{
		length+=AAFontWidths[(unsigned char)*ptr++];
	}
	D3D_RenderHUDString(stringPtr,centreX-length/2,y,colour);
}

extern void RenderStringVertically(char *stringPtr, int centreX, int bottomY, int colour)
{
	struct VertexTag quadVertices[4];
	int y = bottomY;

	quadVertices[0].X = centreX - (HUD_FONT_HEIGHT/2) - 1;
	quadVertices[1].X = quadVertices[0].X;
	quadVertices[2].X = quadVertices[0].X+2+HUD_FONT_HEIGHT*1;
	quadVertices[3].X = quadVertices[2].X;

	int width = (centreX - (HUD_FONT_HEIGHT/2) - 1) + 2+HUD_FONT_HEIGHT*1;

	while (*stringPtr)
	{
		char c = *stringPtr++;
		{
			int topLeftU = 1+((c-32)&15)*16;
			int topLeftV = 1+((c-32)>>4)*16;

			// top left
			quadVertices[0].U = topLeftU - 1;
			quadVertices[0].V = topLeftV - 1;

			// top right
			quadVertices[1].U = topLeftU + HUD_FONT_WIDTH;
			quadVertices[1].V = topLeftV - 1;

			// bottom right
			quadVertices[2].U = topLeftU + HUD_FONT_WIDTH;
			quadVertices[2].V = topLeftV + HUD_FONT_HEIGHT + 1;

			// bottom left
			quadVertices[3].U = topLeftU - 1;
			quadVertices[3].V = topLeftV + HUD_FONT_HEIGHT + 1;

			quadVertices[0].Y = y;
			quadVertices[1].Y = y - HUD_FONT_WIDTH*1 -1;
			quadVertices[2].Y = y - HUD_FONT_WIDTH*1 -1;
			quadVertices[3].Y = y;

			int height = y - (y - HUD_FONT_WIDTH*1 -1);

			D3D_HUDQuad_Output(AAFontImageNumber, quadVertices, colour, FILTERING_BILINEAR_OFF);

		}
	   	y -= AAFontWidths[(unsigned char)c];
	}
}

void DrawMenuTextGlow(int topLeftX, int topLeftY, int size, int alpha)
{
	int textureWidth = 0;
	int textureHeight = 0;
	int texturePOW2Width = 0;
	int texturePOW2Height = 0;

	if (alpha > ONE_FIXED) // ONE_FIXED = 65536
		alpha = ONE_FIXED;

	alpha = (alpha / 256);
	if (alpha > 255) 
		alpha = 255;

	// textures original resolution (if it's a non power of 2, these will be the non power of 2 values)
	textureWidth = AvPMenuGfxStorage[AVPMENUGFX_GLOWY_LEFT].Width;
	textureHeight = AvPMenuGfxStorage[AVPMENUGFX_GLOWY_LEFT].Height;

	// these values are the texture width and height as power of two values
//	texturePOW2Width = AvPMenuGfxStorage[AVPMENUGFX_GLOWY_LEFT].newWidth;
//	texturePOW2Height = AvPMenuGfxStorage[AVPMENUGFX_GLOWY_LEFT].newHeight;

	// do the text alignment justification
	topLeftX -= textureWidth;

	DrawQuad(topLeftX, topLeftY, textureWidth, textureHeight, AVPMENUGFX_GLOWY_LEFT, D3DCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);

	// now do the middle section
	topLeftX += textureWidth;

	textureWidth = AvPMenuGfxStorage[AVPMENUGFX_GLOWY_MIDDLE].Width;
	textureHeight = AvPMenuGfxStorage[AVPMENUGFX_GLOWY_MIDDLE].Height;

	DrawQuad(topLeftX, topLeftY, textureWidth * size, textureHeight, AVPMENUGFX_GLOWY_MIDDLE, D3DCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);

	// now do the right section
	topLeftX += textureWidth * size;

	textureWidth = AvPMenuGfxStorage[AVPMENUGFX_GLOWY_RIGHT].Width;
	textureHeight = AvPMenuGfxStorage[AVPMENUGFX_GLOWY_RIGHT].Height;

	DrawQuad(topLeftX, topLeftY, textureWidth, textureHeight, AVPMENUGFX_GLOWY_RIGHT, D3DCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);
}

void DrawFadeQuad(int topX, int topY, int alpha)
{
	alpha = alpha / 256;
	if (alpha > 255) alpha = 255;
	D3DCOLOR colour = D3DCOLOR_ARGB(alpha,0,0,0);

	CheckOrthoBuffer(4, NO_TEXTURE, TRANSLUCENCY_GLOWING, TEXTURE_WRAP);

	// bottom left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = -1.0f;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = 1.0f;
	orthoVerts[orthoVBOffset].y = 1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = 1.0f;;
	orthoVerts[orthoVBOffset].y = -1.0f;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;
}

/* more quad drawing functions than you can shake a stick at! */
void DrawFmvFrame(uint32_t frameWidth, uint32_t frameHeight, uint32_t textureWidth, uint32_t textureHeight, LPDIRECT3DTEXTURE8 fmvTexture)
{
#if 0
	int topX = (640 - frameWidth) / 2;
	int topY = (480 - frameHeight) / 2;

	float x1 = (float(topX / 640.0f) * 2) - 1;
	float y1 = (float(topY / 480.0f) * 2) - 1;

	float x2 = ((float(topX + frameWidth) / 640.0f) * 2) - 1;
	float y2 = ((float(topY + frameHeight) / 480.0f) * 2) - 1;

	D3DCOLOR colour = D3DCOLOR_ARGB(255, 255, 255, 255);

	ORTHOVERTEX fmvVerts[4];

	// bottom left
	fmvVerts[0].x = x1;
	fmvVerts[0].y = y2;
	fmvVerts[0].z = 1.0f;
	fmvVerts[0].colour = colour;
	fmvVerts[0].u = 0.0f;
	fmvVerts[0].v = (1.0f / textureHeight) * frameHeight;

	// top left
	fmvVerts[1].x = x1;
	fmvVerts[1].y = y1;
	fmvVerts[1].z = 1.0f;
	fmvVerts[1].colour = colour;
	fmvVerts[1].u = 0.0f;
	fmvVerts[1].v = 0.0f;

	// bottom right
	fmvVerts[2].x = x2;
	fmvVerts[2].y = y2;
	fmvVerts[2].z = 1.0f;
	fmvVerts[2].colour = colour;
	fmvVerts[2].u = (1.0f / textureWidth) * frameWidth;
	fmvVerts[2].v = (1.0f / textureHeight) * frameHeight;

	// top right
	fmvVerts[3].x = x2;
	fmvVerts[3].y = y1;
	fmvVerts[3].z = 1.0f;
	fmvVerts[3].colour = colour;
	fmvVerts[3].u = (1.0f / textureWidth) * frameWidth;
	fmvVerts[3].v = 0.0f;

	ChangeTextureAddressMode(TEXTURE_CLAMP);
	ChangeTranslucencyMode(TRANSLUCENCY_OFF);
		
	// set the texture
	LastError = d3d.lpD3DDevice->SetTexture(0, fmvTexture);

	d3d.lpD3DDevice->SetFVF(D3DFVF_ORTHOVERTEX);
	HRESULT LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &fmvVerts[0], sizeof(ORTHOVERTEX));
	if (FAILED(LastError))
	{
		OutputDebugString("DrawPrimitiveUP failed\n");
	}
#endif
}

/* more quad drawing functions than you can shake a stick at! */
void DrawFmvFrame2(uint32_t frameWidth, uint32_t frameHeight, uint32_t textureWidth, uint32_t textureHeight, LPDIRECT3DTEXTURE8 tex[3])
{
/*
	int topX = (640 - frameWidth) / 2;
	int topY = (480 - frameHeight) / 2;

	float x1 = (float(topX / 640.0f) * 2) - 1;
	float y1 = (float(topY / 480.0f) * 2) - 1;

	float x2 = ((float(topX + frameWidth) / 640.0f) * 2) - 1;
	float y2 = ((float(topY + frameHeight) / 480.0f) * 2) - 1;

	D3DCOLOR colour = D3DCOLOR_ARGB(255, 255, 255, 255);

	FMVVERTEX fmvVerts[4];

	// bottom left
	fmvVerts[0].x = x1;
	fmvVerts[0].y = y2;
	fmvVerts[0].z = 1.0f;
//	fmvVerts[0].colour = colour;
	fmvVerts[0].u1 = 0.0f;
	fmvVerts[0].v1 = (1.0f / textureHeight) * frameHeight;

	fmvVerts[0].u2 = 0.0f;
	fmvVerts[0].v2 = 1.0f;

	fmvVerts[0].u3 = 0.0f;
	fmvVerts[0].v3 = 1.0f;

	// top left
	fmvVerts[1].x = x1;
	fmvVerts[1].y = y1;
	fmvVerts[1].z = 1.0f;
//	fmvVerts[1].colour = colour;
	fmvVerts[1].u1 = 0.0f;
	fmvVerts[1].v1 = 0.0f;

	fmvVerts[1].u2 = 0.0f;
	fmvVerts[1].v2 = 0.0f;

	fmvVerts[1].u3 = 0.0f;
	fmvVerts[1].v3 = 0.0f;

	// bottom right
	fmvVerts[2].x = x2;
	fmvVerts[2].y = y2;
	fmvVerts[2].z = 1.0f;
//	fmvVerts[2].colour = colour;
	fmvVerts[2].u1 = (1.0f / textureWidth) * frameWidth;
	fmvVerts[2].v1 = (1.0f / textureHeight) * frameHeight;

	fmvVerts[2].u2 = 1.0f;
	fmvVerts[2].v2 = 1.0f;

	fmvVerts[2].u3 = 1.0f;
	fmvVerts[2].v3 = 1.0f;

	// top right
	fmvVerts[3].x = x2;
	fmvVerts[3].y = y1;
	fmvVerts[3].z = 1.0f;
//	fmvVerts[3].colour = colour;
	fmvVerts[3].u1 = (1.0f / textureWidth) * frameWidth;
	fmvVerts[3].v1 = 0.0f;

	fmvVerts[3].u2 = 1.0f;
	fmvVerts[3].v2 = 0.0f;

	fmvVerts[3].u3 = 1.0f;
	fmvVerts[3].v3 = 0.0f;

	ChangeTextureAddressMode(TEXTURE_CLAMP);
	ChangeTranslucencyMode(TRANSLUCENCY_OFF);

	// set the texture
	LastError = d3d.lpD3DDevice->SetTexture(0, tex[0]);
	LastError = d3d.lpD3DDevice->SetTexture(1, tex[1]);
	LastError = d3d.lpD3DDevice->SetTexture(2, tex[2]);

	d3d.lpD3DDevice->SetVertexDeclaration(d3d.fmvVertexDecl);
	d3d.lpD3DDevice->SetVertexShader(d3d.fmvVertexShader);
	d3d.lpD3DDevice->SetPixelShader(d3d.fmvPixelShader);

//	d3d.lpD3DDevice->SetFVF(D3DFVF_ORTHOVERTEX);
	HRESULT LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &fmvVerts[0], sizeof(FMVVERTEX));
	if (FAILED(LastError))
	{
		OutputDebugString("DrawPrimitiveUP failed\n");
	}
*/
}

void DrawProgressBar(RECT srcRect, RECT destRect, uint32_t textureID, AVPTEXTURE *tex, uint32_t newWidth, uint32_t newHeight)
{
	CheckOrthoBuffer(4, textureID, TRANSLUCENCY_OFF, TEXTURE_CLAMP, FILTERING_BILINEAR_ON);

	float x1 = (float(destRect.left / (float)ScreenDescriptorBlock.SDB_Width) * 2) - 1;
	float y1 = (float(destRect.top / (float)ScreenDescriptorBlock.SDB_Height) * 2) - 1;

	float x2 = ((float(destRect.right) / (float)ScreenDescriptorBlock.SDB_Width) * 2) - 1;
	float y2 = ((float(destRect.bottom ) / (float)ScreenDescriptorBlock.SDB_Height) * 2) - 1;

	if (!tex)
	{
		return;
	}

	int width = tex->width;
	int height = tex->height;

	// new, power of 2 values
	int realWidth = newWidth;
	int realHeight = newHeight;

	float RecipW = (1.0f / realWidth);
	float RecipH = (1.0f / realHeight);

	D3DCOLOR colour = D3DCOLOR_XRGB(255, 255, 255);

	// bottom left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((width - srcRect.left) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((height - srcRect.bottom) * RecipH);
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((width - srcRect.left) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((height - srcRect.top) * RecipH);
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((width - srcRect.right) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((height - srcRect.bottom) * RecipH);
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((width - srcRect.right) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((height - srcRect.top) * RecipH);
	orthoVBOffset++;
}

void DrawQuad(uint32_t x, uint32_t y, uint32_t width, uint32_t height, int32_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType)
{
	float x1 = (float(x / 640.0f) * 2) - 1;
	float y1 = (float(y / 480.0f) * 2) - 1;

	float x2 = ((float(x + width) / 640.0f) * 2) - 1;
	float y2 = ((float(y + height) / 480.0f) * 2) - 1;

	uint32_t texturePOW2Width, texturePOW2Height;
	
	// if in menus (outside game)
	if (textureID >= texIDoffset)
	{
		Tex_GetDimensions(textureID, texturePOW2Width, texturePOW2Height);
	}
	else 
	{
		if (mainMenu)
		{
			texturePOW2Width = AvPMenuGfxStorage[textureID].newWidth;
			texturePOW2Height = AvPMenuGfxStorage[textureID].newHeight;
		}
		else
		{
			texturePOW2Width = ImageHeaderArray[textureID].ImageWidth;
			texturePOW2Height = ImageHeaderArray[textureID].ImageHeight;
		}
	}

	CheckOrthoBuffer(4, textureID, translucencyType, TEXTURE_CLAMP);

	// bottom left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = (1.0f / texturePOW2Height) * height;
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = 0.0f;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (1.0f / texturePOW2Width) * width;
	orthoVerts[orthoVBOffset].v = (1.0f / texturePOW2Height) * height;
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (1.0f / texturePOW2Width) * width;
	orthoVerts[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;
}

void DrawAlphaMenuQuad(int topX, int topY, int image_num, int alpha) 
{
	// textures actual height/width (whether it's non power of two or not)
	int textureWidth = AvPMenuGfxStorage[image_num].Width;
	int textureHeight = AvPMenuGfxStorage[image_num].Height;

	// we pad non power of two textures to pow2
	int texturePOW2Width = AvPMenuGfxStorage[image_num].newWidth;
	int texturePOW2Height = AvPMenuGfxStorage[image_num].newHeight;

	alpha = (alpha / 256);
	if (alpha > 255) 
		alpha = 255;

	DrawQuad(topX, topY, textureWidth, textureHeight, image_num, D3DCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);
}

void DrawSmallMenuCharacter(int topX, int topY, int texU, int texV, int red, int green, int blue, int alpha) 
{
	alpha = (alpha / 256);

	red = (red / 256);
	green = (green / 256);
	blue = (blue / 256);

	// clamp if needed
	if (alpha > 255) alpha = 255;
	if (red > 255) red = 255;
	if (green > 255) green = 255;
	if (blue > 255) blue = 255;

	D3DCOLOR colour = D3DCOLOR_ARGB(alpha, 255, 255, 255);
//	D3DCOLOR colour = D3DCOLOR_ARGB(alpha, red, green, blue);

	int font_height = 15;
	int font_width = 15;

	// aa_font.bmp is 256 x 256
	int image_height = 256;
	int image_width = 256;

	float RecipW = 1.0f / image_width; // 0.00390625
	float RecipH = 1.0f / image_height;

	float x1 = (float(topX / 640.0f) * 2) - 1;
	float y1 = (float(topY / 480.0f) * 2) - 1;

	float x2 = ((float(topX + font_width) / 640.0f) * 2) - 1;
	float y2 = ((float(topY + font_height) / 480.0f) * 2) - 1;

	CheckOrthoBuffer(4, AVPMENUGFX_SMALL_FONT, TRANSLUCENCY_GLOWING, TEXTURE_CLAMP);

	// bottom left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV + font_height) * RecipH);
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV) * RecipH);
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU + font_height) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV + font_height) * RecipH);
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU + font_width) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV) * RecipH);
	orthoVBOffset++;
}

void DrawTallFontCharacter(int topX, int topY, int texU, int texV, int char_width, int alpha) 
{
	alpha = (alpha / 256);
	if (alpha > 255) 
		alpha = 255;

	D3DCOLOR colour = D3DCOLOR_ARGB(alpha, 255, 255, 255);

	int real_height = 512;
	int real_width = 512;

	float RecipW = 1.0f / real_width;
	float RecipH = 1.0f / real_height;

	int height_of_char = 33;

	float x1 = (float(topX / 640.0f) * 2) - 1;
	float y1 = (float(topY / 480.0f) * 2) - 1;

	float x2 = ((float(topX + char_width) / 640.0f) * 2) - 1;
	float y2 = ((float(topY + height_of_char) / 480.0f) * 2) - 1;

	CheckOrthoBuffer(4, TALLFONT_TEX, TRANSLUCENCY_GLOWING, TEXTURE_CLAMP);

	// bottom left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV + height_of_char) * RecipH);
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV) * RecipH);
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU + char_width) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV + height_of_char) * RecipH);
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU + char_width) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV) * RecipH);
	orthoVBOffset++;
}

};