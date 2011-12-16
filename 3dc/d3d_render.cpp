#include "console.h"
#include "OnScreenKeyboard.h"
#include "TextureManager.h"
#include "r2base.h"
#include "Fonts.h"
#include <vector>
#include <algorithm>
#include "logString.h"
#include "RenderList.h"
#include "VertexBuffer.h"
#include "FmvCutscenes.h"
#define UseLocalAssert FALSE
#include "ourasert.h"
#include <assert.h>
#include "avp_menus.h"
#include "avp_userprofile.h"
#include "avp_menugfx.hpp"
#include "io.h"
#include "pheromon.h"
#include "tables.h"
#include "particle.h"
#include "kshape.h"
#include "3dc.h"
#include "inline.h"
#include "gamedef.h"
#include "dxlog.h"
#include "d3d_hud.h"
#include "HUD_layout.h"
#define HAVE_VISION_H 1
#include "lighting.h"
#include "showcmds.h"
#include "frustum.h"
#include "d3d_render.h"
#include "bh_types.h"

bool frustumCull = false;

#define FMV_ON 0
#define FMV_SIZE 128

AVPTEXTURE FMVTextureHandle[4];
AVPTEXTURE NoiseTextureHandle;

int WaterXOrigin;
int WaterZOrigin;
float WaterUScale;
float WaterVScale;

int MeshZScale;
int MeshXScale;
int WaterFallBase;

VECTORCH MeshWorldVertex[256];
uint32_t MeshVertexColour[256];

int WireFrameMode;

uint32_t FMVParticleColour;

const int kNumMeshVerts = 256;
VECTORCH MeshVertex[kNumMeshVerts];

extern void DrawParticles();
extern void DrawCoronas();
extern bool IsDemoVersion();

int NumberOfLandscapePolygons;

// set to 'null' texture initially
texID_t currentWaterTexture = NO_TEXTURE;

const float zNear = 64.0f;
const float zFar  = 1000000.0f;

// extern variables
extern D3DINFO    d3d;
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern VIEWDESCRIPTORBLOCK* Global_VDB_Ptr;
extern float CameraZoomScale;
extern float p, o;
extern DISPLAYBLOCK *Player;
extern DISPLAYBLOCK *ActiveBlockList[];
extern DISPLAYBLOCK *OnScreenBlockList[];
extern int HUDScaleFactor;
extern int NumOnScreenBlocks;
extern char LevelName[];
extern int NumActiveBlocks;
extern unsigned char GammaValues[256];
extern int GlobalAmbience;
extern char CloakedPredatorIsMoving;

// extern functions
extern void BuildFrustum();
extern BOOL LevelHasStars;
extern void HandleRainShaft(MODULE *modulePtr, int bottomY, int topY, int numberOfRaindrops);
extern void RenderMirrorSurface(void);
extern void RenderMirrorSurface2(void);
extern void RenderParticlesInMirror(void);
extern void CheckForObjectsInWater(int minX, int maxX, int minZ, int maxZ, int averageY);
extern void HandleRain(int numberOfRaindrops);
extern void RenderSky(void);
extern void RenderStarfield(void);
extern void ScanImagesForFMVs();
extern bool CheckPointIsInFrustum(D3DXVECTOR3 *point);
bool CreateVolatileResources();
bool ReleaseVolatileResources();
void ColourFillBackBuffer(int FillColour);

int LightIntensityAtPoint(VECTORCH *pointPtr);
static float currentCameraZoomScale = 1.0f;

D3DLVERTEX *mainVertex = NULL;
uint16_t *mainIndex = NULL;

ORTHOVERTEX *orthoVertex = NULL;
uint16_t *orthoIndex = NULL;

PARTICLEVERTEX *particleVertex = NULL;
uint16_t *particleIndex = NULL;

DECAL_VERTEX *decalVertex = NULL;
uint16_t *decalIndex = NULL;

static uint32_t pVb = 0;
static uint32_t vb = 0;
static uint32_t decalVb = 0;
static uint32_t orthoVBOffset = 0;
static uint32_t NumberOfRenderedTriangles = 0;

RenderList *particleList = 0;
RenderList *mainList = 0;
RenderList *orthoList = 0;
RenderList *decalList = 0;

static HRESULT LastError; // remove me eventually (when no more D3D calls exist in this file)

// set our std::vectors capacity allowing us to use them as regular arrays
void RenderListInit()
{
	particleList = new RenderList(400);
	mainList     = new RenderList(800);
	orthoList    = new RenderList(50);
	decalList    = new RenderList(400);
}

void RenderListDeInit()
{
	delete particleList;
	particleList = 0;

	delete mainList;
	mainList = 0;

	delete orthoList;
	orthoList = 0;

	delete decalList;
	decalList = 0;
}

struct renderParticle
{
	PARTICLE		particle;
	PARTICLEVERTEX	vertices[4];
//	int				translucency;

//	inline bool operator<(const renderParticle& rhs) const {return translucency < rhs.translucency;}
};

// particle buckets
std::vector<std::vector<renderParticle> > particleBucket;
bool pbInit = false;

struct renderCorona
{
	PARTICLE		particle;
	VECTORCHF		coronaPoint;
	uint32_t		numVerts;
	int				translucency;

	inline bool operator<(const renderCorona& rhs) const {return translucency < rhs.translucency;}
};

std::vector<renderParticle> particleArray;
std::vector<renderCorona>   coronaArray;

// convert a width pixel range value (eg 0-640) to device coordinate (eg -1.0 to 1.0)
inline float WPos2DC(int32_t pos)
{
	if (mainMenu)
	{
		return (float(pos / (float)640) * 2) - 1;
	}
	else
	{
		return (float(pos / (float)ScreenDescriptorBlock.SDB_Width) * 2) - 1;
	}
}

// convert a width pixel range value (eg 0-480) to device coordinate (eg -1.0 to 1.0)
inline float HPos2DC(int32_t pos)
{
	if (mainMenu)
	{
		return (float(pos / (float)480) * 2) - 1;
	}
	else
	{
		return (float(pos / (float)ScreenDescriptorBlock.SDB_Height) * 2) - 1;
	}
}

// AvP uses numVerts to represent the actual number of polygon vertices we're passing to the backend.
// we then use the OUTPUT_TRIANGLE function to generate the indices required to represent those
// polygons. This function calculates the number of indices required based on the number of verts.
// AvP code defined these, so hence the magic numbers.

uint32_t GetNumIndices(uint32_t numVerts)
{
	uint32_t numIndices = 0;

	switch (numVerts)
	{
		case 3:
			numIndices = 3;
			break;
		case 4:
			numIndices = 6;
			break;
		case 5:
			numIndices = 9;
			break;
		case 6:
			numIndices = 12;
			break;
		case 7:
			numIndices = 15;
			break;
		case 8:
			numIndices = 18;
			break;
		case 0:
			numIndices = 0;
			break;
		case 256:
			numIndices = 1350;
			break;
		default:
			// need to check this is ok!!
			numIndices = numVerts;
	}
	return numIndices;
}

bool staticDecalsTest = false;

// lock our dynamic vertex and index buffers, and reset counters and array indexes used to keep 
// track of the number of verts and indices in those buffers. Function could be renamed as 
// we no longer use an execute buffer
static bool LockExecuteBuffer()
{
	// lock particle vertex and index buffers
	d3d.particleVB->Lock((void**)&particleVertex);
	d3d.particleIB->Lock(&particleIndex);

	// lock main vertex and index buffers
	d3d.mainVB->Lock((void**)&mainVertex);
	d3d.mainIB->Lock(&mainIndex);

	// lock ortho vertex and index buffers
	d3d.orthoVB->Lock((void**)&orthoVertex);
	d3d.orthoIB->Lock(&orthoIndex);

	d3d.decalVB->Lock((void**)&decalVertex);

	if (!staticDecalsTest)
	{
		d3d.decalIB->Lock(&decalIndex);

		uint32_t vertexCount = 0;

		// generate indices
		for (int i = 0; i < (12288 * 2); i+=6)
		{
			decalIndex[i]   = 0 + vertexCount;//(vertexCount - (4) + (0));
			decalIndex[i+1] = 1 + vertexCount;//(vertexCount - (4) + (1));
			decalIndex[i+2] = 2 + vertexCount;//(vertexCount - (4) + (2));

//			vertexCount+=3;

			decalIndex[i+3] = 0 + vertexCount;//(vertexCount - (4) + (0));
			decalIndex[i+4] = 2 + vertexCount;//(vertexCount - (4) + (2));
			decalIndex[i+5] = 3 + vertexCount;//(vertexCount - (4) + (3));

			vertexCount+=4;
		}

//		staticDecalsTest = true;
	}

	// reset lists to empty state
	particleList->Reset();
	mainList->Reset();
	orthoList->Reset();
	decalList->Reset();

	// reset counters and indexes
	vb = 0;
	decalVb = 0;
	orthoVBOffset = 0;
	pVb = 0;

    return true;
}

// unlock all dynamic vertex and index buffers. function could be renamed as no longer using execute buffers
static bool UnlockExecuteBufferAndPrepareForUse()
{
	// unlock particle vertex and index buffers
	d3d.particleVB->Unlock();
	d3d.particleIB->Unlock();

	// unlock main vertex and index buffers
	d3d.mainVB->Unlock();
	d3d.mainIB->Unlock();

	// unlock ortho vertex and index buffers
	d3d.orthoVB->Unlock();
	d3d.orthoIB->Unlock();

	if (!staticDecalsTest)
	{
		d3d.decalIB->Unlock();
		staticDecalsTest = true;
	}

	d3d.decalVB->Unlock();

	return true;
}

float cot(float in)
{
	return 1.0f / tan(in);
}

// REMOVE ME 
void DrawRHWquad(uint32_t x, uint32_t y, uint32_t width, uint32_t height, texID_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType);

