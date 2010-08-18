#include "console.h"
#include "onscreenKeyboard.h"
#include "textureManager.h"

#include "d3_func.h"

#include "r2base.h"
#include "font2.h"

// macros for Pix functions
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

// STL stuff
#include <vector>
#include <algorithm>

#include "logString.h"
#include "RenderList.h"
#include "vertexBuffer.h"

extern void DisableZBufferWrites();
extern void EnableZBufferWrites();

VertexBuffer *FMVvertexBuffer = 0;

#include <d3dx9math.h>
#include "fmvCutscenes.h"

#define UseLocalAssert FALSE
#include "ourasert.h"
#include <assert.h>
#include "avp_menus.h"
#include "avp_userprofile.h"
#include "avp_menugfx.hpp"

uint32_t NumVertices = 0;

// set to 'null' texture initially
uint32_t currentWaterTexture = NO_TEXTURE;

uint32_t	NumIndicies = 0;
uint32_t	vb = 0;
static uint32_t NumberOfRenderedTriangles = 0;

// vertex declarations
D3DVERTEXELEMENT9 declMain[] = {{0, 0,  D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0},
							{0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0},
							{0, 16, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		1},
                            {0, 20, D3DDECLTYPE_FLOAT2,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0},
                            D3DDECL_END()};

D3DVERTEXELEMENT9 declOrtho[] = {{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0},
								 {0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0},
								 {0, 16, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0},
                            D3DDECL_END()};

D3DVERTEXELEMENT9 declFMV[] = {{0, 0,  D3DDECLTYPE_FLOAT3,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0},
							   {0, 12, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0},
							   {0, 20, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	1},
							   {0, 28, D3DDECLTYPE_FLOAT2,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	2},
                            D3DDECL_END()};

D3DVERTEXELEMENT9 declTallFontText[] = {{0, 0, D3DDECLTYPE_FLOAT3,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,	0},
								{0, 12, D3DDECLTYPE_D3DCOLOR,	D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,		0},	
							    {0, 16, D3DDECLTYPE_FLOAT2,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	0},
							    {0, 24, D3DDECLTYPE_FLOAT2,		D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,	1},
                            D3DDECL_END()};

LPD3DXCONSTANTTABLE	vertexConstantTable = NULL;
LPD3DXCONSTANTTABLE	orthoConstantTable = NULL;
LPD3DXCONSTANTTABLE	fmvConstantTable = NULL;
LPD3DXCONSTANTTABLE	cloudConstantTable = NULL;

D3DXMATRIX viewMatrix;

extern D3DXMATRIX matOrtho;
extern D3DXMATRIX matOrtho2;
extern D3DXMATRIX matProjection;
extern D3DXMATRIX matIdentity;
extern D3DXMATRIX matViewPort;

// externs
extern D3DINFO d3d;
extern uint32_t fov;

extern int NumImagesArray[];

bool CreateVolatileResources();
bool ReleaseVolatileResources();
void ColourFillBackBuffer(int FillColour);

extern "C" {
extern SCREENDESCRIPTORBLOCK ScreenDescriptorBlock;
extern AVPIndexedFont IntroFont_Light;
extern VIEWDESCRIPTORBLOCK* Global_VDB_Ptr;
extern int CloakingPhase;
#include "particle.h"
#include "kshape.h"
int LightIntensityAtPoint(VECTORCH *pointPtr);
}

const uint32_t MAX_VERTEXES = 4096;
const uint32_t MAX_INDICES = 9216;

struct RENDER_STATES
{
	uint32_t	vertStart;
	uint32_t	vertEnd;
	uint32_t	indexStart;
	uint32_t	indexEnd;

	uint32_t	sortKey;

	bool operator<(const RENDER_STATES& rhs) const {return sortKey < rhs.sortKey;}
};

D3DLVERTEX *mainVertex = NULL;
WORD *mainIndex = NULL;

D3DLVERTEX *particleVertex = NULL;
WORD *particleIndex = NULL;
uint32_t	pVb = 0;
uint32_t	pIb = 0;
uint32_t	numPVertices = 0;

std::vector<RENDER_STATES> particleList;
std::vector<RENDER_STATES> renderList;
std::vector<RENDER_STATES> transRenderList;
std::vector<RENDER_STATES> orthoList;

uint32_t particleListCount = 0;
uint32_t renderListCount = 0;
uint32_t transRenderListCount = 0;
uint32_t orthoListCount = 0;

ORTHOVERTEX *orthoVerts = NULL;
WORD *orthoIndex = NULL;

static uint32_t orthoVBOffset = 0;
static uint32_t orthoIBOffset = 0;

bool UnlockExecuteBufferAndPrepareForUse();
bool ExecuteBuffer();
bool LockExecuteBuffer();
void ChangeTranslucencyMode(enum TRANSLUCENCY_TYPE translucencyRequired);
void ChangeTextureAddressMode(enum TEXTURE_ADDRESS_MODE textureAddressMode);
void ChangeFilteringMode(enum FILTERING_MODE_ID filteringRequired);

#define RGBLIGHT_MAKE(r,g,b) RGB_MAKE(r,g,b)
#define RGBALIGHT_MAKE(r,g,b,a) RGBA_MAKE(r,g,b,a)

static HRESULT LastError;

// initialise our std::vectors to a particle size, allowing us to use them as
// regular arrays
void RenderListInit()
{
	particleList.reserve(MAX_VERTEXES);
	particleList.resize(MAX_VERTEXES);
	memset(&particleList[0], 0, sizeof(RENDER_STATES));

	renderList.reserve(MAX_VERTEXES);
	renderList.resize(MAX_VERTEXES);
	memset(&renderList[0], 0, sizeof(RENDER_STATES));

	transRenderList.reserve(MAX_VERTEXES);
	transRenderList.resize(MAX_VERTEXES);
	memset(&transRenderList[0], 0, sizeof(RENDER_STATES));

	orthoList.reserve(MAX_VERTEXES);
	orthoList.resize(MAX_VERTEXES);
	memset(&orthoList[0], 0, sizeof(RENDER_STATES));
}

struct renderParticle
{
	PARTICLE		particle;
	RENDERVERTEX	vertices[9];
	uint32_t		numVerts;
	int				translucency;

	inline bool operator<(const renderParticle& rhs) const {return translucency < rhs.translucency;}
};

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
// we then use the OUTPUT_TRIANGLE function to generate the indicies required to represent those
// polygons. This function calculates the number of indicies required based on the number of verts.
// AvP code defined these, so hence the magic numbers.

uint32_t GetRealNumVerts(uint32_t numVerts)
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
			realNumVerts = 0;
			break;
		case 256:
			realNumVerts = 1350;
			break;
		default:
			// need to check this is ok!!
			realNumVerts = numVerts;
	}
	return realNumVerts;
}

// lock our vertex and index buffers, and reset counters and array indexes used to keep track 
// of the number of verts and indicies in those buffers. Function needs to be renamed as we no 
// longer use an execute buffer
bool LockExecuteBuffer()
{
	LastError = d3d.lpD3DVertexBuffer->Lock(0, 0, (void**)&mainVertex, D3DLOCK_DISCARD);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	LastError = d3d.lpD3DIndexBuffer->Lock(0, 0, (void**)&mainIndex, D3DLOCK_DISCARD);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// lock particle vertex buffer
	LastError = d3d.lpD3DParticleVertexBuffer->Lock(0, 0, (void**)&particleVertex, D3DLOCK_DISCARD);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// lock particle index buffer
	LastError = d3d.lpD3DParticleIndexBuffer->Lock(0, 0, (void**)&particleIndex, D3DLOCK_DISCARD);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// lock 2D ortho vertex buffer
	LastError = d3d.lpD3DOrthoVertexBuffer->Lock(0, 0, (void**)&orthoVerts, D3DLOCK_DISCARD);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// lock 2D ortho index buffer
	LastError = d3d.lpD3DOrthoIndexBuffer->Lock(0, 0, (void**)&orthoIndex, D3DLOCK_DISCARD);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// reset counters and indexes
	NumVertices = 0;
	NumIndicies = 0;
	vb = 0;

	orthoVBOffset = 0;
	orthoIBOffset = 0;
	orthoListCount = 0;

	renderListCount = 0;
	transRenderListCount = 0;

	pVb = 0;
	pIb = 0;
	particleListCount = 0;
	numPVertices = 0;

    return true;
}

