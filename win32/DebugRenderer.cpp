#include "DebugRenderer.h"

CDebugRenderer CDebugRenderer::_instance;

CDebugRenderer::CDebugRenderer(void)
{
}

CDebugRenderer::~CDebugRenderer(void)
{
}
/**
 * Adds a vector from a point to another
 */
void CDebugRenderer::AddVector(float x1, float y1, float z1, float x2, float y2, float z2, D3DCOLOR color)
{
	if((nOfUsedVertices + 2) > MAX_DEBUG_VERTICES) return;
	vertices[nOfUsedVertices].position = D3DXVECTOR3(x1,y2,z1);
	vertices[nOfUsedVertices].diffuse = color;
	nOfUsedVertices++;
	vertices[nOfUsedVertices].position = D3DXVECTOR3(x2,y2,z2);
	vertices[nOfUsedVertices].diffuse = color;
	nOfUsedVertices++;

}
/**
 * Adds a vector from a point to another using magnitude
 */
void CDebugRenderer::AddVector(float x, float y, float z, float size,  D3DCOLOR color)
{
	if((nOfUsedVertices + 2) > MAX_DEBUG_VERTICES) return;
	vertices[nOfUsedVertices].position = D3DXVECTOR3(x,y,z);
	vertices[nOfUsedVertices].diffuse = color;
	nOfUsedVertices++;
	vertices[nOfUsedVertices].position = D3DXVECTOR3(x * size,y * size,z * size);
	vertices[nOfUsedVertices].diffuse = color;
	nOfUsedVertices++;
}

/**
 * Adds a vector from a point to another relative to an anchor point
 */
void CDebugRenderer::AddVector(float px, float py, float pz, float x1, float y1, float z1, float x2, float y2, float z2, D3DCOLOR color)
{
	if((nOfUsedVertices + 2) > MAX_DEBUG_VERTICES) return;
	vertices[nOfUsedVertices].position = D3DXVECTOR3(x1+px,y2+py,z1+pz);
	vertices[nOfUsedVertices].diffuse = color;
	nOfUsedVertices++;
	vertices[nOfUsedVertices].position = D3DXVECTOR3(x2+px,y2+py,z2+pz);
	vertices[nOfUsedVertices].diffuse = color;
	nOfUsedVertices++;
}

/**
 * Adds a vector from a point to another using magnitude and relative to an achor point
 */ 
void CDebugRenderer::AddVector(float px, float py, float pz, float x, float y, float z, float size, D3DCOLOR color)
{
	if((nOfUsedVertices + 2) > MAX_DEBUG_VERTICES) return;
	vertices[nOfUsedVertices].position = D3DXVECTOR3(x + px,y + py,z + pz);
	vertices[nOfUsedVertices].diffuse = color;
	nOfUsedVertices++;
	vertices[nOfUsedVertices].position = D3DXVECTOR3((x * size) + px,(y * size) + py,(z * size) + pz);
	vertices[nOfUsedVertices].diffuse = color;
	nOfUsedVertices++;
}


void CDebugRenderer::Draw()
{
	d3dDevice->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);
	d3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
	d3dDevice->SetVertexShader(vertexShader);
	d3dDevice->SetPixelShader(pixelShader);
	d3dDevice->SetVertexDeclaration(debugVertexDeclaration);
	d3dDevice->DrawPrimitiveUP(D3DPT_LINELIST,nOfUsedVertices / 2,vertices,sizeof(DebugVertex));
}


void CDebugRenderer::Initialize(IDirect3DDevice9 * device)
{
	d3dDevice = device;
	nOfUsedVertices = 0;
	// Vertex element declaration
	D3DVERTEXELEMENT9 debugVertexElement[] = {
		 {0,  0,  D3DDECLTYPE_FLOAT3,     D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,  0},
         {0, 12, D3DDECLTYPE_D3DCOLOR,    D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,      0},
         D3DDECL_END()};
	// Create declaration
	d3dDevice->CreateVertexDeclaration(debugVertexElement,&debugVertexDeclaration);
	// Shader code pointer
	ID3DXBuffer * shaderCode = NULL;
	// Loade debug vertex shader
	ID3DXBuffer * errors;

	HRESULT dxError;
	
	
	D3DXCompileShaderFromFile("DebugVertexShader.vsh",NULL,NULL,"vs_main","vs_2_0",NULL,&shaderCode,&errors,&vertexConstantTable);

	if(errors)
	{
		char * error = new char[errors->GetBufferSize()];
		sprintf(error,"Errors: %s",errors->GetBufferPointer());
		delete [] error;
	}
	// Create from buffer
	dxError = d3dDevice->CreateVertexShader( (DWORD*)shaderCode->GetBufferPointer(),&vertexShader );
	// Load debug pixel shader
	D3DXCompileShaderFromFile("DebugPixelShader.psh",NULL,NULL,"ps_main","ps_2_0",NULL,&shaderCode,&errors,NULL);

	if(errors)
	{
		char * error = new char[errors->GetBufferSize()];
		sprintf(error,"Error %s",errors->GetBufferPointer());
		delete [] error;
	}

	dxError = d3dDevice->CreatePixelShader( (DWORD*)shaderCode->GetBufferPointer(),&pixelShader);

}

/**
 * Return instance
 */ 
CDebugRenderer * CDebugRenderer::getInstance()
{
	return &_instance;
}
/**
 * Updates the modelview matrix
 */
void CDebugRenderer::Update(float * modelViewMatrix)
{
	nOfUsedVertices = 0;

	//D3DXMATRIX proj(modelViewMatrix);
	//D3DXMATRIX trans;
	//D3DXMatrixTranspose(&trans,&proj);

	// Updates the modelview matrix
	//d3dDevice->SetVertexShader(vertexShader);
	HRESULT err = vertexConstantTable->SetFloatArray(d3dDevice,vertexConstantTable->GetConstant(NULL,0),modelViewMatrix,16);
	//HRESULT err = d3dDevice->SetVertexShaderConstantF(0,trans,4);
	if(err != S_OK)
		__asm int 3;
}