static bool ExecuteBuffer()
{
	// sort the list of render objects
//	particleList->Sort();
//	decalList->Sort();
//	mainList->Sort();

	// Melanikus 18/11/11 - Nvidia 3D Vision fix
	// Setting the projection matrix to 
	// the rhw shader so we can have a non
	// ortho projection
	R_MATRIX projection;
	D3DXMatrixPerspectiveFovRH(&projection,D3DXToRadian(90.0f),4.0f/3.0f,0.0f,1.0f);
	d3d.rhwDecl->Set();
	d3d.effectSystem->SetActive(d3d.rhwEffect);
	d3d.effectSystem->SetVertexShaderConstant(d3d.rhwEffect, 0, CONST_MATRIX, &d3d.matProjection);

	// these two just add the vertex data to the below lists (they dont draw anything themselves
	// and they HAVE to be called before the below code)
	DrawParticles();
	DrawCoronas();

	if (mainList->GetSize())
	{
		// set vertex declaration
		d3d.mainDecl->Set();

		// set main rendering vb and ibs to active
		d3d.mainVB->Set();
		d3d.mainIB->Set();

		// set main shaders to active
		d3d.effectSystem->SetActive(d3d.mainEffect);

		// we don't need world matrix here as avp has done all the world transforms itself
		R_MATRIX matWorldViewProj = d3d.matView * d3d.matProjection;

		d3d.effectSystem->SetVertexShaderConstant(d3d.mainEffect, 0, CONST_MATRIX, &matWorldViewProj);

		// draw our main list (level geometry, player weapon etc)
		mainList->Draw();
	}

	// render any decals
	if (decalList->GetSize())
	{
		// set vertex declaration
		d3d.decalDecl->Set();

		d3d.decalVB->Set();
		d3d.decalIB->Set();

		// set decal shaders as active
		d3d.effectSystem->SetActive(d3d.decalEffect);

		R_MATRIX matWorldViewProj = d3d.matView * d3d.matProjection;

		// pass the projection matrix to the vertex shader
		d3d.effectSystem->SetVertexShaderConstant(d3d.decalEffect, 0, CONST_MATRIX, &matWorldViewProj);

		// daw the ortho list
		decalList->Draw();
	}

	// render any particles
	if (particleList->GetSize())
	{
		// set vertex declaration
		d3d.particleDecl->Set();

		// set the particle VB and IBs as active
		d3d.particleVB->Set();
		d3d.particleIB->Set();

		// set main shaders to active
		d3d.effectSystem->SetActive(d3d.particleEffect);

		d3d.effectSystem->SetVertexShaderConstant(d3d.particleEffect, 0, CONST_MATRIX, &d3d.matProjection);

		// Draw the particles in the list
		particleList->Draw();
	}

	// render any orthographic quads
	if (orthoList->GetSize())
	{
		// set vertex declaration
		d3d.orthoDecl->Set();

		d3d.orthoVB->Set();
		d3d.orthoIB->Set();

		// set orthographic projection shaders as active
		d3d.effectSystem->SetActive(d3d.orthoEffect);

		// pass the orthographic projection matrix to the vertex shader
		d3d.effectSystem->SetVertexShaderConstant(d3d.orthoEffect, 0, CONST_MATRIX, &d3d.matOrtho);

		// daw the ortho list
		orthoList->Draw();
	}

	return true;
}

void DrawFmvFrame(uint32_t frameWidth, uint32_t frameHeight, const std::vector<texID_t> &textureIDs)
{
#ifdef _XBOX
	return;
#endif

	// offset the video vertically in the centre of the screen
	uint32_t topX = (640 - frameWidth) / 2;
	uint32_t topY = (480 - frameHeight) / 2;

	float x1 = WPos2DC(topX);
	float y1 = HPos2DC(topY);

	float x2 = WPos2DC(topX + frameWidth);
	float y2 = HPos2DC(topY + frameHeight);

	// for non power of 2 texture UV adjustment - get top two textures (tex 2 and 3 should be the same resolution)
	Texture tempTexture  = Tex_GetTextureDetails(textureIDs[0]);
	Texture tempTexture2 = Tex_GetTextureDetails(textureIDs[1]);

	FMVVERTEX fmvVerts[4];

	// bottom left
	fmvVerts[0].x = x1;
	fmvVerts[0].y = y2;
	fmvVerts[0].z = 1.0f;
	fmvVerts[0].u1 = 0.0f;
	fmvVerts[0].v1 = (1.0f / tempTexture.realHeight) * tempTexture.height;

	fmvVerts[0].u2 = 0.0f;
	fmvVerts[0].v2 = (1.0f / tempTexture2.realHeight) * tempTexture2.height;

	fmvVerts[0].u3 = 0.0f;
	fmvVerts[0].v3 = (1.0f / tempTexture2.realHeight) * tempTexture2.height;

	// top left
	fmvVerts[1].x = x1;
	fmvVerts[1].y = y1;
	fmvVerts[1].z = 1.0f;
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
	fmvVerts[2].u1 = (1.0f / tempTexture.realWidth) * tempTexture.width;
	fmvVerts[2].v1 = (1.0f / tempTexture.realHeight) * tempTexture.height;

	fmvVerts[2].u2 = (1.0f / tempTexture2.realWidth) * tempTexture2.width;
	fmvVerts[2].v2 = (1.0f / tempTexture2.realHeight) * tempTexture2.height;

	fmvVerts[2].u3 = (1.0f / tempTexture2.realWidth) * tempTexture2.width;
	fmvVerts[2].v3 = (1.0f / tempTexture2.realHeight) * tempTexture2.height;

	// top right
	fmvVerts[3].x = x2;
	fmvVerts[3].y = y1;
	fmvVerts[3].z = 1.0f;
	fmvVerts[3].u1 = (1.0f / tempTexture.realWidth) * tempTexture.width;
	fmvVerts[3].v1 = 0.0f;

	fmvVerts[3].u2 = (1.0f / tempTexture2.realWidth) * tempTexture2.width;
	fmvVerts[3].v2 = 0.0f;

	fmvVerts[3].u3 = (1.0f / tempTexture2.realWidth) * tempTexture2.width;
	fmvVerts[3].v3 = 0.0f;

	d3d.fmvDecl->Set();

	// set the YUV->RGB FMV shader
	d3d.effectSystem->SetActive(d3d.fmvEffect);

	// set orthographic projection
	d3d.effectSystem->SetVertexShaderConstant(d3d.fmvEffect, 0, CONST_MATRIX, &d3d.matOrtho);

	// set the texture
	for (size_t i = 0; i < textureIDs.size(); i++)
	{
		R_SetTexture(i, textureIDs[i]);
		ChangeTextureAddressMode(i, TEXTURE_CLAMP);
		ChangeFilteringMode(i, FILTERING_BILINEAR_OFF);
	}

	ChangeTranslucencyMode(TRANSLUCENCY_OFF);

	LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &fmvVerts[0], sizeof(FMVVERTEX));
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("DrawPrimitiveUP failed\n");
	}
}

void DrawProgressBar(const RECT &srcRect, const RECT &destRect, texID_t textureID)
{
	float x1 = WPos2DC(destRect.left);
	float y1 = HPos2DC(destRect.top);

	float x2 = WPos2DC(destRect.right);
	float y2 = HPos2DC(destRect.bottom);

	Texture thisTexture = Tex_GetTextureDetails(textureID);

	float RecipW = (1.0f / thisTexture.realWidth);
	float RecipH = (1.0f / thisTexture.realHeight);

	orthoList->AddItem(4, textureID, TRANSLUCENCY_OFF, FILTERING_BILINEAR_ON, TEXTURE_CLAMP);

	RCOLOR colour = RCOLOR_XRGB(255, 255, 255);

	// bottom left
	orthoVertex[orthoVBOffset].x = x1;
	orthoVertex[orthoVBOffset].y = y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (float)((thisTexture.width - srcRect.left) * RecipW);
	orthoVertex[orthoVBOffset].v = (float)((thisTexture.height - srcRect.bottom) * RecipH);
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = x1;
	orthoVertex[orthoVBOffset].y = y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (float)((thisTexture.width - srcRect.left) * RecipW);
	orthoVertex[orthoVBOffset].v = (float)((thisTexture.height - srcRect.top) * RecipH);
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = x2;
	orthoVertex[orthoVBOffset].y = y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (float)((thisTexture.width - srcRect.right) * RecipW);
	orthoVertex[orthoVBOffset].v = (float)((thisTexture.height - srcRect.bottom) * RecipH);
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = x2;
	orthoVertex[orthoVBOffset].y = y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (float)((thisTexture.width - srcRect.right) * RecipW);
	orthoVertex[orthoVBOffset].v = (float)((thisTexture.height - srcRect.top) * RecipH);
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

void DrawTallFontCharacter(uint32_t topX, uint32_t topY, texID_t textureID, uint32_t texU, uint32_t texV, uint32_t charWidth, uint32_t charHeight, uint32_t alpha)
{
	alpha = (alpha / 256);
	if (alpha > 255)
		alpha = 255;

	RCOLOR colour = RCOLOR_ARGB(alpha, 255, 255, 255);

	Texture tempTexture = Tex_GetTextureDetails(textureID);

	float RecipW = 1.0f / tempTexture.realWidth;
	float RecipH = 1.0f / tempTexture.realHeight;

	float x1 = WPos2DC(topX);
	float y1 = HPos2DC(topY);

	float x2 = WPos2DC(topX + charWidth);
	float y2 = HPos2DC(topY + charHeight);

	if ((/*d3d.supportsShaders*/1 ) && (!IsDemoVersion()))
	{
		R_SetTexture(0, textureID);
		R_SetTexture(1, AVPMENUGFX_CLOUDY);

		// set vertex declaration
		d3d.tallTextDecl->Set();

		// set orthographic projection shaders as active
		d3d.effectSystem->SetActive(d3d.tallTextEffect);

		// we need to pass these values as floats
		float CloakingPhaseF = static_cast<float>(CloakingPhase);
		float topXF = static_cast<float>(topX);

		d3d.effectSystem->SetVertexShaderConstant(d3d.tallTextEffect, 0, CONST_FLOAT,  &CloakingPhaseF);
		d3d.effectSystem->SetVertexShaderConstant(d3d.tallTextEffect, 1, CONST_MATRIX, &d3d.matOrtho);
		d3d.effectSystem->SetVertexShaderConstant(d3d.tallTextEffect, 2, CONST_FLOAT,  &topXF);

		ChangeTextureAddressMode(0, TEXTURE_CLAMP);
		ChangeTextureAddressMode(1, TEXTURE_WRAP);
		ChangeTranslucencyMode(TRANSLUCENCY_GLOWING);
		ChangeFilteringMode(0, FILTERING_BILINEAR_ON);
		ChangeFilteringMode(1, FILTERING_BILINEAR_ON);

		TALLFONT textQuad[4];

		// bottom left
		textQuad[0].x = x1;
		textQuad[0].y = y2;
		textQuad[0].z = 1.0f;
		textQuad[0].colour = colour;
		textQuad[0].u1 = (float)((texU) * RecipW);
		textQuad[0].v1 = (float)((texV + charHeight) * RecipH);

		textQuad[0].u2 = 0.0f;
		textQuad[0].v2 = 1.0f;

		// top left
		textQuad[1].x = x1;
		textQuad[1].y = y1;
		textQuad[1].z = 1.0f;
		textQuad[1].colour = colour;
		textQuad[1].u1 = (float)((texU) * RecipW);
		textQuad[1].v1 = (float)((texV) * RecipH);

		textQuad[1].u2 = 0.0f;
		textQuad[1].v2 = 0.0f;

		// bottom right
		textQuad[2].x = x2;
		textQuad[2].y = y2;
		textQuad[2].z = 1.0f;
		textQuad[2].colour = colour;
		textQuad[2].u1 = (float)((texU + charWidth) * RecipW);
		textQuad[2].v1 = (float)((texV + charHeight) * RecipH);

		textQuad[2].u2 = 1.0f;
		textQuad[2].v2 = 1.0f;

		// top right
		textQuad[3].x = x2;
		textQuad[3].y = y1;
		textQuad[3].z = 1.0f;
		textQuad[3].colour = colour;
		textQuad[3].u1 = (float)((texU + charWidth) * RecipW);
		textQuad[3].v1 = (float)((texV) * RecipH);

		textQuad[3].u2 = 1.0f;
		textQuad[3].v2 = 0.0f;

		LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, textQuad, sizeof(TALLFONT));
		if (FAILED(LastError)) 
		{
			OutputDebugString("DrawTallFontCharacter quad draw failed\n");
		}
	}
	else
	{
		orthoList->AddItem(4, textureID, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, TEXTURE_CLAMP);

		// bottom left
		orthoVertex[orthoVBOffset].x = x1;
		orthoVertex[orthoVBOffset].y = y2;
		orthoVertex[orthoVBOffset].z = 1.0f;
		orthoVertex[orthoVBOffset].colour = colour;
		orthoVertex[orthoVBOffset].u = (float)((texU) * RecipW);
		orthoVertex[orthoVBOffset].v = (float)((texV + charHeight) * RecipH);
		orthoVBOffset++;

		// top left
		orthoVertex[orthoVBOffset].x = x1;
		orthoVertex[orthoVBOffset].y = y1;
		orthoVertex[orthoVBOffset].z = 1.0f;
		orthoVertex[orthoVBOffset].colour = colour;
		orthoVertex[orthoVBOffset].u = (float)((texU) * RecipW);
		orthoVertex[orthoVBOffset].v = (float)((texV) * RecipH);
		orthoVBOffset++;

		// bottom right
		orthoVertex[orthoVBOffset].x = x2;
		orthoVertex[orthoVBOffset].y = y2;
		orthoVertex[orthoVBOffset].z = 1.0f;
		orthoVertex[orthoVBOffset].colour = colour;
		orthoVertex[orthoVBOffset].u = (float)((texU + charWidth) * RecipW);
		orthoVertex[orthoVBOffset].v = (float)((texV + charHeight) * RecipH);
		orthoVBOffset++;

		// top right
		orthoVertex[orthoVBOffset].x = x2;
		orthoVertex[orthoVBOffset].y = y1;
		orthoVertex[orthoVBOffset].z = 1.0f;
		orthoVertex[orthoVBOffset].colour = colour;
		orthoVertex[orthoVBOffset].u = (float)((texU + charWidth) * RecipW);
		orthoVertex[orthoVBOffset].v = (float)((texV) * RecipH);
		orthoVBOffset++;

		orthoList->CreateOrthoIndices(orthoIndex);
	}
}