// unlock all vertex and index buffers. function needs to be renamed as no longer using execute buffers
bool UnlockExecuteBufferAndPrepareForUse()
{
	// unlock main vertex buffer
	LastError = d3d.lpD3DVertexBuffer->Unlock();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// unlock index buffer for main vertex buffer
	LastError = d3d.lpD3DIndexBuffer->Unlock();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// unlock particle vertex buffer
	LastError = d3d.lpD3DParticleVertexBuffer->Unlock();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// unlock particle index buffer
	LastError = d3d.lpD3DParticleIndexBuffer->Unlock();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// unlock orthographic quad vertex buffer
	LastError = d3d.lpD3DOrthoVertexBuffer->Unlock();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	// unlock orthographic index buffer
	LastError = d3d.lpD3DOrthoIndexBuffer->Unlock();
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		return false;
	}

	return true;
}

bool ExecuteBuffer()
{
	// sort the list of render objects
	std::sort(renderList.begin(), renderList.begin() + renderListCount);
	std::sort(particleList.begin(), particleList.begin() + particleListCount);

	LastError = d3d.lpD3DDevice->SetStreamSource(0, d3d.lpD3DVertexBuffer, 0, sizeof(D3DLVERTEX));
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	LastError = d3d.lpD3DDevice->SetIndices(d3d.lpD3DIndexBuffer);
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
	}

	#ifndef USE_D3DVIEWTRANSFORM
		D3DXMatrixIdentity(&viewMatrix); // we want to use the identity matrix in this case
	#endif

	// we don't need world matrix here as avp has done all the world transforms itself
	D3DXMATRIXA16 matWorldViewProj = viewMatrix * matProjection;
	vertexConstantTable->SetMatrix(d3d.lpD3DDevice, "WorldViewProj", &matWorldViewProj);

	d3d.lpD3DDevice->SetVertexDeclaration(d3d.vertexDecl);
	d3d.lpD3DDevice->SetVertexShader(d3d.vertexShader);
	d3d.lpD3DDevice->SetPixelShader(d3d.pixelShader);

	ChangeTextureAddressMode(TEXTURE_WRAP);

	// draw opaque polygons
	for (uint32_t i = 0; i < renderListCount; i++)
	{
		// change render states if required
		R_SetTexture(0, renderList[i].sortKey >> 24);

		ChangeTranslucencyMode((enum TRANSLUCENCY_TYPE)	((renderList[i].sortKey >> 20) & 15));
		ChangeFilteringMode((enum FILTERING_MODE_ID)	((renderList[i].sortKey >> 16) & 15));

		uint32_t numPrimitives = (renderList[i].indexEnd - renderList[i].indexStart) / 3;

		if (numPrimitives > 0)
		{
			LastError = d3d.lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
				0,
				0,
				NumVertices,
				renderList[i].indexStart,
				numPrimitives);

			if (FAILED(LastError))
			{
				LogDxError(LastError, __LINE__, __FILE__);
			}
		}
		NumberOfRenderedTriangles += numPrimitives / 3;
	}

	// do transparents here..
	for (uint32_t i = 0; i < transRenderListCount; i++)
	{
		// change render states if required
		R_SetTexture(0, transRenderList[i].sortKey >> 24);

		ChangeTranslucencyMode((enum TRANSLUCENCY_TYPE)	((transRenderList[i].sortKey >> 20) & 15));
		ChangeFilteringMode((enum FILTERING_MODE_ID)	((transRenderList[i].sortKey >> 16) & 15));

		uint32_t numPrimitives = (transRenderList[i].indexEnd - transRenderList[i].indexStart) / 3;

		if (numPrimitives > 0)
		{
			LastError = d3d.lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
			   0,
			   0,
			   NumVertices,
			   transRenderList[i].indexStart,
			   numPrimitives);

			if (FAILED(LastError))
			{
				LogDxError(LastError, __LINE__, __FILE__);
			}
		}
		NumberOfRenderedTriangles += numPrimitives / 3;
	}

#if 1
	// render any particles
	if (particleListCount)
	{
		D3DPERF_BeginEvent(D3DCOLOR_XRGB(128,0,128), WIDEN("Starting to draw particles..."));

		LastError = d3d.lpD3DDevice->SetStreamSource(0, d3d.lpD3DParticleVertexBuffer, 0, sizeof(D3DLVERTEX));
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		LastError = d3d.lpD3DDevice->SetIndices(d3d.lpD3DParticleIndexBuffer);
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		vertexConstantTable->SetMatrix(d3d.lpD3DDevice, "WorldViewProj", &matProjection);

		DisableZBufferWrites();
		ChangeTextureAddressMode(TEXTURE_CLAMP);

		for (uint32_t i = 0; i < particleListCount; i++)
		{
			// change render states if required
			R_SetTexture(0, particleList[i].sortKey >> 24);

			ChangeTranslucencyMode((enum TRANSLUCENCY_TYPE)	((particleList[i].sortKey >> 20) & 15));
			ChangeFilteringMode((enum FILTERING_MODE_ID)	((particleList[i].sortKey >> 16) & 15));

			uint32_t numPrimitives = (particleList[i].indexEnd - particleList[i].indexStart) / 3;

			if (numPrimitives > 0)
			{
				LastError = d3d.lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
					0,
					0,
					numPVertices,
					particleList[i].indexStart,
					numPrimitives);

				if (FAILED(LastError))
				{
					LogDxError(LastError, __LINE__, __FILE__);
				}
			}
		}

		EnableZBufferWrites();
		ChangeTextureAddressMode(TEXTURE_WRAP);

		D3DPERF_EndEvent();
	}
#endif

	// render any orthographic quads
	if (orthoListCount)
	{
		LastError = d3d.lpD3DDevice->SetStreamSource(0, d3d.lpD3DOrthoVertexBuffer, 0, sizeof(ORTHOVERTEX));
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		LastError = d3d.lpD3DDevice->SetIndices(d3d.lpD3DOrthoIndexBuffer);
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		LastError = d3d.lpD3DDevice->SetVertexDeclaration(d3d.orthoVertexDecl);
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

		// pass the orthographicp projection matrix to the vertex shader
		orthoConstantTable->SetMatrix(d3d.lpD3DDevice, "WorldViewProj", &matOrtho);

		// loop through list drawing the quads
		for (uint32_t i = 0; i < orthoListCount; i++)
		{
			// change render states if required
			R_SetTexture(0, orthoList[i].sortKey >> 24);

			ChangeTranslucencyMode((enum TRANSLUCENCY_TYPE)	((orthoList[i].sortKey >> 20) & 15));
			ChangeFilteringMode((enum FILTERING_MODE_ID)	((orthoList[i].sortKey >> 16) & 15));
			ChangeTextureAddressMode((enum TEXTURE_ADDRESS_MODE) ((orthoList[i].sortKey >> 14) & 3));

			uint32_t primitiveCount = (orthoList[i].indexEnd - orthoList[i].indexStart) / 3;

			LastError = d3d.lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
			   0,
			   0,
			   orthoVBOffset,
			   orthoList[i].indexStart,
			   primitiveCount);

			if (FAILED(LastError))
			{
				LogDxError(LastError, __LINE__, __FILE__);
			}
		}
	}

	return true;
}

