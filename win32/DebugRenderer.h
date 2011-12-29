#pragma once
#include <d3dx9.h>
#include <vector>

#define MAX_DEBUG_VERTICES 10000

class DebugVertex
{
public:
	D3DXVECTOR3 position;
	D3DCOLOR diffuse;
};
/**
 * Mel - 13/12/2011
 * This is a simple class used to draw some debug info 
 * in the renderer, we have a few debug meshes and vertex
 * buffers to draw debug stuff like vectors and placed objects
 */
class CDebugRenderer
{
private:
	/**ssss
	 * Used to hold our vectors
	 */
	IDirect3DVertexBuffer9 * vectorsBuffer;
	/**
	 * Debug vertices
	 */
	DebugVertex vertices[MAX_DEBUG_VERTICES];
	/**
	 * Vertices so far
	 */
	size_t nOfUsedVertices;
	/**
	 * Vertex declaration for debug data
	 */
	IDirect3DVertexDeclaration9 * debugVertexDeclaration;
	/**
	 * Vertex element for our debug verte
	 */
	IDirect3DDevice9 * d3dDevice;
	/**
	 * Vertex shader pointer
	 */
	LPDIRECT3DVERTEXSHADER9 vertexShader;
	/**
	 * Vertex shader pointer
	 */
	LPDIRECT3DPIXELSHADER9 pixelShader;
	/**
	 * Vertex shader constants table
	 */
	ID3DXConstantTable * vertexConstantTable;
	/**
	 * Singleton instance
	 */
	static CDebugRenderer _instance;

public:
	CDebugRenderer(void);
	~CDebugRenderer(void);

	void AddVector(float x1, float y1, float z1, float x2, float y2, float z2, D3DCOLOR color);
	void AddVector(float x, float y, float z, float size, D3DCOLOR color);
	void AddVector(float px, float py, float pz, float x1, float y1, float z1, float x2, float y2, float z2, D3DCOLOR color);
	void AddVector(float px, float py, float pz, float x, float y, float z, float size, D3DCOLOR color);
	
	void Draw();
	void Initialize(IDirect3DDevice9 * device); 
	static CDebugRenderer * getInstance();
	
	void Update(float * modelViewMatrix);
};