void SetupFMVTexture(FMVTEXTURE *ftPtr)
{
	ftPtr->RGBBuffer = new uint8_t[128 * 128 * sizeof(uint32_t)];
	ftPtr->SoundVolume = 0;
}

void UpdateFMVTexture(FMVTEXTURE *ftPtr)
{
	assert(ftPtr);

	if (!ftPtr)
		return;

	if (!NextFMVTextureFrame(ftPtr))
	{
	 	return;
	}

	uint32_t width = 0;
	uint32_t height = 0;
	Tex_GetDimensions(ftPtr->textureID, width, height);

	uint8_t *srcPtr = &ftPtr->RGBBuffer[0];
	uint8_t *destPtr = 0;
	uint8_t *originalPtr = 0;
	uint32_t pitch = 0;

	if (!Tex_Lock(ftPtr->textureID, &originalPtr, &pitch, TextureLock_Discard))
	{
		// don't touch the pointer if the lock failed
		return;
	}

	for (uint32_t y = 0; y < height; y++)
	{
		destPtr = originalPtr + y * pitch;

		for (uint32_t x = 0; x < width; x++)
		{
			memcpy(destPtr, srcPtr, sizeof(uint32_t));

			destPtr += sizeof(uint32_t);
			srcPtr  += sizeof(uint32_t);
		}
	}

	Tex_Unlock(ftPtr->textureID);
}

void DrawFadeQuad(uint32_t topX, uint32_t topY, uint32_t alpha)
{
	alpha = alpha / 256;
	if (alpha > 255)
		alpha = 255;

	RCOLOR colour = RCOLOR_ARGB(alpha,0,0,0);

	orthoList->AddItem(4, NO_TEXTURE, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, TEXTURE_WRAP);

	// bottom left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = 1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = 1.0f;;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

void DrawFontQuad(uint32_t x, uint32_t y, uint32_t charWidth, uint32_t charHeight, texID_t textureID, float *uvArray, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType)
{
	float x1 = WPos2DC(x);
	float y1 = HPos2DC(y);

	float x2 = WPos2DC(x + charWidth);
	float y2 = HPos2DC(y + charHeight);

	orthoList->AddItem(4, textureID, translucencyType, FILTERING_BILINEAR_OFF, TEXTURE_CLAMP);

	// bottom left
	orthoVertex[orthoVBOffset].x = x1;
	orthoVertex[orthoVBOffset].y = y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = uvArray[0];
	orthoVertex[orthoVBOffset].v = uvArray[1];
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = x1;
	orthoVertex[orthoVBOffset].y = y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = uvArray[2];
	orthoVertex[orthoVBOffset].v = uvArray[3];
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = x2;
	orthoVertex[orthoVBOffset].y = y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = uvArray[4];
	orthoVertex[orthoVBOffset].v = uvArray[5];
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = x2;
	orthoVertex[orthoVBOffset].y = y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = uvArray[6];
	orthoVertex[orthoVBOffset].v = uvArray[7];
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

void DrawRHWquad(uint32_t x, uint32_t y, uint32_t width, uint32_t height, texID_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType)
{
	RHW_VERTEX rhw[4];

	Texture tempTexture  = Tex_GetTextureDetails(textureID);

	// bottom left
	rhw[0].x = x;
	rhw[0].y = y + height;
	rhw[0].z = 1.0f;
	rhw[0].rhw = 1.0f;
	rhw[0].color = colour;
	rhw[0].u = 0.0f;
	rhw[0].v = (1.0f / tempTexture.realHeight) * tempTexture.height;

	// top left
	rhw[1].x = x;
	rhw[1].y = y;
	rhw[1].z = 1.0f;
	rhw[1].rhw = 1.0f;
	rhw[1].color = colour;
	rhw[1].u = 0.0f;
	rhw[1].v = 0.0f;

	// bottom right
	rhw[2].x = x + width;
	rhw[2].y = y + height;
	rhw[2].z = 1.0f;
	rhw[2].rhw = 1.0f;
	rhw[2].color = colour;
	rhw[2].u = (1.0f / tempTexture.realWidth) * tempTexture.width;
	rhw[2].v = (1.0f / tempTexture.realHeight) * tempTexture.height;

	// top right
	rhw[3].x = x + width;
	rhw[3].y = y;
	rhw[3].z = 1.0f;
	rhw[3].rhw = 1.0f;
	rhw[3].color = colour;
	rhw[3].u = (1.0f / tempTexture.realWidth) * tempTexture.width;
	rhw[3].v = 0.0f;

	d3d.rhwDecl->Set();

	// set the YUV FMV shader
	d3d.effectSystem->SetActive(d3d.rhwEffect);

	// set orthographic projection
//	d3d.effectSystem->SetVertexShaderConstant(d3d.rhwEffect, 0, CONST_MATRIX, &d3d.matOrtho);

	R_SetTexture(0, textureID);

	ChangeTextureAddressMode(0, TEXTURE_CLAMP);
	ChangeFilteringMode(0, FILTERING_BILINEAR_OFF);
	ChangeTranslucencyMode(TRANSLUCENCY_OFF);

	LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &rhw[0], sizeof(RHW_VERTEX));
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("DrawPrimitiveUP failed\n");
	}

}