void CheckOrthoBuffer(uint32_t numVerts, uint32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum TEXTURE_ADDRESS_MODE textureAddressMode, enum FILTERING_MODE_ID filteringMode = FILTERING_BILINEAR_ON)
{
	assert (numVerts == 4);

	// check if we've got enough room. if not, flush
		// TODO

	assert(orthoIBOffset <= MAX_VERTEXES - 12);

	orthoList[orthoListCount].sortKey = 0; // zero it out

	// store our render state values in sortKey which we can use as a sorting value
	orthoList[orthoListCount].sortKey = (textureID << 24) | (translucencyMode << 20) | (filteringMode << 16) | (textureAddressMode << 14);

	// create a new list item for it
	orthoList[orthoListCount].vertStart = 0;
	orthoList[orthoListCount].vertEnd = 0;

	if (orthoListCount != 0 &&
		// if sort keys match, render states match so we can merge the two list items
		orthoList[orthoListCount-1].sortKey == orthoList[orthoListCount].sortKey)
	{
		orthoListCount--;
	}
	else
	{
		orthoList[orthoListCount].vertStart = orthoVBOffset;
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

void NewQuad(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t colour, uint32_t textureID, enum TRANSLUCENCY_TYPE translucencyType, float *UVList = NULL)
{
	float x1 = WPos2DC(x);
	float y1 = HPos2DC(y);

	float x2 = WPos2DC(x + width);
	float y2 = HPos2DC(y + height);

	uint32_t texturePOW2Width = 0;
	uint32_t texturePOW2Height = 0;

	if (textureID != NO_TEXTURE)
	{
		Tex_GetDimensions(textureID, texturePOW2Width, texturePOW2Height);
		CheckOrthoBuffer(4, textureID, translucencyType, TEXTURE_CLAMP);
	}
	else
	{
		texturePOW2Width = 1;
		texturePOW2Height = 1;
		CheckOrthoBuffer(4, NO_TEXTURE, translucencyType, TEXTURE_CLAMP);
	}
	
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

void DrawFmvFrame(uint32_t frameWidth, uint32_t frameHeight, uint32_t textureWidth, uint32_t textureHeight, r_Texture fmvTexture)
{
	uint32_t topX = (640 - frameWidth) / 2;
	uint32_t topY = (480 - frameHeight) / 2;

	float x1 = WPos2DC(topX);
	float y1 = HPos2DC(topY);

	float x2 = WPos2DC(topX + frameWidth);
	float y2 = HPos2DC(topY + frameHeight);

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
	LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &fmvVerts[0], sizeof(ORTHOVERTEX));
	if (FAILED(LastError))
	{
		OutputDebugString("DrawPrimitiveUP failed\n");
	}
}

void DrawFmvFrame2(uint32_t frameWidth, uint32_t frameHeight, uint32_t *textures, uint32_t numTextures)
{
	// offset the video vertically in the centre of the screen
	uint32_t topX = (640 - frameWidth) / 2;
	uint32_t topY = (480 - frameHeight) / 2;

	float x1 = WPos2DC(topX);
	float y1 = HPos2DC(topY);

	float x2 = WPos2DC(topX + frameWidth);
	float y2 = HPos2DC(topY + frameHeight);

	FMVVERTEX fmvVerts[4];

	// bottom left
	fmvVerts[0].x = x1;
	fmvVerts[0].y = y2;
	fmvVerts[0].z = 1.0f;
	fmvVerts[0].u1 = 0.0f;
	fmvVerts[0].v1 = 1.0f; //(1.0f / textureHeight) * frameHeight;

	fmvVerts[0].u2 = 0.0f;
	fmvVerts[0].v2 = 1.0f;

	fmvVerts[0].u3 = 0.0f;
	fmvVerts[0].v3 = 1.0f;

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
	fmvVerts[2].u1 = 1.0f; //(1.0f / textureWidth) * frameWidth;
	fmvVerts[2].v1 = 1.0f; //(1.0f / textureHeight) * frameHeight;

	fmvVerts[2].u2 = 1.0f;
	fmvVerts[2].v2 = 1.0f;

	fmvVerts[2].u3 = 1.0f;
	fmvVerts[2].v3 = 1.0f;

	// top right
	fmvVerts[3].x = x2;
	fmvVerts[3].y = y1;
	fmvVerts[3].z = 1.0f;
	fmvVerts[3].u1 = 1.0f; //(1.0f / textureWidth) * frameWidth;
	fmvVerts[3].v1 = 0.0f;

	fmvVerts[3].u2 = 1.0f;
	fmvVerts[3].v2 = 0.0f;

	fmvVerts[3].u3 = 1.0f;
	fmvVerts[3].v3 = 0.0f;

#if 0
	FMVVERTEX *fmvVerts;

	if (!FMVvertexBuffer)
	{
		FMVvertexBuffer = new VertexBuffer;
		FMVvertexBuffer->Create(4, FMVvertexBuffer->FVF_FMV, /*FMVvertexBuffer->USAGE_STATIC*/USAGE_STATIC);
		FMVvertexBuffer->Lock((void**)&fmvVerts);

		// bottom left
		fmvVerts[0].x = x1;
		fmvVerts[0].y = y2;
		fmvVerts[0].z = 1.0f;
		fmvVerts[0].u1 = 0.0f;
		fmvVerts[0].v1 = 1.0f; //(1.0f / textureHeight) * frameHeight;

		fmvVerts[0].u2 = 0.0f;
		fmvVerts[0].v2 = 1.0f;

		fmvVerts[0].u3 = 0.0f;
		fmvVerts[0].v3 = 1.0f;

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
		fmvVerts[2].u1 = 1.0f; //(1.0f / textureWidth) * frameWidth;
		fmvVerts[2].v1 = 1.0f; //(1.0f / textureHeight) * frameHeight;

		fmvVerts[2].u2 = 1.0f;
		fmvVerts[2].v2 = 1.0f;

		fmvVerts[2].u3 = 1.0f;
		fmvVerts[2].v3 = 1.0f;

		// top right
		fmvVerts[3].x = x2;
		fmvVerts[3].y = y1;
		fmvVerts[3].z = 1.0f;
		fmvVerts[3].u1 = 1.0f; //(1.0f / textureWidth) * frameWidth;
		fmvVerts[3].v1 = 0.0f;

		fmvVerts[3].u2 = 1.0f;
		fmvVerts[3].v2 = 0.0f;

		fmvVerts[3].u3 = 1.0f;
		fmvVerts[3].v3 = 0.0f;
	
		FMVvertexBuffer->Unlock();
	}
#endif
	// set orthographic projection
	fmvConstantTable->SetMatrix(d3d.lpD3DDevice, "WorldViewProj", &matOrtho);

	ChangeTextureAddressMode(TEXTURE_CLAMP);
	ChangeTranslucencyMode(TRANSLUCENCY_OFF);
	ChangeFilteringMode(FILTERING_BILINEAR_OFF);

	// set the texture
	for (uint32_t i = 0; i < numTextures; i++)
	{
		R_SetTexture(i, textures[i]);
	}

	d3d.lpD3DDevice->SetVertexDeclaration(d3d.fmvVertexDecl);
	d3d.lpD3DDevice->SetVertexShader(d3d.fmvVertexShader);
	d3d.lpD3DDevice->SetPixelShader(d3d.fmvPixelShader);

//	FMVvertexBuffer->Draw();

	LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &fmvVerts[0], sizeof(FMVVERTEX));
	if (FAILED(LastError))
	{
		LogDxError(LastError, __LINE__, __FILE__);
		OutputDebugString("DrawPrimitiveUP failed\n");
	}
}

void DrawProgressBar(const RECT &srcRect, const RECT &destRect, uint32_t textureID, AVPTEXTURE *tex, uint32_t newWidth, uint32_t newHeight)
{
	CheckOrthoBuffer(4, textureID, TRANSLUCENCY_OFF, TEXTURE_CLAMP, FILTERING_BILINEAR_ON);

	float x1 = WPos2DC(destRect.left);
	float y1 = HPos2DC(destRect.top);

	float x2 = WPos2DC(destRect.right);
	float y2 = HPos2DC(destRect.bottom);

	if (!tex)
	{
		return;
	}

	uint32_t width = tex->width;
	uint32_t height = tex->height;

	// new, power of 2 values
	uint32_t realWidth = newWidth;
	uint32_t realHeight = newHeight;

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

void DrawTallFontCharacter(uint32_t topX, uint32_t topY, uint32_t textureID, uint32_t texU, uint32_t texV, uint32_t charWidth, uint32_t alpha)
{
	alpha = (alpha / 256);
	if (alpha > 255)
		alpha = 255;

	D3DCOLOR colour = D3DCOLOR_ARGB(alpha, 255, 255, 255);

	uint32_t realWidth = 512;
	uint32_t realHeight = 512;

	float RecipW = 1.0f / realWidth;
	float RecipH = 1.0f / realHeight;

	uint32_t charHeight = 33;

	float x1 = WPos2DC(topX);
	float y1 = HPos2DC(topY);

	float x2 = WPos2DC(topX + charWidth);
	float y2 = HPos2DC(topY + charHeight);

#if 1

	R_SetTexture(0, textureID);
	R_SetTexture(1, AVPMENUGFX_CLOUDY);

	d3d.lpD3DDevice->SetVertexDeclaration(d3d.cloudVertexDecl);
	d3d.lpD3DDevice->SetVertexShader(d3d.cloudVertexShader);
	d3d.lpD3DDevice->SetPixelShader(d3d.cloudPixelShader);

	cloudConstantTable->SetMatrix(d3d.lpD3DDevice, "WorldViewProj", &matOrtho);
	LastError = cloudConstantTable->SetInt(d3d.lpD3DDevice, "CloakingPhase", CloakingPhase);
	LastError = cloudConstantTable->SetInt(d3d.lpD3DDevice, "pX", topX);
	
	if (FAILED(LastError))
	{
		OutputDebugString("SetInt failed\n");
	}

	ChangeFilteringMode(FILTERING_BILINEAR_ON);

	ORTHOVERTEX testOrtho[4];
	int16_t indices[6];

	indices[0] = 1;
	indices[1] = 2;
	indices[2] = 3;

	indices[3] = 2;
	indices[4] = 4;
	indices[5] = 3;

	// bottom left
	testOrtho[0].x = x1;
	testOrtho[0].y = y2;
	testOrtho[0].z = 1.0f;
	testOrtho[0].colour = colour;
	testOrtho[0].u = (float)((texU) * RecipW);
	testOrtho[0].v = (float)((texV + charHeight) * RecipH);

	// top left
	testOrtho[1].x = x1;
	testOrtho[1].y = y1;
	testOrtho[1].z = 1.0f;
	testOrtho[1].colour = colour;
	testOrtho[1].u = (float)((texU) * RecipW);
	testOrtho[1].v = (float)((texV) * RecipH);

	// bottom right
	testOrtho[2].x = x2;
	testOrtho[2].y = y2;
	testOrtho[2].z = 1.0f;
	testOrtho[2].colour = colour;
	testOrtho[2].u = (float)((texU + charWidth) * RecipW);
	testOrtho[2].v = (float)((texV + charHeight) * RecipH);

	// top right
	testOrtho[3].x = x2;
	testOrtho[3].y = y1;
	testOrtho[3].z = 1.0f;
	testOrtho[3].colour = colour;
	testOrtho[3].u = (float)((texU + charWidth) * RecipW);
	testOrtho[3].v = (float)((texV) * RecipH);

	LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, testOrtho, sizeof(ORTHOVERTEX));
	if (FAILED(LastError)) 
	{
		OutputDebugString(" draw menu quad failed ");
	}

#else // turned off for testing
	CheckOrthoBuffer(4, textureID, TRANSLUCENCY_GLOWING, TEXTURE_CLAMP);

	// bottom left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV + charHeight) * RecipH);
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
	orthoVerts[orthoVBOffset].u = (float)((texU + charWidth) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV + charHeight) * RecipH);
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = (float)((texU + charWidth) * RecipW);
	orthoVerts[orthoVBOffset].v = (float)((texV) * RecipH);
	orthoVBOffset++;

#endif
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
/*
			destPtr[0] = srcPtr[0];
			destPtr[1] = srcPtr[1];
			destPtr[2] = srcPtr[2];
			destPtr[3] = srcPtr[3];
*/
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

void DrawHUDQuad(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float *UVList, int32_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType)
{
	if (!UVList)
		return;

	assert (textureID != -1);

	float x1 = WPos2DC(x);
	float y1 = HPos2DC(y);

	float x2 = WPos2DC(x + width);
	float y2 = HPos2DC(y + height);

	uint32_t texturePOW2Width, texturePOW2Height;
	Tex_GetDimensions(textureID, texturePOW2Width, texturePOW2Height);

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

void DrawFontQuad(uint32_t x, uint32_t y, uint32_t charWidth, uint32_t charHeight, uint32_t textureID, float *uvArray, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType)
{
	float x1 = WPos2DC(x);
	float y1 = HPos2DC(y);

	float x2 = WPos2DC(x + charWidth);
	float y2 = HPos2DC(y + charHeight);

	CheckOrthoBuffer(4, textureID, translucencyType, TEXTURE_CLAMP, FILTERING_BILINEAR_OFF);

	// bottom left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = uvArray[0];
	orthoVerts[orthoVBOffset].v = uvArray[1];
	orthoVBOffset++;

	// top left
	orthoVerts[orthoVBOffset].x = x1;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = uvArray[2];
	orthoVerts[orthoVBOffset].v = uvArray[3];
	orthoVBOffset++;

	// bottom right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y2;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = uvArray[4];
	orthoVerts[orthoVBOffset].v = uvArray[5];
	orthoVBOffset++;

	// top right
	orthoVerts[orthoVBOffset].x = x2;
	orthoVerts[orthoVBOffset].y = y1;
	orthoVerts[orthoVBOffset].z = 1.0f;
	orthoVerts[orthoVBOffset].colour = colour;
	orthoVerts[orthoVBOffset].u = uvArray[6];
	orthoVerts[orthoVBOffset].v = uvArray[7];
	orthoVBOffset++;
}

void DrawQuad(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t textureID, uint32_t colour, enum TRANSLUCENCY_TYPE translucencyType)
{
	float x1 = WPos2DC(x);
	float y1 = HPos2DC(y);

	float x2 = WPos2DC(x + width);
	float y2 = HPos2DC(y + height);

	uint32_t texturePOW2Width = 0;
	uint32_t texturePOW2Height = 0;
	Tex_GetDimensions(textureID, texturePOW2Width, texturePOW2Height);

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

void DrawAlphaMenuQuad(uint32_t topX, uint32_t topY, uint32_t textureID, uint32_t alpha)
{
	// textures actual height/width (whether it's non power of two or not)
	uint32_t textureWidth, textureHeight;
	Tex_GetDimensions(textureID, textureWidth, textureHeight);

	alpha = (alpha / 256);
	if (alpha > 255)
		alpha = 255;

	DrawQuad(topX, topY, textureWidth, textureHeight, textureID, D3DCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);
}

void DrawMenuTextGlow(uint32_t topLeftX, uint32_t topLeftY, uint32_t size, uint32_t alpha)
{
	uint32_t textureWidth = 0;
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

	DrawQuad(topLeftX, topLeftY, textureWidth, textureHeight, AVPMENUGFX_GLOWY_LEFT, D3DCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);

	// now do the middle section
	topLeftX += textureWidth;

	Tex_GetDimensions(AVPMENUGFX_GLOWY_MIDDLE, textureWidth, textureHeight);

	DrawQuad(topLeftX, topLeftY, textureWidth * size, textureHeight, AVPMENUGFX_GLOWY_MIDDLE, D3DCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);

	// now do the right section
	topLeftX += textureWidth * size;

	Tex_GetDimensions(AVPMENUGFX_GLOWY_RIGHT, textureWidth, textureHeight);

	DrawQuad(topLeftX, topLeftY, textureWidth, textureHeight, AVPMENUGFX_GLOWY_RIGHT, D3DCOLOR_ARGB(alpha, 255, 255, 255), TRANSLUCENCY_GLOWING);
}

void DrawSmallMenuCharacter(uint32_t topX, uint32_t topY, uint32_t texU, uint32_t texV, uint32_t red, uint32_t green, uint32_t blue, uint32_t alpha)
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

	uint32_t font_height = 15;
	uint32_t font_width = 15;

	// aa_font.bmp is 256 x 256
	uint32_t image_height = 256;
	uint32_t image_width = 256;

	float RecipW = 1.0f / image_width; // 0.00390625
	float RecipH = 1.0f / image_height;

	float x1 = WPos2DC(topX);
	float y1 = HPos2DC(topY);

	float x2 = WPos2DC(topX + font_width);
	float y2 = HPos2DC(topY + font_height);

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

extern "C" {

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

extern int SpecialFXImageNumber;

extern "C"
{
	void TransformToViewspace(VECTORCHF *vector)
	{
		D3DXVECTOR3 output;
		D3DXVECTOR3 input(vector->vx, vector->vy, vector->vz);

		D3DXVec3TransformCoord(&output, &input, &viewMatrix);

		vector->vx = output.x;
		vector->vy = output.y;
		vector->vz = output.z;
	}
} // extern C

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
	viewMatrix = D3DXMATRIX(
			vecRight.x, vecRight.y, vecRight.z, 0.0f,
			vecUp.x, vecUp.y, vecUp.z, 0.0f,
			vecFront.x, vecFront.y, vecFront.z, 0.0f,
			vecPosition.x, vecPosition.y, vecPosition.z, 1.0f);
*/
}

void DrawCoronas()
{
	if (coronaArray.size() == 0)
		return;

	uint32_t numVertsBackup = RenderPolygon.NumberOfVertices;

	R_SetTexture(0, SpecialFXImageNumber);

	d3d.lpD3DDevice->SetVertexDeclaration(d3d.vertexDecl);
	d3d.lpD3DDevice->SetPixelShader(d3d.pixelShader);

	DisableZBufferWrites();

	D3DCOLOR colour;
	float RecipW, RecipH;

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(SpecialFXImageNumber, texWidth, texHeight);

	RecipW = 1.0f / (float) texWidth;
	RecipH = 1.0f / (float) texHeight;

	for (size_t i = 0; i < coronaArray.size(); i++)
	{
		PARTICLE_DESC *particleDescPtr = &ParticleDescription[coronaArray[i].particle.ParticleID];

		assert (coronaArray[i].numVerts == 4);

		if (particleDescPtr->IsLit && !(coronaArray[i].particle.ParticleID == PARTICLE_ALIEN_BLOOD && CurrentVisionMode == VISION_MODE_PRED_SEEALIENS))
		{
			int intensity = LightIntensityAtPoint(&coronaArray[i].particle.Position);
			if (coronaArray[i].particle.ParticleID == PARTICLE_SMOKECLOUD || coronaArray[i].particle.ParticleID == PARTICLE_ANDROID_BLOOD)
			{
				colour = RGBALIGHT_MAKE
				  		(
				   			MUL_FIXED(intensity, coronaArray[i].particle.ColourComponents.Red),
				   			MUL_FIXED(intensity, coronaArray[i].particle.ColourComponents.Green),
				   			MUL_FIXED(intensity, coronaArray[i].particle.ColourComponents.Blue),
				   			coronaArray[i].particle.ColourComponents.Alpha
				   		);
			}
			else
			{
				colour = RGBALIGHT_MAKE
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
			colour = RGBALIGHT_MAKE
						(
							FastRandom()&255,
							FastRandom()&255,
							FastRandom()&255,
							particleDescPtr->Alpha
						);
		}

		// transform our coronas world position into projection then viewport space
		D3DXVECTOR3 tempVec;
		D3DXVECTOR3 newVec;

		tempVec.x = coronaArray[i].coronaPoint.vx;
		tempVec.y = coronaArray[i].coronaPoint.vy;
		tempVec.z = coronaArray[i].coronaPoint.vz;

		// projection transform
		D3DXVec3TransformCoord(&newVec, &tempVec, &matProjection);

		// do viewport transform on this
		D3DXVec3TransformCoord(&tempVec, &newVec, &matViewPort);

		// generate the quad around this point
		ORTHOVERTEX ortho[4];
		uint32_t size = 100;

		// bottom left
		ortho[0].x = WPos2DC(tempVec.x - size);
		ortho[0].y = HPos2DC(tempVec.y + size);
		ortho[0].z = 1.0f;
		ortho[0].colour = colour;
		ortho[0].u = 192.0f * RecipW;
		ortho[0].v = 63.0f * RecipH;

		// top left
		ortho[1].x = WPos2DC(tempVec.x - size);
		ortho[1].y = HPos2DC(tempVec.y - size);
		ortho[1].z = 1.0f;
		ortho[1].colour = colour;
		ortho[1].u = 192.0f * RecipW;
		ortho[1].v = 0.0f * RecipH;

		// bottom right
		ortho[2].x = WPos2DC(tempVec.x + size);
		ortho[2].y = HPos2DC(tempVec.y + size);
		ortho[2].z = 1.0f;
		ortho[2].colour = colour;
		ortho[2].u = 255.0f * RecipW;
		ortho[2].v = 63.0f * RecipH;

		// top right
		ortho[3].x = WPos2DC(tempVec.x + size);
		ortho[3].y = HPos2DC(tempVec.y - size);
		ortho[3].z = 1.0f;
		ortho[3].colour = colour;
		ortho[3].u = 255.0f * RecipW;
		ortho[3].v = 0.0f * RecipH;

		d3d.lpD3DDevice->SetVertexShader(d3d.orthoVertexShader);
		orthoConstantTable->SetMatrix(d3d.lpD3DDevice, "WorldViewProj", &matOrtho);

		ChangeTranslucencyMode((enum TRANSLUCENCY_TYPE)particleDescPtr->TranslucencyType);

		LastError = d3d.lpD3DDevice->SetVertexDeclaration(d3d.orthoVertexDecl);
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
		}

		// draw the quad
		LastError = d3d.lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, ortho, sizeof(ORTHOVERTEX));
		if (FAILED(LastError))
		{
			LogDxError(LastError, __LINE__, __FILE__);
			OutputDebugString("DrawPrimitiveUP failed\n");
		}
	}

	EnableZBufferWrites();

	coronaArray.clear();

	// restore RenderPolygon.NumberOfVertices value...
	RenderPolygon.NumberOfVertices = numVertsBackup;
}

void DrawParticles()
{
	if (particleArray.size() == 0)
		return;

	uint32_t numVertsBackup = RenderPolygon.NumberOfVertices;

	// sort particle array
	std::sort(particleArray.begin(), particleArray.end());

	// loop particles and add them to vertex buffer
	for (size_t i = 0; i < particleArray.size(); i++)
	{
		RenderPolygon.NumberOfVertices = particleArray[i].numVerts;
		D3D_Particle_Output(&particleArray[i].particle, &particleArray[i].vertices[0]);
	}

	particleArray.clear();

	// restore RenderPolygon.NumberOfVertices value...
	RenderPolygon.NumberOfVertices = numVertsBackup;
}

#define FMV_ON 0
#define FMV_EVERYWHERE 0
#define FMV_SIZE 128

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

// Externs
extern int NormalFrameTime;
extern int HUDScaleFactor;
extern MODULE *playerPherModule;
extern int NumOnScreenBlocks;
extern DISPLAYBLOCK *OnScreenBlockList[];
extern char LevelName[];
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

extern void ScanImagesForFMVs();

int NumberOfLandscapePolygons;

/* OUTPUT_TRIANGLE - how this bugger works

	For example, if this takes in the parameters:
	( 0 , 2 , 3, 4)

	Image we have already got 12 vertexes counted in NumVertices
	What below does is construct a triangle in a certain order (ie as specified
	by the first 3 params passed into the function) using the data already in our vertex buffer
	so, for first parameter, set our first vertex to:

		v1 = 12 + 0 - 4
		v1 is therefore = 8;

		8 refers to the 8th vertice in our vertex buffer, ie 4 back from the end (as 12 - 4 = 8)

		then,
		v2 = 12 + 2 - 4
		v2 is therefore = 10;

		10 is 2 back, and 2 above the previous value. We've stepped over 9 :)

		IE.. 0, 2 order is forming. v1 is set to 11, so the '0,2,3' pattern emerged.

		that's a 2am explanation for ya :p

		edit: the above is probably really wrong. figure it out yourself :D
*/

static inline void OUTPUT_TRIANGLE(int a, int b, int c, int n)
{
	mainIndex[NumIndicies]   = (NumVertices - (n) + (a));
	mainIndex[NumIndicies+1] = (NumVertices - (n) + (b));
	mainIndex[NumIndicies+2] = (NumVertices - (n) + (c));
	NumIndicies+=3;
}

static inline void OUTPUT_TRIANGLE2(int a, int b, int c, int n)
{
	assert(pIb <= (9216 * 3) - 3);
	particleIndex[pIb]   = (numPVertices - (n) + (a));
	particleIndex[pIb+1] = (numPVertices - (n) + (b));
	particleIndex[pIb+2] = (numPVertices - (n) + (c));
	pIb+=3;
}

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

void CheckVertexBuffer(uint32_t numVerts, uint32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum FILTERING_MODE_ID filteringMode = FILTERING_BILINEAR_ON)
{
	uint32_t realNumVerts = GetRealNumVerts(numVerts);

	// check if we've got enough room. if not, flush
	if (NumVertices + numVerts > (MAX_VERTEXES-12))
	{
		UnlockExecuteBufferAndPrepareForUse();
		ExecuteBuffer();
		LockExecuteBuffer();
	}

	if (translucencyMode == TRANSLUCENCY_OFF)
	{
		renderList[renderListCount].sortKey = 0; // zero it out

		// store our render state values in sortKey which we can use as a sorting value
		renderList[renderListCount].sortKey = (textureID << 24) | (translucencyMode << 20) | (filteringMode << 16);

		renderList[renderListCount].vertStart = 0;
		renderList[renderListCount].vertEnd = 0;

		renderList[renderListCount].indexStart = 0;
		renderList[renderListCount].indexEnd = 0;

		if (renderListCount != 0 &&
			// if the two keys match, render states match so we can merge the list items
			renderList[renderListCount-1].sortKey == renderList[renderListCount].sortKey)
		{
			renderListCount--;
		}
		else
		{
			renderList[renderListCount].vertStart = NumVertices;
			renderList[renderListCount].indexStart = NumIndicies;
		}

		renderList[renderListCount].vertEnd = NumVertices + numVerts;
		renderList[renderListCount].indexEnd = NumIndicies + realNumVerts;
		renderListCount++;
	}
	else
	{
		transRenderList[transRenderListCount].sortKey = 0; // zero it out
		transRenderList[transRenderListCount].sortKey = (textureID << 24) | (translucencyMode << 20) | (filteringMode << 16);

		transRenderList[transRenderListCount].vertStart = 0;
		transRenderList[transRenderListCount].vertEnd = 0;

		transRenderList[transRenderListCount].indexStart = 0;
		transRenderList[transRenderListCount].indexEnd = 0;

		if (transRenderListCount != 0 &&
			transRenderList[transRenderListCount-1].sortKey == transRenderList[transRenderListCount].sortKey)
		{
			transRenderListCount--;
		}
		else
		{
			transRenderList[transRenderListCount].vertStart = NumVertices;
			transRenderList[transRenderListCount].indexStart = NumIndicies;
		}

		transRenderList[transRenderListCount].vertEnd = NumVertices + numVerts;
		transRenderList[transRenderListCount].indexEnd = NumIndicies + realNumVerts;
		transRenderListCount++;
	}

	NumVertices += numVerts;
}

#if 0 // bjd - commenting out
void SetFogDistance(int fogDistance)
{
	if (fogDistance > 10000)
		fogDistance = 10000;

	fogDistance += 5000;
	fogDistance = 2000;
	CurrentRenderStates.FogDistance = fogDistance;
//	textprint("fog distance %d\n",fogDistance);
}
#endif

void D3D_ZBufferedGouraudTexturedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	// bjd - This is the function responsible for drawing level geometry and the players weapon

	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	uint32_t textureID = (inputPolyPtr->PolyColour & ClrTxDefn);

	float RecipW, RecipH;

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	RecipW = 1.0f / (float) texWidth;
	RecipH = 1.0f / (float) texHeight;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, textureID, RenderPolygon.TranslucencyMode);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z;

		mainVertex[vb].color = RGBALIGHT_MAKE(GammaValues[vertices->R], GammaValues[vertices->G], GammaValues[vertices->B], vertices->A);
		mainVertex[vb].specular = RGBALIGHT_MAKE(GammaValues[vertices->SpecularR], GammaValues[vertices->SpecularG], GammaValues[vertices->SpecularB], 255);

		mainVertex[vb].tu = (float)(vertices->U) * RecipW;
		mainVertex[vb].tv = (float)(vertices->V) * RecipH;
		vb++;
	}

	#if FMV_EVERYWHERE
	TextureHandle = FMVTextureHandle[0];
	#endif

	D3D_OutputTriangles();
}

