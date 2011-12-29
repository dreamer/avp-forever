#pragma once
#define D3D_DEBUG_INFO
#include <stdint.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <WinBase.h>
#include <algorithm>

/**
 * Main vertex declaration
 */
#pragma pack(4)
struct CD3D9RenderVertex
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
	D3DXVECTOR2 uvCoords;
	D3DCOLOR color;
};
#pragma pack()
/**
 * Structure containing all relevant DirectX states
 */
#pragma pack(1) // No padding
class CD3D9RenderItem
{
protected:
	unsigned shaderId  : 8;  // Current shader
	unsigned textureId : 16; // AvP texture's Id
	unsigned normalId  : 16; // AvP texture normal's Id
	unsigned cubeMapId : 8;  // AvP cube map's Id
	unsigned colWrite  : 4;  // D3DRS_COLORWRITEENABLE
	unsigned fillMode  : 3;  // D3DFILLMODE
	unsigned texAdrr   : 3;  // D3DTEXTUREADDRESS
	unsigned zEnable   : 1;  // D3DRS_ZENABLE
	unsigned zWrite    : 1;  // D3DRS_ZWRITEENABLE
	unsigned zCmpFunc  : 4;  // D3DRS_ALPHAFUNC - D3DCMPFUNC
	unsigned aEnable   : 1;  // D3DRS_ALPHATESTENABLE
	unsigned aFunc     : 4;  // D3DCMPFUNC
	unsigned aRefC     : 8;  // D3DRS_ALPHAREF Alpha reference constant
	unsigned aBlendEnb : 1;  // D3DRS_ALPHABLENDENABLE
	unsigned aBlendOp  : 3;  // D3DBLENDOP
	unsigned aSrcBlend : 5;  // D3DRS_SRCBLEND - D3DBLEND
	unsigned aDstBlend : 5;  // D3DRS_DESTBLEND - D3DBLEND
	unsigned unused    : 5;  // Unused bytes to pad to 128 bits
	// Index offset of our 1st primtive
	uint32_t startIndice;
	// How many vertices
	uint32_t nOfVertices;
	// Smallest index used
	uint32_t minIndex;
	// Model's matrix with world's orientation and position
	D3DXMATRIX * modelMatrix;
public:
	CD3D9RenderItem()
	{
		shaderId  = 0;
		textureId = 0;
		normalId  = 0;
		cubeMapId = 0;
		colWrite  = D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED;
		fillMode  = D3DFILL_SOLID;
		texAdrr   = D3DTADDRESS_WRAP;
		zEnable   = D3DZB_TRUE;
		zWrite    = TRUE;
		zCmpFunc  = D3DCMP_LESSEQUAL;
		aEnable   = FALSE;
		aFunc     = D3DCMP_ALWAYS;
		aRefC     = 0x00;
		aBlendEnb = FALSE;
		aBlendOp  = D3DBLENDOP_ADD;
		aSrcBlend = D3DBLEND_ONE;
		aDstBlend = D3DBLEND_ZERO;
		unused    = 0;
		// 1st triangle
		startIndice = 0;
		// number of vertices
		nOfVertices = 0;
		// Object's world matrix
		modelMatrix = NULL;
	}