void DrawQuad(uint32_t x, uint32_t y, uint32_t width, uint32_t height, texID_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType)
{
	float x1 = WPos2DC(x);
	float y1 = HPos2DC(y);

	float x2 = WPos2DC(x + width);
	float y2 = HPos2DC(y + height);

	Texture tempTexture = Tex_GetTextureDetails(textureID);

	orthoList->AddItem(4, textureID, translucencyType, FILTERING_BILINEAR_ON, TEXTURE_CLAMP);

	// bottom left
	orthoVertex[orthoVBOffset].x = x1;
	orthoVertex[orthoVBOffset].y = y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = (1.0f / tempTexture.realHeight) * tempTexture.height;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = x1;
	orthoVertex[orthoVBOffset].y = y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = x2;
	orthoVertex[orthoVBOffset].y = y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (1.0f / tempTexture.realWidth) * tempTexture.width;
	orthoVertex[orthoVBOffset].v = (1.0f / tempTexture.realHeight) * tempTexture.height;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = x2;
	orthoVertex[orthoVBOffset].y = y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (1.0f / tempTexture.realWidth) * tempTexture.width;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

void DrawAlphaMenuQuad(uint32_t topX, uint32_t topY, texID_t textureID, uint32_t alpha)
{
	// textures actual height/width (whether it's non power of two or not)
	uint32_t textureWidth, textureHeight;
	Tex_GetDimensions(textureID, textureWidth, textureHeight);

	alpha = (alpha / 256);
	if (alpha > 255)
		alpha = 255;

	DrawQuad(topX, topY, textureWidth, textureHeight, textureID, RCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);
}

void DrawMenuTextGlow(uint32_t topLeftX, uint32_t topLeftY, uint32_t size, uint32_t alpha)
{
	uint32_t textureWidth  = 0;
	uint32_t textureHeight = 0;

	if (alpha > ONE_FIXED) // ONE_FIXED = 65536
		alpha = ONE_FIXED;

	alpha = (alpha / 256);
	if (alpha > 255)
		alpha = 255;

	// textures original resolution (if it's a non power of 2, these will be the non power of 2 values)
	Tex_GetDimensions(AVPMENUGFX_GLOWY_LEFT, textureWidth, textureHeight);

	// do the text alignment justification
	topLeftX -= textureWidth;

	DrawQuad(topLeftX, topLeftY, textureWidth, textureHeight, AVPMENUGFX_GLOWY_LEFT, RCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);

	// now do the middle section
	topLeftX += textureWidth;

	Tex_GetDimensions(AVPMENUGFX_GLOWY_MIDDLE, textureWidth, textureHeight);

	DrawQuad(topLeftX, topLeftY, textureWidth * size, textureHeight, AVPMENUGFX_GLOWY_MIDDLE, RCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);

	// now do the right section
	topLeftX += textureWidth * size;

	Tex_GetDimensions(AVPMENUGFX_GLOWY_RIGHT, textureWidth, textureHeight);

	DrawQuad(topLeftX, topLeftY, textureWidth, textureHeight, AVPMENUGFX_GLOWY_RIGHT, RCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);
}

void DrawSmallMenuCharacter(uint32_t topX, uint32_t topY, uint32_t texU, uint32_t texV, uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha)
{
	alpha = (alpha / 256);
	red   = (red   / 256);
	green = (green / 256);
	blue  = (blue  / 256);

	// clamp if needed
	if (alpha > 255) alpha = 255;
	if (red   > 255) red   = 255;
	if (green > 255) green = 255;
	if (blue  > 255) blue  = 255;

	RCOLOR colour = RCOLOR_ARGB(alpha, 255, 255, 255);

	uint32_t font_height = HUD_FONT_HEIGHT;
	uint32_t font_width  = HUD_FONT_WIDTH;

	// aa_font.bmp is 256 x 256
	uint32_t textureWidth  = 0;
	uint32_t textureHeight = 0;

	Tex_GetDimensions(AVPMENUGFX_SMALL_FONT, textureWidth, textureHeight);

	float RecipW = 1.0f / textureWidth; // 0.00390625
	float RecipH = 1.0f / textureHeight;

	float x1 = WPos2DC(topX);
	float y1 = HPos2DC(topY);

	float x2 = WPos2DC(topX + font_width);
	float y2 = HPos2DC(topY + font_height);

	orthoList->AddItem(4, AVPMENUGFX_SMALL_FONT, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, TEXTURE_CLAMP);

	// bottom left
	orthoVertex[orthoVBOffset].x = x1;
	orthoVertex[orthoVBOffset].y = y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (float)((texU) * RecipW);
	orthoVertex[orthoVBOffset].v = (float)((texV + font_height) * RecipH);
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = x1;
	orthoVertex[orthoVBOffset].y = y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (float)((texU) * RecipW);
	orthoVertex[orthoVBOffset].v = (float)((texV) * RecipH);
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = x2;
	orthoVertex[orthoVBOffset].y = y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (float)((texU + font_height) * RecipW);
	orthoVertex[orthoVBOffset].v = (float)((texV + font_height) * RecipH);
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = x2;
	orthoVertex[orthoVBOffset].y = y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = (float)((texU + font_width) * RecipW);
	orthoVertex[orthoVBOffset].v = (float)((texV) * RecipH);
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

void TransformToViewspace(VECTORCHF *vector)
{
	D3DXVECTOR3 output;
	D3DXVECTOR3 input(vector->vx, vector->vy, vector->vz);

	D3DXVec3TransformCoord(&output, &input, &d3d.matView);

	vector->vx = output.x;
	vector->vy = output.y;
	vector->vz = output.z;
}

void UpdateProjectionMatrix()
{
	// manually generate the perspective matrix as per http://msdn.microsoft.com/en-us/library/bb205350(VS.85).aspx
	float fovY = D3DXToRadian(d3d.fieldOfView * CameraZoomScale);
	float aspect = (float)ScreenDescriptorBlock.SDB_Width / (float)ScreenDescriptorBlock.SDB_Height;
	float yScale = cot(fovY/2.0f);

	float originalYScale = yScale;
	yScale *= p;

	float xScale = originalYScale / aspect;
	aspect /= p;

	xScale *= o;

	// column 1
	d3d.matProjection._11 = xScale;
	d3d.matProjection._21 = 0.0f;
	d3d.matProjection._31 = 0.0f;
	d3d.matProjection._41 = 0.0f;

	// column 2
	d3d.matProjection._12 = 0.0f;
	d3d.matProjection._22 = yScale;
	d3d.matProjection._32 = 0.0f;
	d3d.matProjection._42 = 0.0f;

	// column 3
	d3d.matProjection._13 = 0.0f;
	d3d.matProjection._23 = 0.0f;
	d3d.matProjection._33 = zFar/(zFar-zNear);
	d3d.matProjection._43 = -zNear*zFar/(zFar-zNear);

	// column 4
	d3d.matProjection._14 = 0.0f;
	d3d.matProjection._24 = 0.0f;
	d3d.matProjection._34 = 1.0f;
	d3d.matProjection._44 = 0.0f;
}

void UpdateViewMatrix(float *viewMat)
{	
	/*
	 * Melanikus - 09/11/11
	 * We inverted vecFront's Y component because vecUp is upside down (VecUp points to -Y).
	 * We forgot to invert vecRight's Y component, an error that could only be noticed with 
	 * the alien climbing a wall and looking to the Up axis relative to that wall. The error 
	 * was due the different axis between world player orientation and local camera matrix.
	 */ 
	D3DXVECTOR3 vecRight	(viewMat[0], -viewMat[1], viewMat[2]);
	D3DXVECTOR3 vecUp		(viewMat[4],  viewMat[5], viewMat[6]);
	D3DXVECTOR3 vecFront	(viewMat[8], -viewMat[9], viewMat[10]);
	D3DXVECTOR3 vecPosition (viewMat[3], -viewMat[7], viewMat[11]);

	D3DXVec3Normalize(&vecFront, &vecFront);

	D3DXVec3Cross(&vecUp, &vecFront, &vecRight);
	D3DXVec3Normalize(&vecUp, &vecUp);

	D3DXVec3Cross(&vecRight, &vecUp, &vecFront);
	D3DXVec3Normalize(&vecRight, &vecRight);

	// right
	d3d.matView._11 = vecRight.x;
	d3d.matView._21 = vecRight.y;
	d3d.matView._31 = vecRight.z;

	// up
	d3d.matView._12 = vecUp.x;
	d3d.matView._22 = vecUp.y;
	d3d.matView._32 = vecUp.z;

	// front
	d3d.matView._13 = vecFront.x;
	d3d.matView._23 = vecFront.y;
	d3d.matView._33 = vecFront.z;

	// 4th
	d3d.matView._14 = 0.0f;
	d3d.matView._24 = 0.0f;
	d3d.matView._34 = 0.0f;
	d3d.matView._44 = 1.0f;

	d3d.matView._41 = -D3DXVec3Dot(&vecPosition, &vecRight);
	d3d.matView._42 = -D3DXVec3Dot(&vecPosition, &vecUp);
	d3d.matView._43 = -D3DXVec3Dot(&vecPosition, &vecFront);
}

void DrawCoronas()
{
	if (coronaArray.size() == 0)
		return;

	uint32_t numVertsBackup = RenderPolygon.NumberOfVertices;

	RCOLOR colour;

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(SpecialFXImageNumber, texWidth, texHeight);

	float RecipW = 1.0f / (float) texWidth;
	float RecipH = 1.0f / (float) texHeight;

	for (size_t i = 0; i < coronaArray.size(); i++)
	{
		PARTICLE_DESC *particleDescPtr = &ParticleDescription[coronaArray[i].particle.ParticleID];

		assert (coronaArray[i].numVerts == 4);

		if (particleDescPtr->IsLit && !(coronaArray[i].particle.ParticleID == PARTICLE_ALIEN_BLOOD && CurrentVisionMode == VISION_MODE_PRED_SEEALIENS))
		{
			int intensity = LightIntensityAtPoint(&coronaArray[i].particle.Position);
			if (coronaArray[i].particle.ParticleID == PARTICLE_SMOKECLOUD || coronaArray[i].particle.ParticleID == PARTICLE_ANDROID_BLOOD)
			{
				colour = RGBA_MAKE
						(
							MUL_FIXED(intensity, coronaArray[i].particle.ColourComponents.Red),
							MUL_FIXED(intensity, coronaArray[i].particle.ColourComponents.Green),
							MUL_FIXED(intensity, coronaArray[i].particle.ColourComponents.Blue),
							coronaArray[i].particle.ColourComponents.Alpha
						);
			}
			else
			{
				colour = RGBA_MAKE
						(
							MUL_FIXED(intensity, particleDescPtr->RedScale[CurrentVisionMode]),
							MUL_FIXED(intensity, particleDescPtr->GreenScale[CurrentVisionMode]),
							MUL_FIXED(intensity, particleDescPtr->BlueScale[CurrentVisionMode]),
							particleDescPtr->Alpha
						);
			}
		}
		else
		{
			colour = coronaArray[i].particle.Colour;
		}

		if (RAINBOWBLOOD_CHEATMODE)
		{
			colour = RGBA_MAKE
						(
							FastRandom()&255,
							FastRandom()&255,
							FastRandom()&255,
							particleDescPtr->Alpha
						);
		}

		// transform our coronas worldview position into projection then viewport space
		D3DXVECTOR3 transformedPoint;

		transformedPoint.x = coronaArray[i].coronaPoint.vx;
		transformedPoint.y = coronaArray[i].coronaPoint.vy;
		transformedPoint.z = coronaArray[i].coronaPoint.vz;

		float centreX = transformedPoint.x / transformedPoint.z;
		float centreY = transformedPoint.y / transformedPoint.z;

		D3DXMATRIX finalMatrix = d3d.matProjection * d3d.matViewPort;

		// already view transformed, do projection transform and viewport transform
		D3DXVec3TransformCoord(&transformedPoint, &transformedPoint, &finalMatrix);

		// generate the quad around this point
		uint32_t size = 100;

		uint32_t sizeX = (ScreenDescriptorBlock.SDB_Width / 100) * 10;
		uint32_t sizeY = sizeX;//(ScreenDescriptorBlock.SDB_Height / 100) * 11;

		orthoList->AddItem(4, SpecialFXImageNumber, (enum TRANSLUCENCY_TYPE)particleDescPtr->TranslucencyType, FILTERING_BILINEAR_ON, TEXTURE_CLAMP, ZWRITE_DISABLED);

		// bottom left
		orthoVertex[orthoVBOffset].x = WPos2DC(transformedPoint.x - sizeX);
		orthoVertex[orthoVBOffset].y = HPos2DC(transformedPoint.y + sizeY);
		orthoVertex[orthoVBOffset].z = 1.0f;
		orthoVertex[orthoVBOffset].colour = colour;
		orthoVertex[orthoVBOffset].u = 192.0f * RecipW;
		orthoVertex[orthoVBOffset].v = 63.0f * RecipH;
		orthoVBOffset++;

		// top left
		orthoVertex[orthoVBOffset].x = WPos2DC(transformedPoint.x - sizeX);
		orthoVertex[orthoVBOffset].y = HPos2DC(transformedPoint.y - sizeY);
		orthoVertex[orthoVBOffset].z = 1.0f;
		orthoVertex[orthoVBOffset].colour = colour;
		orthoVertex[orthoVBOffset].u = 192.0f * RecipW;
		orthoVertex[orthoVBOffset].v = 0.0f * RecipH;
		orthoVBOffset++;

		// bottom right
		orthoVertex[orthoVBOffset].x = WPos2DC(transformedPoint.x + sizeX);
		orthoVertex[orthoVBOffset].y = HPos2DC(transformedPoint.y + sizeY);
		orthoVertex[orthoVBOffset].z = 1.0f;
		orthoVertex[orthoVBOffset].colour = colour;
		orthoVertex[orthoVBOffset].u = 255.0f * RecipW;
		orthoVertex[orthoVBOffset].v = 63.0f * RecipH;
		orthoVBOffset++;

		// top right
		orthoVertex[orthoVBOffset].x = WPos2DC(transformedPoint.x + sizeX);
		orthoVertex[orthoVBOffset].y = HPos2DC(transformedPoint.y - sizeY);
		orthoVertex[orthoVBOffset].z = 1.0f;
		orthoVertex[orthoVBOffset].colour = colour;
		orthoVertex[orthoVBOffset].u = 255.0f * RecipW;
		orthoVertex[orthoVBOffset].v = 0.0f * RecipH;
		orthoVBOffset++;

		orthoList->CreateOrthoIndices(orthoIndex);
	}

	coronaArray.clear();

	// restore RenderPolygon.NumberOfVertices value...
	RenderPolygon.NumberOfVertices = numVertsBackup;
}

void DrawParticles()
{
	if (!particleBucket.size())
		return;

	// loop particles and add them to vertex buffer
	for (size_t i = 0; i < NUM_TRANSLUCENCY_TYPES; i++)
	{
		for (size_t j = 0; j < particleBucket[i].size(); j++)
		{
			D3D_Particle_Output(&particleBucket[i][j].particle, &particleBucket[i][j].vertices[0]);
		}
	}

	for (size_t i = 0; i < NUM_TRANSLUCENCY_TYPES; i++)
	{
		particleBucket[i].clear();
	}
}

void D3D_ZBufferedGouraudTexturedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	// bjd - This is the function responsible for drawing level geometry and the players weapon

	texID_t textureID = (inputPolyPtr->PolyColour & ClrTxDefn);

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	float RecipW = 1.0f / (float) texWidth;
	float RecipH = 1.0f / (float) texHeight;

	mainList->AddItem(RenderPolygon.NumberOfVertices, textureID, RenderPolygon.TranslucencyMode);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].x = (float)vertices->X;
		mainVertex[vb].y = (float)-vertices->Y;
		mainVertex[vb].z = (float)vertices->Z;

		mainVertex[vb].color = RGBA_MAKE(GammaValues[vertices->R], GammaValues[vertices->G], GammaValues[vertices->B], vertices->A);
		mainVertex[vb].specular = RGBA_MAKE(GammaValues[vertices->SpecularR], GammaValues[vertices->SpecularG], GammaValues[vertices->SpecularB], /*255*/0);

		mainVertex[vb].u = (float)(vertices->U) * RecipW;
		mainVertex[vb].v = (float)(vertices->V) * RecipH;
		vb++;
	}

	mainList->CreateIndices(mainIndex, RenderPolygon.NumberOfVertices);
}