void D3D_ZBufferedGouraudPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	// responsible for drawing predators lock on triangle, for one

	// Take header information
	int32_t flags = inputPolyPtr->PolyFlags;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, NO_TEXTURE, RenderPolygon.TranslucencyMode);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z;

		if (flags & iflag_transparent)
		{
	  		mainVertex[vb].color = RGBALIGHT_MAKE(vertices->R, vertices->G, vertices->B, vertices->A);
		}
		else
		{
			mainVertex[vb].color = RGBLIGHT_MAKE(vertices->R, vertices->G, vertices->B);
		}

		mainVertex[vb].specular = (D3DCOLOR)1.0f;
		mainVertex[vb].tu = 0.0f;
		mainVertex[vb].tv = 0.0f;
		vb++;
	}

	D3D_OutputTriangles();
}

void D3D_PredatorThermalVisionPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	CheckVertexBuffer(RenderPolygon.NumberOfVertices, NO_TEXTURE, TRANSLUCENCY_OFF);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z;

		mainVertex[vb].color = RGBALIGHT_MAKE(vertices->R, vertices->G, vertices->B, vertices->A);
		mainVertex[vb].specular = (D3DCOLOR)1.0f;
		mainVertex[vb].tu = 0.0f;
		mainVertex[vb].tv = 0.0f;
		vb++;
	}

	D3D_OutputTriangles();
}

