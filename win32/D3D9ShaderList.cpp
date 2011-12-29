#include "D3D9ShaderList.h"


CD3D9ShaderList::CD3D9ShaderList(void)
{
	lpD3D9Device = NULL;
}


CD3D9ShaderList::~CD3D9ShaderList(void)
{
}

void CD3D9ShaderList::InitialiseShaderList(IDirect3DDevice9 * device)
{
	lpD3D9Device = device;
}

void CD3D9ShaderList::CreateVertexShader(LPCSTR shaderPath,uint32_t shaderID,D3DVERTEXELEMENT9 * vertexElementDeclaration)
{
	// Creates a new D3D9VertexShader
	CD3D9VertexShader * vs = new CD3D9VertexShader();
	// Creates vertex element declaration
	HRESULT error = lpD3D9Device->CreateVertexDeclaration(vertexElementDeclaration,&vs->vertexDeclaration);
	// Failed?
	if(FAILED(error))
	{
		OutputDebugString(DXGetErrorString(error));
		OutputDebugString(DXGetErrorDescription(error));
	}
	// Poitner to the compiled shader assembly
	ID3DXBuffer * compiledShader = NULL;
	// Pointer to any possible output messages
	ID3DXBuffer * outputMessages = NULL;
	// Create shader from file
	error = D3DXCompileShaderFromFile(shaderPath,NULL,NULL,"vs_main","vs_2_0",NULL,&compiledShader,&outputMessages,&vs->vertexShaderConstants);
	// Failed?
	if(FAILED(error))
	{
		OutputDebugString(DXGetErrorString(error));
		OutputDebugString(DXGetErrorDescription(error));
	}
	// Any errors?
	if(outputMessages)
	{
		char * err = new char[outputMessages->GetBufferSize()];
		sprintf(err,"Vertex Shader compilation output: \n %s",outputMessages->GetBufferPointer());
		OutputDebugString(err);
	}
	// Release messages (if any)
	if(outputMessages) outputMessages->Release();
	// Create vertex shader from assembly
	error = lpD3D9Device->CreateVertexShader((DWORD *)compiledShader->GetBufferPointer(),&vs->vertexShader);
	// Failed?
	if(FAILED(error))
	{
		OutputDebugString(DXGetErrorString(error));
		OutputDebugString(DXGetErrorDescription(error));
	}
	// Release shader assembly
	compiledShader->Release();
	// If all went well, add to the list under the shader ID 
	vertexShaders.insert(std::pair<uint32_t,CD3D9VertexShader *>(shaderID,vs));

}
void CD3D9ShaderList::CreatePixelShader(LPCSTR shaderPath, uint32_t shaderID)
{
	// Creates a new D3D9VertexShader
	CD3D9PixelShader * ps = new CD3D9PixelShader();
	// Poitner to the compiled shader assembly
	ID3DXBuffer * compiledShader = NULL;
	// Pointer to any possible output messages
	ID3DXBuffer * outputMessages = NULL;
	// Create shader from file
	HRESULT error = D3DXCompileShaderFromFile(shaderPath,NULL,NULL,"ps_main","ps_2_0",NULL,&compiledShader,&outputMessages,&ps->pixelShaderConstants);
	// Failed?
	if(FAILED(error))
	{
		OutputDebugString(DXGetErrorString(error));
		OutputDebugString(DXGetErrorDescription(error));
	}
	// Any errors?
	if(outputMessages)
	{
		char * err = new char[outputMessages->GetBufferSize()];
		sprintf(err,"Pixel  Shader compilation output: \n %s",outputMessages->GetBufferPointer());
		OutputDebugString(err);
	}
	// Release messages (if any)
	if(outputMessages) outputMessages->Release();
	// Create vertex shader from assembly
	error = lpD3D9Device->CreatePixelShader((DWORD *)compiledShader->GetBufferPointer(),&ps->pixelShader);
	// Failed?
	if(FAILED(error))
	{
		OutputDebugString(DXGetErrorString(error));
		OutputDebugString(DXGetErrorDescription(error));
	}
	// Release shader assembly
	compiledShader->Release();
	// If all went well, add to the list under the shader ID 
	pixelShaders.insert(std::pair<uint32_t,CD3D9PixelShader *>(shaderID,ps));
}

void CD3D9ShaderList::SetVertexShaderViewProjectionMatrix(D3DXMATRIX * viewProj, uint32_t shaderID)
{
	CD3D9VertexShader * vs;
	if(vertexShaders.find(shaderID) != vertexShaders.end())
		vs = vertexShaders[shaderID];
	HRESULT error = vs->vertexShaderConstants->SetMatrix(lpD3D9Device,vs->vertexShaderConstants->GetConstantByName(NULL,"ViewProjMatrix"),viewProj);
	
	// Failed?
	if(FAILED(error))
	{
		OutputDebugString(DXGetErrorString(error));
		OutputDebugString(DXGetErrorDescription(error));
	}
	
}
void CD3D9ShaderList::SetVertexShaderModelMatrix(D3DXMATRIX * model, uint32_t shaderID)
{
	return;
	CD3D9VertexShader * vs;
	if(vertexShaders.find(shaderID) != vertexShaders.end())
		vs = vertexShaders[shaderID];

	HRESULT error = vs->vertexShaderConstants->SetMatrix(lpD3D9Device,vs->vertexShaderConstants->GetConstantByName(NULL,"ModelMatrix"),model);
	// Failed?
	if(FAILED(error))
	{
		OutputDebugString(DXGetErrorString(error));
		OutputDebugString(DXGetErrorDescription(error));
	}
}

CD3D9VertexShader * CD3D9ShaderList::GetVertexShader(uint32_t shaderID)
{
	if(vertexShaders.find(shaderID) != vertexShaders.end())
		return vertexShaders[shaderID];	
	else return 0;
}
CD3D9PixelShader * CD3D9ShaderList::GetPixelShader(uint32_t shaderID)
{
	if(pixelShaders.find(shaderID) != pixelShaders.end())
		return pixelShaders[shaderID];	
	else return 0;

	
}

void CD3D9ShaderList::SetShaderAndVertexDeclaration(uint16_t shaderID)
{
	HRESULT error = lpD3D9Device->SetVertexDeclaration(vertexShaders[shaderID]->vertexDeclaration);
	error = lpD3D9Device->SetVertexShader(vertexShaders[shaderID]->vertexShader);
	// Just for test
	error = lpD3D9Device->SetPixelShader(pixelShaders[shaderID]->pixelShader);

	if(FAILED(error))
	{
		OutputDebugString(DXGetErrorString(error));
		OutputDebugString(DXGetErrorDescription(error));
	}
}