void D3D_ZBufferedGouraudPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	// responsible for drawing predators lock on triangle, for one

	// Take header information
	int32_t flags = inputPolyPtr->PolyFlags;

	mainList->AddItem(RenderPolygon.NumberOfVertices, NO_TEXTURE, RenderPolygon.TranslucencyMode);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].x = (float)vertices->X;
		mainVertex[vb].y = (float)-vertices->Y;
		mainVertex[vb].z = (float)vertices->Z;

		if (flags & iflag_transparent)
		{
	  		mainVertex[vb].color = RGBA_MAKE(vertices->R, vertices->G, vertices->B, vertices->A);
		}
		else
		{
			mainVertex[vb].color = RGBA_MAKE(vertices->R, vertices->G, vertices->B, 255);
		}

		mainVertex[vb].specular = RGB_MAKE(255, 255, 255);
		mainVertex[vb].u = 0.0f;
		mainVertex[vb].v = 0.0f;
		vb++;
	}

	mainList->CreateIndices(mainIndex, RenderPolygon.NumberOfVertices);
}

void D3D_PredatorThermalVisionPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	mainList->AddItem(RenderPolygon.NumberOfVertices, NO_TEXTURE, TRANSLUCENCY_OFF);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].x = (float)vertices->X;
		mainVertex[vb].y = (float)-vertices->Y;
		mainVertex[vb].z = (float)vertices->Z;

		mainVertex[vb].color = RGBA_MAKE(vertices->R, vertices->G, vertices->B, vertices->A);
		mainVertex[vb].specular = RCOLOR_ARGB(0,0,0,0);
		mainVertex[vb].u = 0.0f;
		mainVertex[vb].v = 0.0f;
		vb++;
	}

	mainList->CreateIndices(mainIndex, RenderPolygon.NumberOfVertices);
}

void D3D_ZBufferedCloakedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	texID_t textureID = (inputPolyPtr->PolyColour & ClrTxDefn);

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	float RecipW = 1.0f / (float) texWidth;
	float RecipH = 1.0f / (float) texHeight;

	mainList->AddItem(RenderPolygon.NumberOfVertices, textureID, TRANSLUCENCY_NORMAL);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].x = (float)vertices->X;
		mainVertex[vb].y = (float)-vertices->Y;
		mainVertex[vb].z = (float)vertices->Z;

	   	mainVertex[vb].color = RGBA_MAKE(vertices->R,vertices->G,vertices->B,vertices->A);
		mainVertex[vb].specular = RGBA_MAKE(0,0,0,255);

		mainVertex[vb].u = (float)(vertices->U) * RecipW;
		mainVertex[vb].v = (float)(vertices->V) * RecipH;
		vb++;
	}

	mainList->CreateIndices(mainIndex, RenderPolygon.NumberOfVertices);
}