void D3D_ZBufferedCloakedPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	uint32_t textureID = (inputPolyPtr->PolyColour & ClrTxDefn);

	float RecipW, RecipH;

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	RecipW = 1.0f / (float) texWidth;
	RecipH = 1.0f / (float) texHeight;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, textureID, TRANSLUCENCY_NORMAL);

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

	new_x1 = WPos2DC(x0);
	new_y1 = HPos2DC(y0);

	new_x2 = WPos2DC(x1);
	new_y2 = HPos2DC(y1);

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
	/*
	if (D3DZFunc != D3DCMP_LESSEQUAL)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
		D3DZFunc = D3DCMP_LESSEQUAL;
	}
	*/ // bjd - fixme
}

void D3D_HUDQuad_Output(uint32_t textureID, struct VertexTag *quadVerticesPtr, uint32_t colour, enum FILTERING_MODE_ID filteringType)
{
	float RecipW, RecipH;

	assert (textureID != -1);

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	RecipW = 1.0f / texWidth;
	RecipH = 1.0f / texHeight;

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

void D3D_DecalSystem_Setup(void)
{
	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	LockExecuteBuffer();

	DisableZBufferWrites();
/*
	if (D3DZWriteEnable != FALSE)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		D3DZWriteEnable = FALSE;
	}
*/
//	D3DPERF_BeginEvent(D3DCOLOR_XRGB(128,0,128), WIDEN("Starting to draw Decals...\n"));
}

void D3D_DecalSystem_End(void)
{
	DrawParticles();
	DrawCoronas();

	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	LockExecuteBuffer();

	EnableZBufferWrites();

/*
	if (D3DZWriteEnable != TRUE)
	{
		d3d.lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
		D3DZWriteEnable = TRUE;
	}
*/
//	D3DPERF_EndEvent();
}

void D3D_Decal_Output(DECAL *decalPtr, RENDERVERTEX *renderVerticesPtr)
{
	// function responsible for bullet marks on walls, etc
	DECAL_DESC *decalDescPtr = &DecalDescription[decalPtr->DecalID];

	uint32_t textureID = SpecialFXImageNumber;

//	AVPTEXTURE *textureHandle = NULL;

	float RecipW, RecipH;
	D3DCOLOR colour;
	D3DCOLOR specular = RGBALIGHT_MAKE(0, 0, 0, 0);

	if (decalPtr->DecalID == DECAL_FMV)
	{
		#if !FMV_ON
			return;
		#endif

//		textureHandle = FMVTextureHandle[decalPtr->Centre.vx];

		RecipW = 1.0f / 128.0f;
		RecipH = 1.0f / 128.0f;

		textureID = NO_TEXTURE;
	}

	else if (decalPtr->DecalID == DECAL_SHAFTOFLIGHT || decalPtr->DecalID == DECAL_SHAFTOFLIGHT_OUTER)
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
		mainVertex[vb].sz = (float)vertices->Z - 50;

		mainVertex[vb].color = colour;
		mainVertex[vb].specular = specular;

		mainVertex[vb].tu = (float)(vertices->U) * RecipW;
		mainVertex[vb].tv = (float)(vertices->V) * RecipH;
		vb++;
	}

	D3D_OutputTriangles();
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
	renderParticle newParticle;

	newParticle.numVerts = RenderPolygon.NumberOfVertices;
	newParticle.particle = *particlePtr;

	memcpy(&newParticle.vertices[0], renderVerticesPtr, newParticle.numVerts * sizeof(RENDERVERTEX));
	newParticle.translucency = ParticleDescription[particlePtr->ParticleID].TranslucencyType;

	particleArray.push_back(newParticle);
}