	bool isEqual(CD3D9RenderItem * a)
	{
		return this->textureId == a->textureId;
	}
	/**
	 * Release dynamic data
	 */
	~CD3D9RenderItem()
	{
		if(modelMatrix)
		{
			//delete modelMatrix;
			modelMatrix = 0;
		}
	}
	void SetIndexRegion(uint32_t offset, uint32_t nVertices, uint32_t firstVBIndex)
	{
		startIndice = offset;
		nOfVertices = nVertices;
		minIndex = firstVBIndex;

	}
	void SetTextures(uint32_t texId,uint32_t normId = 0, uint32_t cubeId = 0)
	{
		textureId = texId  & 0x0000FFFF;
		normalId  = normId & 0x0000FFFF;
		cubeId    = cubeId & 0x000000FF;

	}
	/**
	 * Sets this render Item's Model Matrix
	 */
	void SetModelMatrix(D3DXMATRIX * modelMat)
	{
		modelMatrix = modelMat;
	}
	/**
	 * Return model matrix
	 */
	D3DXMATRIX * GetModelMatrix()
	{
		return modelMatrix;
	}
	/**
	 * Return texture's ID
	 */ 
	uint16_t GetTextureId()
	{
		return textureId;
	}
	/**
	 * Returns shader ID
	 */
	uint16_t GetShaderId()
	{
		return shaderId;
	}
	/**
	 * Returns index offset
	 */
	uint32_t GetIndexOffset()
	{
		return startIndice;
	}
	/**
	 * Returns index offset
	 */
	uint32_t GetVertexOffset()
	{
		return minIndex;
	}
	/**
	 * Return number of indices
	 */
	uint32_t GetNumberOfPrimitives()
	{
		return nOfVertices-2;
	}
	/**
	 * Return number of vertices
	 */
	uint32_t GetNumberOfVertices()
	{
		return nOfVertices;
	}


};
#pragma pack()
/**
 * Render list, hold the actual vertex buffers and draw routines
 * 
 */
class CD3D9RenderList
{
protected:
	/**
	 * Default values for render item
	 */
	const CD3D9RenderItem defaultRenderItem;
	/**
	 * List of dynamic rendering objects' vertices
	 */
	IDirect3DVertexBuffer9 * dynamicVertices;
	/**
	 * List of dynamic rendering objects' indices
	 */
	IDirect3DIndexBuffer9 * dynamicIndices;
	/**
	 * List of static rendering objects' vertices
	 */
	IDirect3DVertexBuffer9 * staticVertices;
	/**
	 * List of static rendering objects' indices
	 */
	IDirect3DIndexBuffer9 * staticIndices;
	/**
	 * Number of dynamic indexes added
	 */
	uint32_t dynamicIndicesCount;
	/**
	 * Number of static indexes added
	 */
	uint32_t staticIndicesCount;
	/**
	 * Number of dynamic vertices added
	 */
	uint32_t dynamicVerticesCount;
	/**
	 * Number of static vertices added
	 */
	uint32_t staticVerticesCount;

	/**
	 * List of static render items
	 */
	std::vector<CD3D9RenderItem *> staticRenderItems;
	/**
	 * List of dynamic render items
	 */
	std::vector<CD3D9RenderItem *> dynamicRenderItems;
	/**
	 * Direct3D9's device
	 */
	IDirect3DDevice9 * lpD3DDevice;
	/**
	 * Copied from BJD code, creates indexes for AvP's polygons
	 */
	void AddDynamicIndices(uint16_t *indexArray, uint32_t a, uint32_t b, uint32_t c, uint32_t n, uint32_t &localOffset);
public:
	CD3D9RenderList(void);
	~CD3D9RenderList(void);
	/**
	 * Initialises the render list's vertex buffers
	 */
	void InitialiseRenderList(IDirect3DDevice9 * device);
	/**
	 * Adds vertices to the dynamic render list and calculates the offsets for the corresponding
	 * render item.
	 */
	void AddDynamicVertexData(CD3D9RenderItem * renderItem , void * vertexData, size_t vertexSize, unsigned nOfVertices);
	/**
	 * Adds a render item to the render list
	 */
	void AddRenderItem(CD3D9RenderItem * renderItem);
	/**
	 * Returns the next render item
	 */
	CD3D9RenderItem * GetNextDynamicRenderItem();

	void DrawRenderItem(CD3D9RenderItem * renderItem);
	
	uint32_t NumberOfDynamicIndices();

	void PrepareForNextFrame();
	/**
	 * Sorts the render items and optimizes the render calls
	 * by making the RenderItems sequential
	 */ 
	void SortAndOptimizeRenderItems();

	void Destroy();

};