void D3D_Rectangle(int x0, int y0, int x1, int y1, int r, int g, int b, int a)
{
	float new_x1 = WPos2DC(x0);
	float new_y1 = HPos2DC(y0);

	float new_x2 = WPos2DC(x1);
	float new_y2 = HPos2DC(y1);

	orthoList->AddItem(4, NO_TEXTURE, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, TEXTURE_CLAMP);

	// bottom left
	orthoVertex[orthoVBOffset].x = new_x1;
	orthoVertex[orthoVBOffset].y = new_y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(r,g,b,a);
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = new_x1;
	orthoVertex[orthoVBOffset].y = new_y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(r,g,b,a);
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = new_x2;
	orthoVertex[orthoVBOffset].y = new_y2;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(r,g,b,a);
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = new_x2;
	orthoVertex[orthoVBOffset].y = new_y1;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(r,g,b,a);
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

void D3D_HUDQuad_Output(texID_t textureID, struct VertexTag *quadVerticesPtr, uint32_t colour, enum FILTERING_MODE_ID filteringType)
{
	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	float RecipW = 1.0f / texWidth;
	float RecipH = 1.0f / texHeight;

	orthoList->AddItem(4, textureID, TRANSLUCENCY_GLOWING, filteringType, TEXTURE_CLAMP);

	// bottom left
	orthoVertex[orthoVBOffset].x = WPos2DC(quadVerticesPtr[3].X);
	orthoVertex[orthoVBOffset].y = HPos2DC(quadVerticesPtr[3].Y);
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = quadVerticesPtr[3].U * RecipW;
	orthoVertex[orthoVBOffset].v = quadVerticesPtr[3].V * RecipH;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = WPos2DC(quadVerticesPtr[0].X);
	orthoVertex[orthoVBOffset].y = HPos2DC(quadVerticesPtr[0].Y);
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = quadVerticesPtr[0].U * RecipW;
	orthoVertex[orthoVBOffset].v = quadVerticesPtr[0].V * RecipH;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = WPos2DC(quadVerticesPtr[2].X);
	orthoVertex[orthoVBOffset].y = HPos2DC(quadVerticesPtr[2].Y);
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = quadVerticesPtr[2].U * RecipW;
	orthoVertex[orthoVBOffset].v = quadVerticesPtr[2].V * RecipH;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = WPos2DC(quadVerticesPtr[1].X);
	orthoVertex[orthoVBOffset].y = HPos2DC(quadVerticesPtr[1].Y);
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = quadVerticesPtr[1].U * RecipW;
	orthoVertex[orthoVBOffset].v = quadVerticesPtr[1].V * RecipH;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

void D3D_DrawParticle_Rain(PARTICLE *particlePtr, VECTORCH *prevPositionPtr)
{
	VECTORCH vertices[3];
	vertices[0] = *prevPositionPtr;

	/* translate second vertex into view space */
//	TranslatePointIntoViewspace(&vertices[0]);

	/* is particle within normal view frustum ? */

/*
	if ((-vertices[0].vx <= vertices[0].vz)
	&& (vertices[0].vx <= vertices[0].vz)
	&& (-vertices[0].vy <= vertices[0].vz)
	&& (vertices[0].vy <= vertices[0].vz))
*/
	{
		vertices[1] = particlePtr->Position;
		vertices[2] = particlePtr->Position;
		vertices[1].vx += particlePtr->Offset.vx;
		vertices[2].vx -= particlePtr->Offset.vx;
		vertices[1].vz += particlePtr->Offset.vz;
		vertices[2].vz -= particlePtr->Offset.vz;

		/* translate particle into view space */
//		TranslatePointIntoViewspace(&vertices[1]);
//		TranslatePointIntoViewspace(&vertices[2]);

		mainList->AddItem(3, NO_TEXTURE, TRANSLUCENCY_NORMAL);

		VECTORCH *verticesPtr = vertices;

		for (uint32_t i = 0; i < 3; i++)
		{
			mainVertex[vb].x = (float)verticesPtr->vx;
			mainVertex[vb].y = (float)-verticesPtr->vy;
			mainVertex[vb].z = (float)verticesPtr->vz; // bjd - CHECK

			if (i==3)
			{
				mainVertex[vb].color = RGBA_MAKE(0, 255, 255, 32);
			}
			else
			{
				mainVertex[vb].color = RGBA_MAKE(255, 255, 255, 32);
			}

			mainVertex[vb].specular = RGBA_MAKE(0,0,0,0);
			mainVertex[vb].u = 0.0f;
			mainVertex[vb].v = 0.0f;

			vb++;
			verticesPtr++;
		}

		mainList->CreateIndices(mainIndex, 3);
	}
}

void D3D_DecalSystem_Setup()
{
/*
	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	LockExecuteBuffer();
*/
}

void D3D_DecalSystem_End()
{
//	DrawParticles();
//	DrawCoronas();
/*
	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	LockExecuteBuffer();
*/
}

void D3D_Decal_Output(DECAL *decalPtr, RENDERVERTEX *renderVerticesPtr)
{
	// function responsible for bullet marks on walls, etc
	DECAL_DESC *decalDescPtr = &DecalDescription[decalPtr->DecalID];

	texID_t textureID = SpecialFXImageNumber;

	float RecipW, RecipH;
	RCOLOR colour;

	assert(RenderPolygon.NumberOfVertices == 4);

	if (decalPtr->DecalID == DECAL_SHAFTOFLIGHT || decalPtr->DecalID == DECAL_SHAFTOFLIGHT_OUTER)
	{
		textureID = NO_TEXTURE;
	}
	else
	{
		uint32_t texWidth, texHeight;
		Tex_GetDimensions(textureID, texWidth, texHeight);

		RecipW = 1.0f / (float) texWidth;
		RecipH = 1.0f / (float) texHeight;
	}

	if (decalDescPtr->IsLit)
	{
		int intensity = LightIntensityAtPoint(decalPtr->Vertices);
		colour = RGBA_MAKE
	  		  	(
	  		   		MUL_FIXED(intensity, decalDescPtr->RedScale[CurrentVisionMode]),
	  		   		MUL_FIXED(intensity, decalDescPtr->GreenScale[CurrentVisionMode]),
	  		   		MUL_FIXED(intensity, decalDescPtr->BlueScale[CurrentVisionMode]),
	  		   		decalDescPtr->Alpha
	  		   	);
	}
	else
	{
		colour = RGBA_MAKE
			  	(
			   		decalDescPtr->RedScale[CurrentVisionMode],
			   		decalDescPtr->GreenScale[CurrentVisionMode],
			   		decalDescPtr->BlueScale[CurrentVisionMode],
			   		decalDescPtr->Alpha
			   	);
	}

	if (RAINBOWBLOOD_CHEATMODE)
	{
		colour = RGBA_MAKE
				(
					FastRandom()&255,
					FastRandom()&255,
					FastRandom()&255,
					decalDescPtr->Alpha
				);
	}

	decalList->AddItem(RenderPolygon.NumberOfVertices, textureID, decalDescPtr->TranslucencyType, FILTERING_BILINEAR_ON, TEXTURE_CLAMP, ZWRITE_DISABLED);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		decalVertex[decalVb].x = (float)vertices->X;
		decalVertex[decalVb].y = (float)-vertices->Y;
		decalVertex[decalVb].z = (float)vertices->Z;

		decalVertex[decalVb].colour = colour;

		decalVertex[decalVb].u = (float)(vertices->U) * RecipW;
		decalVertex[decalVb].v = (float)(vertices->V) * RecipH;
		decalVb++;
	}

	decalList->EnableIndicesOffset();

	decalList->IncrementIndexCount(6);

//	decalList->CreateIndices(decalIndex, RenderPolygon.NumberOfVertices);
}

void AddCorona(PARTICLE *particlePtr, VECTORCHF *coronaPoint)
{
	renderCorona newCorona;

	newCorona.numVerts = RenderPolygon.NumberOfVertices;
	newCorona.particle = *particlePtr;

	memcpy(&newCorona.coronaPoint, coronaPoint, sizeof(VECTORCHF));
	newCorona.translucency = ParticleDescription[particlePtr->ParticleID].TranslucencyType;

	coronaArray.push_back(newCorona);
}

void AddParticle(PARTICLE *particlePtr, RENDERVERTEX *renderVerticesPtr)
{
	if (!pbInit)
	{
		particleBucket.resize(NUM_TRANSLUCENCY_TYPES);
		pbInit = true;
	}

	renderParticle newParticle;
	newParticle.particle = *particlePtr;

	// copy vertex data
	for (int i = 0; i < 4; i++)
	{
		newParticle.vertices[i].x = renderVerticesPtr->X;
		newParticle.vertices[i].y = renderVerticesPtr->Y;
		newParticle.vertices[i].z = renderVerticesPtr->Z;
		newParticle.vertices[i].u = renderVerticesPtr->U;
		newParticle.vertices[i].v = renderVerticesPtr->V;
		renderVerticesPtr++;
	}

	uint32_t transID = ParticleDescription[particlePtr->ParticleID].TranslucencyType;

	// add to correct bucket
	particleBucket[transID].push_back(newParticle);
}

void D3D_Particle_Output(PARTICLE *particlePtr, PARTICLEVERTEX *renderVerticesPtr)
{
	// steam jets, pulse rifle muzzle flash, wall lights, fire (inc aliens on fire) etc
	PARTICLE_DESC *particleDescPtr = &ParticleDescription[particlePtr->ParticleID];

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(SpecialFXImageNumber, texWidth, texHeight);

	float RecipW = 1.0f / (float) texWidth;
	float RecipH = 1.0f / (float) texHeight;

	particleList->AddItem(4, SpecialFXImageNumber, (enum TRANSLUCENCY_TYPE)particleDescPtr->TranslucencyType, FILTERING_BILINEAR_ON, TEXTURE_CLAMP, ZWRITE_DISABLED);

	RCOLOR colour;

	if (particleDescPtr->IsLit && !(particlePtr->ParticleID == PARTICLE_ALIEN_BLOOD && CurrentVisionMode == VISION_MODE_PRED_SEEALIENS))
	{
		int intensity = LightIntensityAtPoint(&particlePtr->Position);
		if (particlePtr->ParticleID == PARTICLE_SMOKECLOUD || particlePtr->ParticleID == PARTICLE_ANDROID_BLOOD)
		{
			colour = RGBA_MAKE
				  	(
				   		MUL_FIXED(intensity, particlePtr->ColourComponents.Red),
				   		MUL_FIXED(intensity, particlePtr->ColourComponents.Green),
				   		MUL_FIXED(intensity, particlePtr->ColourComponents.Blue),
				   		particlePtr->ColourComponents.Alpha
				   	);
		}
		else
		{
			colour = RGBA_MAKE
				  	(
				   		MUL_FIXED(intensity, particleDescPtr->RedScale[CurrentVisionMode]),
				   		MUL_FIXED(intensity, particleDescPtr->GreenScale[CurrentVisionMode]),
				   		MUL_FIXED(intensity, particleDescPtr->BlueScale[CurrentVisionMode]),
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
		colour = RGBA_MAKE
					(
						FastRandom()&255,
						FastRandom()&255,
						FastRandom()&255,
						particleDescPtr->Alpha
					);
	}

	for (uint32_t i = 0; i < 4; i++)
	{
		PARTICLEVERTEX *vertices = &renderVerticesPtr[i];

		float zvalue;

		if (particleDescPtr->IsDrawnInFront)
		{
			zvalue = 0.0f;
		}
		else if (particleDescPtr->IsDrawnAtBack)
		{
			zvalue = 1.0f;
		}
		else
		{
			zvalue = (float)vertices->z;
		}

		particleVertex[pVb].x = vertices->x;
		particleVertex[pVb].y = vertices->y;
		particleVertex[pVb].z = vertices->z;

		particleVertex[pVb].colour = colour;

		particleVertex[pVb].u = vertices->u * RecipW;
		particleVertex[pVb].v = vertices->v * RecipH;
		pVb++;
	}

	// 3: Create Indices
	particleList->CreateIndices(particleIndex, 4);
}

void PostLandscapeRendering()
{
	int numOfObjects = NumOnScreenBlocks;

	if (!strcmp(LevelName,"fall") || !strcmp(LevelName,"fall_m"))
	{
		bool drawWaterFall = false;
		bool drawStream    = false;

		while (numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			// if it's a module, which isn't inside another module
			if (modulePtr && modulePtr->name)
			{
				if ((!strcmp(modulePtr->name, "fall01"))
				  ||(!strcmp(modulePtr->name, "well01"))
				  ||(!strcmp(modulePtr->name, "well02"))
				  ||(!strcmp(modulePtr->name, "well03"))
				  ||(!strcmp(modulePtr->name, "well04"))
				  ||(!strcmp(modulePtr->name, "well05"))
				  ||(!strcmp(modulePtr->name, "well06"))
				  ||(!strcmp(modulePtr->name, "well07"))
				  ||(!strcmp(modulePtr->name, "well08"))
				  ||(!strcmp(modulePtr->name, "well")))
				{
					drawWaterFall = true;
				}
				else if ((!strcmp(modulePtr->name, "stream02"))
				       ||(!strcmp(modulePtr->name, "stream03"))
				       ||(!strcmp(modulePtr->name, "watergate")))
				{
		   			drawStream = true;
				}
			}
		}

		if (drawWaterFall)
		{
	   		//UpdateWaterFall();
			WaterFallBase = 109952;

			MeshZScale = (66572 - 51026)/15;
			MeshXScale = (109952 + 3039)/45;

	   		D3D_DrawWaterFall(175545, -3039, 51026);
		}
		if (drawStream)
		{
			int x = 68581;
			int y = 12925;
			int z = 93696;
			MeshXScale = (87869-68581);
			MeshZScale = (105385-93696);

			CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);

			WaterXOrigin = x;
			WaterZOrigin = z;
			WaterUScale = 4.0f / (float)MeshXScale;
			WaterVScale = 4.0f / (float)MeshZScale;
		 	MeshXScale /= 4;
		 	MeshZScale /= 2;

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
	else if (!_stricmp(LevelName,"invasion_a"))
	{
		bool drawWater    = false;
		bool drawEndWater = false;

		while (numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			// if it's a module, which isn't inside another module
			if (modulePtr && modulePtr->name)
			{
				if ((!strcmp(modulePtr->name, "hivepool"))
				  ||(!strcmp(modulePtr->name, "hivepool04")))
				{
					drawWater = true;
					break;
				}
				else
				{
					if (!strcmp(modulePtr->name, "shaftbot"))
					{
						drawEndWater = true;
					}
					if ((!_stricmp(modulePtr->name, "shaft01"))
					 ||(!_stricmp(modulePtr->name, "shaft02"))
					 ||(!_stricmp(modulePtr->name, "shaft03"))
					 ||(!_stricmp(modulePtr->name, "shaft04"))
					 ||(!_stricmp(modulePtr->name, "shaft05"))
					 ||(!_stricmp(modulePtr->name, "shaft06")))
					{
						HandleRainShaft(modulePtr, -11726,-107080,10);
						drawEndWater = true;
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
				
			CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);

			WaterXOrigin = x;
			WaterZOrigin = z;
			WaterUScale = 4.0f / (float)MeshXScale;
			WaterVScale = 4.0f / (float)MeshZScale;
		 	MeshXScale /= 4;
		 	MeshZScale /= 2;

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

			CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);

			WaterXOrigin = x;
			WaterZOrigin = z;
			WaterUScale = 4.0f / (float)(MeshXScale+1800-3782);
			WaterVScale = 4.0f / (float)MeshZScale;
		 	MeshXScale /= 4;
		 	MeshZScale /= 2;

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
	else if (!_stricmp(LevelName, "derelict"))
	{
		bool drawMirrorSurfaces = false;
		bool drawWater = false;

		while (numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			// if it's a module, which isn't inside another module
			if (modulePtr && modulePtr->name)
			{
			  	if ((!_stricmp(modulePtr->name, "start-en01"))
			  	  ||(!_stricmp(modulePtr->name, "start")))
				{
					drawMirrorSurfaces = true;
				}
				else if (!_stricmp(modulePtr->name, "water-01"))
				{
					drawWater = true;
					HandleRainShaft(modulePtr, 32000, 0, 16);
				}
			}
		}

		if (drawMirrorSurfaces)
		{
			// dirt on mirror in marines room
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

			CheckForObjectsInWater(x, x+MeshXScale, z, z+MeshZScale, y);

			WaterXOrigin = x;
			WaterZOrigin = z;
			WaterUScale = 4.0f / (float)MeshXScale;
			WaterVScale = 4.0f / (float)MeshZScale;
		 	MeshXScale /= 2;
		 	MeshZScale /= 2;

			currentWaterTexture = ChromeImageNumber;

		 	D3D_DrawWaterPatch(x, y, z);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z);
		 	D3D_DrawWaterPatch(x, y, z+MeshZScale);
		 	D3D_DrawWaterPatch(x+MeshXScale, y, z+MeshZScale);
		}
	}
	else if (!_stricmp(LevelName, "genshd1"))
	{
		bool drawWater = false;

		while (numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			// if it's a module, which isn't inside another module
			if (modulePtr && modulePtr->name)
			{
				if ((!_stricmp(modulePtr->name, "largespace"))
				  ||(!_stricmp(modulePtr->name, "proc13"))
				  ||(!_stricmp(modulePtr->name, "trench01"))
				  ||(!_stricmp(modulePtr->name, "trench02"))
				  ||(!_stricmp(modulePtr->name, "trench03"))
				  ||(!_stricmp(modulePtr->name, "trench04"))
				  ||(!_stricmp(modulePtr->name, "trench05"))
				  ||(!_stricmp(modulePtr->name, "trench06"))
				  ||(!_stricmp(modulePtr->name, "trench07"))
				  ||(!_stricmp(modulePtr->name, "trench08"))
				  ||(!_stricmp(modulePtr->name, "trench09")))
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
	if (!strcmp(LevelName, "genshd1"))
	{
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

	for (uint32_t x = 0; x < 16; x++)
	{
		for (uint32_t z = 0; z < 16; z++)
		{
			VECTORCH *point = &MeshVertex[i];

			point->vx = xOrigin + (x*MeshXScale)/15;
			point->vz = zOrigin + (z*MeshZScale)/15;

			int offset = 0;

 			offset += EffectOfRipples(point);

			point->vy = yOrigin + offset;

			int alpha = 128 - offset / 4;

			switch (CurrentVisionMode)
			{
				default:
				case VISION_MODE_NORMAL:
				{
					MeshVertexColour[i] = RGBA_MAKE(255, 255, 255, alpha);
					break;
				}
				case VISION_MODE_IMAGEINTENSIFIER:
				{
					MeshVertexColour[i] = RGBA_MAKE(0, 51 ,0, alpha);
					break;
				}
				case VISION_MODE_PRED_THERMAL:
				case VISION_MODE_PRED_SEEALIENS:
				case VISION_MODE_PRED_SEEPREDTECH:
				{
					MeshVertexColour[i] = RGBA_MAKE(0, 0, 28, alpha);
				  	break;
				}
			}

			MeshWorldVertex[i].vx = ((point->vx - WaterXOrigin)/4 + MUL_FIXED(GetSin((point->vy*16) & 4095), 128));
			MeshWorldVertex[i].vy = ((point->vz - WaterZOrigin)/4 + MUL_FIXED(GetSin((point->vy*16+200) & 4095), 128));

//bjd			TranslatePointIntoViewspace(point);

			i++;
		}
	}

	D3D_DrawMoltenMetalMesh_Unclipped();
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
		MakeParticle(&position, &velocity, PARTICLE_WATERFALLSPRAY);
	}
}

void D3D_DrawBackdrop(void)
{
	if (TRIPTASTIC_CHEATMODE || MOTIONBLUR_CHEATMODE)
		return;

	if (WireFrameMode)
	{
   		ColourFillBackBuffer(0);
		return;
	}
	else if (ShowDebuggingText.Tears)
	{
		ColourFillBackBuffer((63<<5));
		return;
	}

	bool needToDrawBackdrop = false;

	int numOfObjects = NumActiveBlocks;
	while (numOfObjects--)
	{
		DISPLAYBLOCK *objectPtr = ActiveBlockList[numOfObjects];
		MODULE *modulePtr = objectPtr->ObMyModule;

		if (modulePtr && (ModuleCurrVisArray[modulePtr->m_index] == 2) && modulePtr->m_flags & MODULEFLAG_SKY)
		{
			needToDrawBackdrop = true;
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

	// if the player is outside the environment, clear the screen!
	if (!playerPherModule)
	{
		ColourFillBackBuffer(0);
		return;
	}

	PLAYER_STATUS *playerStatusPtr = (PLAYER_STATUS *)(Player->ObStrategyBlock->SBdataptr);

	if (!playerStatusPtr->IsAlive || FREEFALL_CHEATMODE)
	{
		// minimise effects of camera glitches
		ColourFillBackBuffer(0);
		return;
	}
}

void DrawNoiseOverlay(int t)
{
	// t == 64 for image intensifier
	float u = (float)(FastRandom()&255);
	float v = (float)(FastRandom()&255);
	uint32_t c = 255;
	uint32_t size = 256;//*CameraZoomScale;

	orthoList->AddItem(4, StaticImageNumber, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, TEXTURE_WRAP);

	// bottom left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(c,c,c,t);
	orthoVertex[orthoVBOffset].u = u/256.0f;
	orthoVertex[orthoVBOffset].v = (v+size)/256.0f;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(c,c,c,t);
	orthoVertex[orthoVBOffset].u = u/256.0f;
	orthoVertex[orthoVBOffset].v = v/256.0f;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = 1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(c,c,c,t);
	orthoVertex[orthoVBOffset].u = (u+size)/256.0f;
	orthoVertex[orthoVBOffset].v = (v+size)/256.0f;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = 1.0f;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(c,c,c,t);
	orthoVertex[orthoVBOffset].u = (u+size)/256.0f;
	orthoVertex[orthoVBOffset].v = v/256.0f;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

void DrawScanlinesOverlay(float level)
{
	float u = 0.0f;//FastRandom()&255;
	float v = 128.0f;//FastRandom()&255;
	uint32_t c = 255;

	int t = static_cast<int>(4.0f+level*64.0f);

	float size = 128.0f*(1.0f-level*0.8f);//*CameraZoomScale;

	orthoList->AddItem(4, PredatorNumbersImageNumber, TRANSLUCENCY_NORMAL, FILTERING_BILINEAR_ON, TEXTURE_WRAP);

	// bottom left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(c,c,c,t);
	orthoVertex[orthoVBOffset].u = (v+size)/256.0f;
	orthoVertex[orthoVBOffset].v = 1.0f;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(c,c,c,t);
	orthoVertex[orthoVBOffset].u = (v-size)/256.0f;
	orthoVertex[orthoVBOffset].v = 1.0f;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = 1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(c,c,c,t);
	orthoVertex[orthoVBOffset].u = (v+size)/256.0f;
	orthoVertex[orthoVBOffset].v = 1.0f;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = 1.0f;;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = RGBA_MAKE(c,c,c,t);
	orthoVertex[orthoVBOffset].u = (v-size)/256.0f;
	orthoVertex[orthoVBOffset].v = 1.0f;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);

	if (level == 1.0f)
	{
		DrawNoiseOverlay(128);
	}
}

void D3D_SkyPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	texID_t textureID = (inputPolyPtr->PolyColour & ClrTxDefn);

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	float RecipW = 1.0f / (float) texWidth;
	float RecipH = 1.0f / (float) texHeight;

	mainList->AddItem(RenderPolygon.NumberOfVertices, textureID, RenderPolygon.TranslucencyMode, FILTERING_BILINEAR_ON, TEXTURE_WRAP, ZWRITE_DISABLED);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].x = (float)vertices->X;
		mainVertex[vb].y = (float)-vertices->Y;
		mainVertex[vb].z = (float)vertices->Z;

		mainVertex[vb].color = RGBA_MAKE(vertices->R,vertices->G,vertices->B,vertices->A);
		mainVertex[vb].specular = RGBA_MAKE(0,0,0,/*255*/0);

		mainVertex[vb].u = ((float)vertices->U) * RecipW;
		mainVertex[vb].v = ((float)vertices->V) * RecipH;
		vb++;
	}

	mainList->CreateIndices(mainIndex, RenderPolygon.NumberOfVertices);
}

// draws water etc
void D3D_DrawMoltenMetalMesh_Unclipped()
{
	VECTORCH *point = MeshVertex;
	VECTORCH *pointWS = MeshWorldVertex;

	mainList->AddItem(kNumMeshVerts, currentWaterTexture, TRANSLUCENCY_NORMAL);

	for (uint32_t i = 0; i < kNumMeshVerts; i++)
	{
		mainVertex[vb].x = (float)point->vx;
		mainVertex[vb].y = (float)-point->vy;
		mainVertex[vb].z = (float)point->vz;

		mainVertex[vb].color = MeshVertexColour[i];
		mainVertex[vb].specular = 0;

		mainVertex[vb].u = pointWS->vx*WaterUScale+(1.0f/256.0f);
		mainVertex[vb].v = pointWS->vy*WaterVScale+(1.0f/256.0f);

		vb++;
		point++;
		pointWS++;
	}

	for (uint32_t x = 0; x < 15; x++)
	{
		for (uint32_t y = 0; y < 15; y++)
		{
			mainList->AddTriangle(mainIndex, 0+x+(16*y),1+x+(16*y),16+x+(16*y), kNumMeshVerts);
			mainList->AddTriangle(mainIndex, 1+x+(16*y),17+x+(16*y),16+x+(16*y), kNumMeshVerts);
		}
	}
}

void ThisFramesRenderingHasBegun()
{
	if (R_BeginScene()) // calls a function to perform d3d_device->Begin();
	{
		LockExecuteBuffer(); // lock vertex buffer
	}
}

size_t lastMem = 0;

void ThisFramesRenderingHasFinished()
{
	Osk_Draw();

	Con_Draw();

	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	R_EndScene();

	Tex_CheckMemoryUsage();

#ifdef _XBOX
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
#endif

 	/* KJL 11:46:56 01/16/97 - kill off any lights which are fated to be removed */
	LightBlockDeallocation();
}

extern void D3D_DrawSliderBar(int x, int y, int alpha)
{
	VertexTag quadVertices[4];
	uint32_t sliderHeight = 11;
	uint32_t colour = alpha >> 8;

	if (colour > 255)
		colour = 255;

	colour = (colour << 24) + 0xffffff;

	quadVertices[0].Y = y;
	quadVertices[1].Y = y;
	quadVertices[2].Y = y + sliderHeight;
	quadVertices[3].Y = y + sliderHeight;

	{
		int topLeftU = 1;
		int topLeftV = 68;

		quadVertices[0].U = topLeftU;
		quadVertices[0].V = topLeftV;
		quadVertices[1].U = topLeftU + 2;
		quadVertices[1].V = topLeftV;
		quadVertices[2].U = topLeftU + 2;
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
	VertexTag quadVertices[4];
	uint32_t sliderHeight = 5;
	uint32_t colour = alpha >> 8;

	if (colour > 255)
	{
		colour = 255;
	}

	colour = (colour << 24) + 0xffffff;

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

#if 0 // fixme
//	CheckVertexBuffer(255, NO_TEXTURE, TRANSLUCENCY_OFF);

//	CheckOrthoBuffer(255, NO_TEXTURE, TRANSLUCENCY_OFF, TEXTURE_CLAMP, FILTERING_BILINEAR_ON);

	for (uint32_t i = 0; i < 255; )
	{
		/* this'll do.. */
		orthoList->AddItem(4, NO_TEXTURE, TRANSLUCENCY_OFF, FILTERING_BILINEAR_ON, TEXTURE_CLAMP);

		unsigned int colour = 0;
		unsigned int c = 0;

		c = GammaValues[i];

		// set alpha to 255 otherwise d3d alpha test stops pixels being rendered
		colour = RGBA_MAKE(MUL_FIXED(c,rScale),MUL_FIXED(c,gScale),MUL_FIXED(c,bScale),255);
/*
		// top left?
		mainVertex[vb].x = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
		mainVertex[vb].y = (float)yTop;
		mainVertex[vb].z = 0.0f;
//		mainVertex[vb].rhw = 1.0f;
		mainVertex[vb].color = colour;
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].u = 0.0f;
		mainVertex[vb].v = 0.0f;

		vb++;

		// bottom left?
		mainVertex[vb].x = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
		mainVertex[vb].y = (float)yBottom;
		mainVertex[vb].z = 0.0f;
//		mainVertex[vb].rhw = 1.0f;
		mainVertex[vb].color = colour;
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].u = 0.0f;
		mainVertex[vb].v = 0.0f;

		vb++;
*/
		i++;
		c = GammaValues[i];
		colour = RGBA_MAKE(MUL_FIXED(c,rScale),MUL_FIXED(c,gScale),MUL_FIXED(c,bScale),255);
/*
		// bottom
		mainVertex[vb].x = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
		mainVertex[vb].y = (float)yBottom;
		mainVertex[vb].z = 0.0f;
//		mainVertex[vb].rhw = 1.0f;
		mainVertex[vb].color = colour;
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].u = 0.0f;
		mainVertex[vb].v = 0.0f;

		vb++;

		mainVertex[vb].x = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
		mainVertex[vb].y = (float)yTop;
		mainVertex[vb].z = 0.0f;
//		mainVertex[vb].rhw = 1.0f;
		mainVertex[vb].color = colour;
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].u = 0.0f;
		mainVertex[vb].v = 0.0f;
*/
		vb++;

		OUTPUT_TRIANGLE(0,1,3, 4);
		OUTPUT_TRIANGLE(1,2,3, 4);
	}
#endif
}


extern void D3D_FadeDownScreen(int brightness, int colour)
{
	int t = 255 - (brightness>>8);
	if (t < 0)
	{
		t = 0;
	}

	orthoList->AddItem(4, NO_TEXTURE, TRANSLUCENCY_NORMAL, FILTERING_BILINEAR_ON, TEXTURE_WRAP);

	// bottom left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = (t<<24)+colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = (t<<24)+colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = 1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = (t<<24)+colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = 1.0f;;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = (t<<24)+colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

extern void D3D_PlayerOnFireOverlay(void)
{
	int t = 128;
	int colour = (FMVParticleColour&0xffffff)+(t<<24);

	float u = (FastRandom()&255)/256.0f;
	float v = (FastRandom()&255)/256.0f;

	orthoList->AddItem(4, BurningImageNumber, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, TEXTURE_WRAP);

	// bottom left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = u;
	orthoVertex[orthoVBOffset].v = v+1.0f;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = u;
	orthoVertex[orthoVBOffset].v = v;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = 1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = u+1.0f;
	orthoVertex[orthoVBOffset].v = v+1.0f;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = 1.0f;;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = u+1.0f;
	orthoVertex[orthoVBOffset].v = v;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

extern void D3D_ScreenInversionOverlay()
{
	// alien overlay
	int theta[2];
	int colour = 0xffffffff;

	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;

	orthoList->AddItem(4, SpecialFXImageNumber, TRANSLUCENCY_DARKENINGCOLOUR, FILTERING_BILINEAR_ON, TEXTURE_WRAP);

	for (uint32_t i = 0; i < 2; i++)
	{
		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;
		{
			// bottom left
			orthoVertex[orthoVBOffset].x = -1.0f;
			orthoVertex[orthoVBOffset].y = 1.0f;
			orthoVertex[orthoVBOffset].z = 1.0f;
			orthoVertex[orthoVBOffset].colour = colour;
			orthoVertex[orthoVBOffset].u = 0.375f + (cos*(-1) - sin*(+1));
			orthoVertex[orthoVBOffset].v = 0.375f + (sin*(-1) + cos*(+1));
			orthoVBOffset++;

			// top left
			orthoVertex[orthoVBOffset].x = -1.0f;
			orthoVertex[orthoVBOffset].y = -1.0f;
			orthoVertex[orthoVBOffset].z = 1.0f;
			orthoVertex[orthoVBOffset].colour = colour;
			orthoVertex[orthoVBOffset].u = 0.375f + (cos*(-1) - sin*(-1));
			orthoVertex[orthoVBOffset].v = 0.375f + (sin*(-1) + cos*(-1));
			orthoVBOffset++;

			// bottom right
			orthoVertex[orthoVBOffset].x = 1.0f;
			orthoVertex[orthoVBOffset].y = 1.0f;
			orthoVertex[orthoVBOffset].z = 1.0f;
			orthoVertex[orthoVBOffset].colour = colour;
			orthoVertex[orthoVBOffset].u = 0.375f + (cos*(+1) - sin*(+1));
			orthoVertex[orthoVBOffset].v = 0.375f + (sin*(+1) + cos*(+1));
			orthoVBOffset++;

			// top right
			orthoVertex[orthoVBOffset].x = 1.0f;;
			orthoVertex[orthoVBOffset].y = -1.0f;
			orthoVertex[orthoVBOffset].z = 1.0f;
			orthoVertex[orthoVBOffset].colour = colour;
			orthoVertex[orthoVBOffset].u = 0.375f + (cos*(+1) - sin*(-1));
			orthoVertex[orthoVBOffset].v = 0.375f + (sin*(+1) + cos*(-1));
			orthoVBOffset++;

			orthoList->CreateOrthoIndices(orthoIndex);
		}

		// only do this when finishing first loop, otherwise we reserve space for 4 verts we never add
		if (i == 0)
		{
			orthoList->AddItem(4, SpecialFXImageNumber, TRANSLUCENCY_COLOUR, FILTERING_BILINEAR_ON, TEXTURE_WRAP);
		}
	}
}

extern void D3D_PredatorScreenInversionOverlay()
{
	uint32_t colour = 0xffffffff;

	orthoList->AddItem(4, NO_TEXTURE, TRANSLUCENCY_DARKENINGCOLOUR, FILTERING_BILINEAR_ON, TEXTURE_WRAP);

	// bottom left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top left
	orthoVertex[orthoVBOffset].x = -1.0f;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// bottom right
	orthoVertex[orthoVBOffset].x = 1.0f;
	orthoVertex[orthoVBOffset].y = 1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	// top right
	orthoVertex[orthoVBOffset].x = 1.0f;
	orthoVertex[orthoVBOffset].y = -1.0f;
	orthoVertex[orthoVBOffset].z = 1.0f;
	orthoVertex[orthoVBOffset].colour = colour;
	orthoVertex[orthoVBOffset].u = 0.0f;
	orthoVertex[orthoVBOffset].v = 0.0f;
	orthoVBOffset++;

	orthoList->CreateOrthoIndices(orthoIndex);
}

extern void D3D_PlayerDamagedOverlay(int intensity)
{
	int theta[2];
	int colour, baseColour;

	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;

	switch (AvP.PlayerType)
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

	orthoList->AddItem(4, SpecialFXImageNumber, TRANSLUCENCY_INVCOLOUR, FILTERING_BILINEAR_ON, TEXTURE_WRAP);

	for (uint32_t i = 0; i < 2; i++)
	{
		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;
		{
			// bottom left
			orthoVertex[orthoVBOffset].x = -1.0f;
			orthoVertex[orthoVBOffset].y = 1.0f;
			orthoVertex[orthoVBOffset].z = 1.0f;
			orthoVertex[orthoVBOffset].colour = colour;
			orthoVertex[orthoVBOffset].u = (float)(0.875f + (cos*(-1) - sin*(+1)));
			orthoVertex[orthoVBOffset].v = (float)(0.375f + (sin*(-1) + cos*(+1)));
			orthoVBOffset++;

			// top left
			orthoVertex[orthoVBOffset].x = -1.0f;
			orthoVertex[orthoVBOffset].y = -1.0f;
			orthoVertex[orthoVBOffset].z = 1.0f;
			orthoVertex[orthoVBOffset].colour = colour;
			orthoVertex[orthoVBOffset].u = (float)(0.875f + (cos*(-1) - sin*(-1)));
			orthoVertex[orthoVBOffset].v = (float)(0.375f + (sin*(-1) + cos*(-1)));
			orthoVBOffset++;

			// bottom right
			orthoVertex[orthoVBOffset].x = 1.0f;
			orthoVertex[orthoVBOffset].y = 1.0f;
			orthoVertex[orthoVBOffset].z = 1.0f;
			orthoVertex[orthoVBOffset].colour = colour;
			orthoVertex[orthoVBOffset].u = (float)(0.875f + (cos*(+1) - sin*(+1)));
			orthoVertex[orthoVBOffset].v = (float)(0.375f + (sin*(+1) + cos*(+1)));
			orthoVBOffset++;

			// top right
			orthoVertex[orthoVBOffset].x = 1.0f;
			orthoVertex[orthoVBOffset].y = -1.0f;
			orthoVertex[orthoVBOffset].z = 1.0f;
			orthoVertex[orthoVBOffset].colour = colour;
			orthoVertex[orthoVBOffset].u = (float)(0.875f + (cos*(+1) - sin*(-1)));
			orthoVertex[orthoVBOffset].v = (float)(0.375f + (sin*(+1) + cos*(-1)));
			orthoVBOffset++;

			orthoList->CreateOrthoIndices(orthoIndex);
		}

		colour = baseColour +(intensity<<24);

		// only do this when finishing first loop, otherwise we reserve space for 4 verts we never add
		if (i == 0)
		{
			orthoList->AddItem(4, SpecialFXImageNumber, TRANSLUCENCY_GLOWING, FILTERING_BILINEAR_ON, TEXTURE_WRAP);
		}
	}
}

// D3D_DrawCable - draws predator grappling hook
void D3D_DrawCable(VECTORCH *centrePtr, MATRIXCH *orientationPtr)
{
	// TODO - not disabling zwrites. probably need to do this but double check (be handy if we didn't have to)

	currentWaterTexture = NO_TEXTURE;

	MeshXScale = 4096 / 16;
	MeshZScale = 4096 / 16;

	for (int field = 0; field < 3; field++)
	{
		int i = 0;
		for (int x = (0 + field*15); x < (16 + field*15); x++)
		{
			for (int z = 0; z < 16; z++)
			{
				VECTORCH *point = &MeshVertex[i];

				int innerRadius = 20;
				VECTORCH radius;
				int theta = ((4096*z) / 15) & 4095;
				int rOffset = GetSin((x*64 + theta/32 - CloakingPhase) & 4095);
				rOffset = MUL_FIXED(rOffset, rOffset) / 512;

				radius.vx = MUL_FIXED(innerRadius + rOffset/8, GetSin(theta));
				radius.vy = MUL_FIXED(innerRadius + rOffset/8, GetCos(theta));
				radius.vz = 0;

				RotateVector(&radius, orientationPtr);

				point->vx = centrePtr[x].vx + radius.vx;
				point->vy = centrePtr[x].vy + radius.vy;
				point->vz = centrePtr[x].vz + radius.vz;

				MeshVertexColour[i] = RGBA_MAKE(0, rOffset, 255, 128);

//bjd				TranslatePointIntoViewspace(point);

				i++;
			}
		}

		D3D_DrawMoltenMetalMesh_Unclipped();
	}
}

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

	if (y1 <= y0)
	{
		return;
	}

	D3D_Rectangle(x0, y0, x1, y1, R, G, B, translucency);
}

extern void D3D_RenderHUDNumber_Centred(uint32_t number, uint32_t x, uint32_t y, uint32_t colour)
{
	// green and red ammo numbers
	VertexTag quadVertices[4];
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

	VertexTag quadVertices[4];

	quadVertices[0].Y = y-1;
	quadVertices[1].Y = y-1;
	quadVertices[2].Y = y + HUD_FONT_HEIGHT + 1;
	quadVertices[3].Y = y + HUD_FONT_HEIGHT + 1;

	while (*stringPtr)
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

	VertexTag quadVertices[4];

	LOCALASSERT(y<=0);

	quadVertices[2].Y = y + HUD_FONT_HEIGHT + 1;
	quadVertices[3].Y = y + HUD_FONT_HEIGHT + 1;

	quadVertices[0].Y = 0;
	quadVertices[1].Y = 0;

	while (*stringPtr)
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

void D3D_RenderHUDString_Centred(char *stringPtr, uint32_t centreX, uint32_t y, uint32_t colour)
{
	// white text only of marine HUD ammo, health numbers etc
	if (stringPtr == NULL)
		return;

	int length = 0;
	char *ptr = stringPtr;

	while (*ptr)
	{
		length+=AAFontWidths[(uint8_t)*ptr++];
	}

	length = MUL_FIXED(HUDScaleFactor,length);

	int x = centreX-length/2;

	VertexTag quadVertices[4];

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
		x += MUL_FIXED(HUDScaleFactor,AAFontWidths[(uint8_t)c]);
	}
}

extern void RenderString(char *stringPtr, int x, int y, int colour)
{
	D3D_RenderHUDString(stringPtr, x, y, colour);
}

extern void RenderStringCentred(char *stringPtr, int centreX, int y, int colour)
{
	int length = 0;
	char *ptr = stringPtr;

	while (*ptr)
	{
		length += AAFontWidths[(uint8_t)*ptr++];
	}

	D3D_RenderHUDString(stringPtr,centreX-length/2,y,colour);
}

extern void RenderStringVertically(char *stringPtr, int centreX, int bottomY, int colour)
{
	VertexTag quadVertices[4];
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