void D3D_Particle_Output(PARTICLE *particlePtr, RENDERVERTEX *renderVerticesPtr)
{
	// steam jets, wall lights, fire (inc aliens on fire) etc
	PARTICLE_DESC *particleDescPtr = &ParticleDescription[particlePtr->ParticleID];

	uint32_t textureID = SpecialFXImageNumber;

	float RecipW, RecipH;

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	RecipW = 1.0f / (float) texWidth;
	RecipH = 1.0f / (float) texHeight;

//	CheckVertexBuffer(RenderPolygon.NumberOfVertices, SpecialFXImageNumber, (enum TRANSLUCENCY_TYPE)particleDescPtr->TranslucencyType);
//	void CheckVertexBuffer(uint32_t numVerts, int32_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum FILTERING_MODE_ID filteringMode = FILTERING_BILINEAR_ON)

	uint32_t realNumVerts = GetRealNumVerts(RenderPolygon.NumberOfVertices);

	assert(textureID >= 0 && textureID <= 255);

	particleList[particleListCount].sortKey = 0; // zero it out
	particleList[particleListCount].sortKey = (textureID << 24) | (particleDescPtr->TranslucencyType << 20) | (FILTERING_BILINEAR_ON << 16);

	particleList[particleListCount].vertStart = 0;
	particleList[particleListCount].vertEnd = 0;

	particleList[particleListCount].indexStart = 0;
	particleList[particleListCount].indexEnd = 0;

	if (particleListCount != 0 &&
		particleList[particleListCount-1].sortKey == particleList[particleListCount].sortKey)
	{
		particleListCount--;
	}
	else
	{
		particleList[particleListCount].vertStart = numPVertices;
		particleList[particleListCount].indexStart = pIb;
	}

	particleList[particleListCount].vertEnd = numPVertices + RenderPolygon.NumberOfVertices;
	particleList[particleListCount].indexEnd = pIb + realNumVerts;

	particleListCount++;

	numPVertices += RenderPolygon.NumberOfVertices;

/*
	char buf[100];
	sprintf(buf, "trans type: %d\n", particleDescPtr->TranslucencyType);
	OutputDebugString(buf);
*/
	D3DCOLOR colour;

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

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

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
			zvalue = (float)vertices->Z;
		}

		assert(pVb <= MAX_VERTEXES - 12);

		particleVertex[pVb].sx = (float)vertices->X;
		particleVertex[pVb].sy = (float)vertices->Y;
		particleVertex[pVb].sz = (float)vertices->Z;

		particleVertex[pVb].color = colour;
		particleVertex[pVb].specular = RGBALIGHT_MAKE(0,0,0,255);

		particleVertex[pVb].tu = ((float)(vertices->U)/* + 0.5f*/) * RecipW;
		particleVertex[pVb].tv = ((float)(vertices->V)/* + 0.5f*/) * RecipH;
		pVb++;
	}

