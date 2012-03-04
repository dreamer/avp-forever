#include "D3D9Renderer.h"
#include "TextureManager.h"

extern std::vector<Texture> textureList;
CD3D9Renderer CD3D9Renderer::_instance;
extern DISPLAYBLOCK * Player;

CD3D9Renderer::CD3D9Renderer(void)
{
	lpD3DDevice = NULL;
}


CD3D9Renderer::~CD3D9Renderer(void)
{
	OutputDebugString("We're getting here? \n");
	renderList->Destroy();
}

/**
 * Return instance
 */
CD3D9Renderer * CD3D9Renderer::GetInstance()
{
	return &_instance;
}

void CD3D9Renderer::Initialise(IDirect3DDevice9 * device)
{
	lpD3DDevice = device;
	renderList = new CD3D9RenderList();
	renderList->InitialiseRenderList(lpD3DDevice);
	shaderList = new CD3D9ShaderList();
	shaderList->InitialiseShaderList(lpD3DDevice);

	D3DVERTEXELEMENT9 vertexElement[] = {
	{0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, 12, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
	{0, 24, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    {0, 32, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
	 D3DDECL_END()};

	shaderList->CreateVertexShader("shaders\\D3D9_DefaultVertexShader.vsh",AVPVS_DEFAULT,vertexElement);

	shaderList->CreatePixelShader("shaders\\D3D9_DefaultPixelShader.psh",AVPPS_DEFAULT);
}

/**
 * Here we process a display block, display blocks are the rendering
 * blocks of AvP containing the world positions, orientations, mesh
 * data, and animation controls. The structure also contains strategy
 * data and dynamics blocks, everything we need for the game to run.
 */
void CD3D9Renderer::ProcessAVPDisplayBlock(DISPLAYBLOCK * displayblock)
{
	// Creates the model matrix for this display block
	D3DXMATRIX * modelMatrix = new D3DXMATRIX;
	*modelMatrix = ConvertMatrixFixedToFloat(&displayblock->ObMat,&displayblock->ObWorld);

	// Get our shape from display block
	SHAPEHEADER * shape = displayblock->ObShapeData;

	// We are rendering something right?
	if(shape == NULL) return;

	// Get the number of polygons in this shape
	unsigned numitems = shape->numitems;

	// Get the polygons array (PolyHeaders)
	int **itemArrayPtr = shape->items;

	// For each item in this shape
	while(numitems)
	{
		// Counter for how many vertices this face has
		int numberOfVerticesInThisItem = 0;
		// Get the polygon's header
		POLYHEADER *polyPtr = (POLYHEADER*) (*itemArrayPtr++);
		// So, avp doesn't say how many idexes there are, you need to loop a pointer until you find a
		//  "Termination" flag which is an Index with value "-1" that takes up 32bits. Not making a
		// 8,16,32 bit variable like "numberOfVertices" so we can read and know in advance reamains one
		// of AvP's many mysteries.
		// Here we get the pointer for the 1st index
		int *vertexNumberPtr = &polyPtr->Poly1stPt;
		// Max number of polys? according to avp code, kShape.cpp, maxpolypts
		// no polygon has more than 9 vertices.
		CD3D9RenderVertex vertices[9];
		// UV indices are in a linear array for each vertex
		// First we get the starting point
		unsigned uvIndex = polyPtr->PolyColour >> 16;
		// Then a pointer to the 1st element
		int * uvArrayForThisPoly = shape->sh_textures[uvIndex];
		// Loop through the indexes and find the number of vertices and store their data
		D3DXVECTOR4 vec;
		for (int * vertexNumberPtr = &polyPtr->Poly1stPt ; (*vertexNumberPtr) != -1 ; vertexNumberPtr++)
		{
			// AvP's positions are scaled in milimiters, and are integers
			vertices[numberOfVerticesInThisItem].position = ConvertVectorIntegerToFloat((VECTORCH *)&(*shape->points)[(*vertexNumberPtr)]);
			D3DXVec3Transform(&vec,&vertices[numberOfVerticesInThisItem].position,modelMatrix);
			vertices[numberOfVerticesInThisItem].position = D3DXVECTOR3(vec.x,vec.y,vec.z);
			// AvP's normals are fixed point variables that need to be converted to float
			vertices[numberOfVerticesInThisItem].normal   = ConvertVectorFixedToFloat((VECTORCH *)&(*shape->sh_vnormals)[(*vertexNumberPtr)]);
			// We get our UVs from the offset, it's a linear array of pairs of UV coordinates stored sequentially
			vertices[numberOfVerticesInThisItem].uvCoords.x = uvArrayForThisPoly[0];
			vertices[numberOfVerticesInThisItem].uvCoords.y = uvArrayForThisPoly[1];
			// We then add to the offset to get the next 2 UV coordinates
			uvArrayForThisPoly+=2;
			// So we know how many actual vertices we got (for index calculations)
			numberOfVerticesInThisItem++;
		}
		// At this point we have all the vertices of our shape with position, normals and uv coordinates
		// The higher 16 bit of PolyCoulour hold the UV coordinate index offset, conveniently the texture
		// ID for this poly is in the lower 16 bits.
		unsigned textureId = polyPtr->PolyColour & 0x0000FFFF;
		// Now we should have all we need to build our RenderItem
		CD3D9RenderItem * renderItem = new CD3D9RenderItem();
		renderItem->SetModelMatrix(modelMatrix);
		renderItem->SetTextures(textureId);

		renderList->AddDynamicVertexData(renderItem,vertices,sizeof(CD3D9RenderVertex),numberOfVerticesInThisItem);

		renderList->AddRenderItem(renderItem);
		numitems--;

	}


}
void CD3D9Renderer::SetViewProjectionMatrix(D3DXMATRIX * projectionMatrix)
{
	shaderList->SetVertexShaderViewProjectionMatrix(projectionMatrix,AVPVS_DEFAULT);
}
void CD3D9Renderer::DrawGeometry()
{
	lpD3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	lpD3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	lpD3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
	lpD3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

	renderList->SortAndOptimizeRenderItems();

	unsigned numItems = renderList->NumberOfDynamicIndices();
	while(numItems--)
	{
		// So basically, what we'll do here, is to get a RenderItem, check it's states,
		// update the piepeline and then ask the RenderList to draw it, first we get the
		// RenderItem (using dynamics for now)
		CD3D9RenderItem * ri = renderList->GetNextDynamicRenderItem();
		D3DXMATRIX * modelMatrix = ri->GetModelMatrix();
		uint16_t texID = ri->GetTextureId();

		if(!textureList[texID].isValid)
		{
			OutputDebugString("CD3D9Renderer::DrawGeometry() invalid texture ID found, aborting");
			exit(0);
		}

		lpD3DDevice->SetTexture(0,textureList[texID].texture);

		D3DXMATRIX ide;
		D3DXMatrixIdentity(&ide);
		shaderList->SetShaderAndVertexDeclaration(ri->GetShaderId());
		shaderList->SetVertexShaderModelMatrix(&ide,0);


		renderList->DrawRenderItem(ri);
	}
	renderList->PrepareForNextFrame();

}
