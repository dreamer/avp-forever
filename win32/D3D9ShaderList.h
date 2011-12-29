#pragma once

#define D3D_DEBUG_INFO


#include <d3d9.h>
#include <DxErr.h>
#include <d3dx9.h>
#include <map>
#include <stdint.h>
/**
 * A vertex shader containing the shader and a
 * declaration of our vertex structure
 */
class CD3D9VertexShader
{
public:
	CD3D9VertexShader()
	{
		vertexDeclaration = NULL;
		vertexShader = NULL;
	}
	friend class CD3D9ShaderList;
protected:
	/**
	 * Vertex declaration for this particular shader
	 */
	IDirect3DVertexDeclaration9 * vertexDeclaration;
	/**
	 * Pointer to our actual vertex shader
	 */ 
	IDirect3DVertexShader9 * vertexShader;
	/**
	 * Pointer to the vertex shader's constants
	 */
	ID3DXConstantTable * vertexShaderConstants;
};

struct shaderKeyCompareFunc
{
	bool operator() (const uint32_t& lhs, const uint32_t& rhs) const
	{
		return lhs<rhs;
	}
};

/**
 * A pixel shader
 */
class CD3D9PixelShader
{
public:
	CD3D9PixelShader()
	{
		pixelShader = NULL;
		pixelShaderConstants = NULL;
	}
	friend class CD3D9ShaderList;
protected:
	/**
	 * Pointer to our actual pixel shader
	 */
	IDirect3DPixelShader9 * pixelShader;
	/**
	 * Pointer to our pixel shader's constants
	 */
	ID3DXConstantTable * pixelShaderConstants;
};
/**
 * Our Direct3D 9 shader list
 */
class CD3D9ShaderList
{
protected:
	IDirect3DDevice9 * lpD3D9Device;
	/**
	 * List of vertex shaders
	 */ 
	std::map<uint32_t,CD3D9VertexShader *,shaderKeyCompareFunc> vertexShaders;
	/**
	 * List of pixel shaders
	 */
	std::map<uint32_t,CD3D9PixelShader *,shaderKeyCompareFunc> pixelShaders;


public:
	CD3D9ShaderList(void);
	~CD3D9ShaderList(void);

	void InitialiseShaderList(IDirect3DDevice9 * device);

	void CreateVertexShader(LPCSTR shaderPath,uint32_t shaderID,D3DVERTEXELEMENT9 * vertexElementDeclaration);
	void CreatePixelShader(LPCSTR shaderPath, uint32_t shaderID);

	void SetVertexShaderViewProjectionMatrix(D3DXMATRIX * viewProj, uint32_t shaderID);
	void SetVertexShaderModelMatrix(D3DXMATRIX * model, uint32_t shaderID);

	CD3D9VertexShader * GetVertexShader(uint32_t shaderID);
	CD3D9PixelShader * GetPixelShader(uint32_t shaderID);

	void SetShaderAndVertexDeclaration(uint16_t shaderID);

	friend class CD3D9VertexShader;
	friend class CD3D9PixelShader;
};