//	D3D_OutputTriangles();

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
			OUTPUT_TRIANGLE2(0,2,1, 3);
			break;
		}
		case 4:
		{
			OUTPUT_TRIANGLE2(0,1,2, 4);
			OUTPUT_TRIANGLE2(0,2,3, 4);
			break;
		}
		case 5:
		{
			OUTPUT_TRIANGLE2(0,1,4, 5);
		    OUTPUT_TRIANGLE2(1,3,4, 5);
		    OUTPUT_TRIANGLE2(1,2,3, 5);

			break;
		}
		case 6:
		{
			OUTPUT_TRIANGLE2(0,4,5, 6);
		    OUTPUT_TRIANGLE2(0,3,4, 6);
		    OUTPUT_TRIANGLE2(0,2,3, 6);
		    OUTPUT_TRIANGLE2(0,1,2, 6);

			break;
		}
		case 7:
		{
			OUTPUT_TRIANGLE2(0,5,6, 7);
		    OUTPUT_TRIANGLE2(0,4,5, 7);
		    OUTPUT_TRIANGLE2(0,3,4, 7);
		    OUTPUT_TRIANGLE2(0,2,3, 7);
		    OUTPUT_TRIANGLE2(0,1,2, 7);

			break;
		}
		case 8:
		{
			OUTPUT_TRIANGLE2(0,6,7, 8);
		    OUTPUT_TRIANGLE2(0,5,6, 8);
		    OUTPUT_TRIANGLE2(0,4,5, 8);
		    OUTPUT_TRIANGLE2(0,3,4, 8);
		    OUTPUT_TRIANGLE2(0,2,3, 8);
		    OUTPUT_TRIANGLE2(0,1,2, 8);

			break;
		}
	}
/*
	if (NumVertices > (MAX_VERTEXES-12))
	{
		UnlockExecuteBufferAndPrepareForUse();
		ExecuteBuffer();
		LockExecuteBuffer();
	}
*/
}

