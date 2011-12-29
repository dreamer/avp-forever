#pragma once
#define D3D_DEBUG_INFO
#include <d3d9.h>
#include "3dc.h"
#include "D3D9RenderDefines.h"
#include "D3D9RenderList.h"
#include "D3D9ShaderList.h"
#include "D3D9AVPAuxiliaryFunctions.h"


/**
 * D3D Renderer for Direct3D9
 */
class CD3D9Renderer
{
protected:
	/**
	 * Pointer to our render list
	 */
	CD3D9RenderList * renderList;
	/**
	 * Pointer to our shader list
	 */
	CD3D9ShaderList * shaderList;
	/**
	 * Pointer to Direct3D Device
	 */
	IDirect3DDevice9 * lpD3DDevice;
	/**
	 * ViewMatrix
	 */
	D3DXMATRIX viewMatrix;
	/**
	 * projectionMatrix
	 */
	D3DXMATRIX projMatrix;
private:
	/**
	 * Singleton instance
	 */
	static CD3D9Renderer _instance;
	CD3D9Renderer(void);
	~CD3D9Renderer(void);
public:
	
	static CD3D9Renderer * GetInstance();
	/**
	 * Initialize the renderer
	 */
	void Initialise(IDirect3DDevice9 * device);
	/**
	 * Creates a render item, vertex and index data from a AvP display block
	 * roughly what ShapePipeline does
	 */
	void ProcessAVPDisplayBlock(DISPLAYBLOCK * displayblock);
	/**
	 * Draws all geometric data in the buffers
	 */
	void DrawGeometry();
	/**
	 * Sets the View/Projection matrix
	 */
	void SetViewProjectionMatrix(D3DXMATRIX * projectionMatrix);
};