void PostLandscapeRendering()
{
	int numOfObjects = NumOnScreenBlocks;

 //bjd - commenting out 	CurrentRenderStates.FogIsOn = 1;

	if (!strcmp(LevelName,"fall") || !strcmp(LevelName,"fall_m"))
	{
		bool drawWaterFall = false;
		bool drawStream = false;

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
		bool drawWater = false;
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
			WaterUScale = 4.0f/(float)(MeshXScale+1800-3782);
			WaterVScale = 4.0f/(float)MeshZScale;
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
		bool drawWater = 0;

		while (numOfObjects)
		{
			DISPLAYBLOCK *objectPtr = OnScreenBlockList[--numOfObjects];
			MODULE *modulePtr = objectPtr->ObMyModule;

			/* if it's a module, which isn't inside another module */
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
	extern char LevelName[];
	if (!strcmp(LevelName, "genshd1"))
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

	for (uint32_t x = 0; x < 16; x++)
	{
		for (uint32_t z = 0; z < 16; z++)
		{
			VECTORCH *point = &MeshVertex[i];

			point->vx = xOrigin+(x*MeshXScale)/15;
			point->vz = zOrigin+(z*MeshZScale)/15;

			int offset = 0;

 			offset += EffectOfRipples(point);

			point->vy = yOrigin+offset;

			int alpha = 128-offset/4;

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

			MeshWorldVertex[i].vx = ((point->vx-WaterXOrigin)/4+MUL_FIXED(GetSin((point->vy*16)&4095),128));
			MeshWorldVertex[i].vy = ((point->vz-WaterZOrigin)/4+MUL_FIXED(GetSin((point->vy*16+200)&4095),128));

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

// bjd - removed function void MakeNoiseTexture(void)

void DrawNoiseOverlay(int t)
{
	// t == 64 for image intensifier
	float u = (float)(FastRandom()&255);
	float v = (float)(FastRandom()&255);
	uint32_t c = 255;
	uint32_t size = 256;//*CameraZoomScale;

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
	uint32_t c = 255;
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

	if (level == 1.0f)
	{
		DrawNoiseOverlay(128);
	}
}

void D3D_SkyPolygon_Output(POLYHEADER *inputPolyPtr, RENDERVERTEX *renderVerticesPtr)
{
	// We assume bit 15 (TxLocal) HAS been
	// properly cleared this time...
	uint32_t textureID = (inputPolyPtr->PolyColour & ClrTxDefn);

	float RecipW, RecipH;

	uint32_t texWidth, texHeight;
	Tex_GetDimensions(textureID, texWidth, texHeight);

	RecipW = 1.0f / (float) texWidth;
	RecipH = 1.0f / (float) texHeight;

	CheckVertexBuffer(RenderPolygon.NumberOfVertices, textureID, RenderPolygon.TranslucencyMode);

	for (uint32_t i = 0; i < RenderPolygon.NumberOfVertices; i++)
	{
		RENDERVERTEX *vertices = &renderVerticesPtr[i];

		mainVertex[vb].sx = (float)vertices->X;
		mainVertex[vb].sy = (float)-vertices->Y;
		mainVertex[vb].sz = (float)vertices->Z;

		mainVertex[vb].color = RGBALIGHT_MAKE(vertices->R,vertices->G,vertices->B,vertices->A);
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);

		mainVertex[vb].tu = ((float)vertices->U) * RecipW;
		mainVertex[vb].tv = ((float)vertices->V) * RecipH;

		vb++;
	}

	#if FMV_EVERYWHERE
	TextureHandle = FMVTextureHandle[0];
	#endif

	D3D_OutputTriangles();
}

void D3D_DrawMoltenMetalMesh_Unclipped(void)
{
	VECTORCH *point = MeshVertex;
	VECTORCH *pointWS = MeshWorldVertex;

	// outputs 450 triangles with each run of the loop
	// 450 triangles has 3 * 450 vertices which = 1350
	CheckVertexBuffer(256, currentWaterTexture, TRANSLUCENCY_NORMAL);

	for (uint32_t i = 0; i < 256; i++)
	{
		if (point->vz <= 1)
			point->vz = 1;
/*
		int x = (point->vx*(Global_VDB_Ptr->VDB_ProjX+1))/point->vz+Global_VDB_Ptr->VDB_CentreX;
		int y = (point->vy*(Global_VDB_Ptr->VDB_ProjY+1))/point->vz+Global_VDB_Ptr->VDB_CentreY;
//			textprint("%d, %d\n",x,y);

		int z = point->vz + HeadUpDisplayZOffset;
		float rhw = 1.0f / (float)point->vz;
		float zf = 1.0f - Zoffset * ZNear/(float)z;

	  	float oneOverZ = ((float)(point->vz)-ZNear)/(float)(point->vz);

		mainVertex[vb].sx = (float)x;
		mainVertex[vb].sy = (float)y;
		mainVertex[vb].sz = zf;
		mainVertex[vb].rhw = rhw;
*/
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

	/* CONSTRUCT POLYS */
	for (uint32_t x = 0; x < 15; x++)
	{
		for (uint32_t y = 0; y < 15; y++)
		{
			OUTPUT_TRIANGLE(0+x+(16*y),1+x+(16*y),16+x+(16*y), 256);
			OUTPUT_TRIANGLE(1+x+(16*y),17+x+(16*y),16+x+(16*y), 256);
		}
	}
}

void ThisFramesRenderingHasBegun(void)
{
	if (R_BeginScene()) // calls a function to perform d3d_device->Begin();
	{
		LockExecuteBuffer(); // lock vertex buffer
	}
}

void ThisFramesRenderingHasFinished(void)
{
	Osk_Draw();

	Con_Draw();

	UnlockExecuteBufferAndPrepareForUse();
	ExecuteBuffer();
	R_EndScene();

 	/* KJL 11:46:56 01/16/97 - kill off any lights which are fated to be removed */
	LightBlockDeallocation();
}

extern void D3D_DrawSliderBar(int x, int y, int alpha)
{
	struct VertexTag quadVertices[4];
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
	struct VertexTag quadVertices[4];
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

	CheckOrthoBuffer(255, NO_TEXTURE, TRANSLUCENCY_OFF, TEXTURE_CLAMP, FILTERING_BILINEAR_ON);

	for (uint32_t i = 0; i < 255; )
	{
		/* this'll do.. */
//		CheckVertexBuffer(/*1530*/4, NO_TEXTURE, TRANSLUCENCY_OFF);

		unsigned int colour = 0;
		unsigned int c = 0;

		c = GammaValues[i];

		// set alpha to 255 otherwise d3d alpha test stops pixels being rendered
		colour = RGBA_MAKE(MUL_FIXED(c,rScale),MUL_FIXED(c,gScale),MUL_FIXED(c,bScale),255);
/*
		// top right?
		mainVertex[vb].sx = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
		mainVertex[vb].sy = (float)yTop;
		mainVertex[vb].sz = 0.0f;
//		mainVertex[vb].rhw = 1.0f;
		mainVertex[vb].color = colour;
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].tu = 0.0f;
		mainVertex[vb].tv = 0.0f;

		vb++;

		// bottom right?
		mainVertex[vb].sx = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
		mainVertex[vb].sy = (float)yBottom;
		mainVertex[vb].sz = 0.0f;
//		mainVertex[vb].rhw = 1.0f;
		mainVertex[vb].color = colour;
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].tu = 0.0f;
		mainVertex[vb].tv = 0.0f;

		vb++;
*/
		i++;
		c = GammaValues[i];
		colour = RGBA_MAKE(MUL_FIXED(c,rScale),MUL_FIXED(c,gScale),MUL_FIXED(c,bScale),255);
/*
		//
		mainVertex[vb].sx = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
		mainVertex[vb].sy = (float)yBottom;
		mainVertex[vb].sz = 0.0f;
//		mainVertex[vb].rhw = 1.0f;
		mainVertex[vb].color = colour;
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].tu = 0.0f;
		mainVertex[vb].tv = 0.0f;

		vb++;

		mainVertex[vb].sx = (float)(Global_VDB_Ptr->VDB_ClipRight*i)/255;
		mainVertex[vb].sy = (float)yTop;
		mainVertex[vb].sz = 0.0f;
//		mainVertex[vb].rhw = 1.0f;
		mainVertex[vb].color = colour;
		mainVertex[vb].specular = RGBALIGHT_MAKE(0,0,0,255);
		mainVertex[vb].tu = 0.0f;
		mainVertex[vb].tv = 0.0f;
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

	theta[0] = (CloakingPhase/8)&4095;
	theta[1] = (800-CloakingPhase/8)&4095;

	CheckOrthoBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_DARKENINGCOLOUR, TEXTURE_WRAP);

	for (uint32_t i = 0; i < 2; i++)
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

		// only do this when finishing first loop, otherwise we reserve space for 4 verts we never add
		if (i == 0)
		{
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

	CheckOrthoBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_INVCOLOUR, TEXTURE_WRAP);

	for (uint32_t i = 0; i < 2; i++)
	{
		float sin = (GetSin(theta[i]))/65536.0f/16.0f;
		float cos = (GetCos(theta[i]))/65536.0f/16.0f;
		{
			// bottom left
			orthoVerts[orthoVBOffset].x = -1.0f;
			orthoVerts[orthoVBOffset].y = 1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = (float)(0.875f + (cos*(-1) - sin*(+1)));
			orthoVerts[orthoVBOffset].v = (float)(0.375f + (sin*(-1) + cos*(+1)));
			orthoVBOffset++;

			// top left
			orthoVerts[orthoVBOffset].x = -1.0f;
			orthoVerts[orthoVBOffset].y = -1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = (float)(0.875f + (cos*(-1) - sin*(-1)));
			orthoVerts[orthoVBOffset].v = (float)(0.375f + (sin*(-1) + cos*(-1)));
			orthoVBOffset++;

			// bottom right
			orthoVerts[orthoVBOffset].x = 1.0f;
			orthoVerts[orthoVBOffset].y = 1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = (float)(0.875f + (cos*(+1) - sin*(+1)));
			orthoVerts[orthoVBOffset].v = (float)(0.375f + (sin*(+1) + cos*(+1)));
			orthoVBOffset++;

			// top right
			orthoVerts[orthoVBOffset].x = 1.0f;
			orthoVerts[orthoVBOffset].y = -1.0f;
			orthoVerts[orthoVBOffset].z = 1.0f;
			orthoVerts[orthoVBOffset].colour = colour;
			orthoVerts[orthoVBOffset].u = (float)(0.875f + (cos*(+1) - sin*(-1)));
			orthoVerts[orthoVBOffset].v = (float)(0.375f + (sin*(+1) + cos*(-1)));
			orthoVBOffset++;
		}

		colour = baseColour +(intensity<<24);

		// only do this when finishing first loop, otherwise we reserve space for 4 verts we never add
		if (i == 0)
		{
			CheckOrthoBuffer(4, SpecialFXImageNumber, TRANSLUCENCY_GLOWING, TEXTURE_WRAP);
		}
	}
}

// D3D_DrawCable - draws predator grappling hook
void D3D_DrawCable(VECTORCH *centrePtr, MATRIXCH *orientationPtr)
{
	// TODO - not disabling zwrites. probably need to do this but double check (be handy if we didn't have to)

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
			for (z=0; z<16; z++)
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

//bjd				TranslatePointIntoViewspace(point);

				i++;
			}
		}

		D3D_DrawMoltenMetalMesh_Unclipped();
	}
}

#if 0
static int GammaSetting;
void UpdateGammaSettings(int g, int forceUpdate)
{
	LPDIRECTDRAWGAMMACONTROL handle;
	DDGAMMARAMP gammaValues;

	if (g==GammaSetting && !forceUpdate) return;

	lpDDSPrimary->QueryInterface(IID_IDirectDrawGammaControl,(LPVOID*)&handle);

//	handle->GetGammaRamp(0,&gammaValues);
	for (int i=0; i<=255; i++)
	{
		int u = ((i*65536)/255);
		int m = MUL_FIXED(u,u);
		int l = MUL_FIXED(2*u,ONE_FIXED-u);

		int a;

		a = m/256+MUL_FIXED(g,l);
		if (a<0) a=0;
		if (a>255) a=255;

		gammaValues.red[i]=a*256;
		gammaValues.green[i]=a*256;
		gammaValues.blue[i]=a*256;
	}
//	handle->SetGammaRamp(0,&gammaValues);
	handle->SetGammaRamp(DDSGR_CALIBRATE,&gammaValues);
	GammaSetting=g;
//	handle->SetGammaRamp(DDSGR_CALIBRATE,&gammaValues);
	//handle->GetGammaRamp(0,&gammaValues);
	RELEASE(handle);
}
#endif



// For extern "C"

};

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

	if (y1<=y0)
		return;

	D3D_Rectangle(x0, y0, x1, y1, R, G, B, translucency);
}

extern void D3D_RenderHUDNumber_Centred(uint32_t number, uint32_t x, uint32_t y, uint32_t colour)
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

	struct VertexTag quadVertices[4];

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
		x += MUL_FIXED(HUDScaleFactor,AAFontWidths[(uint8_t)c]);
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

	while (*ptr)
	{
		length += AAFontWidths[(uint8_t)*ptr++];
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

};
