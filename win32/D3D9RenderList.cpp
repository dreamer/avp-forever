#include "D3D9RenderList.h"


CD3D9RenderList::CD3D9RenderList(void)
{
}


CD3D9RenderList::~CD3D9RenderList(void)
{
	
}
void CD3D9RenderList::Destroy()
{
	dynamicIndices->Release();
	staticIndices->Release();

	dynamicVertices->Release();
	staticVertices->Release();
}


void CD3D9RenderList::InitialiseRenderList(IDirect3DDevice9 * device)
{
	lpD3DDevice = device;
	HRESULT error;
	dynamicIndicesCount = staticIndicesCount = 0;
	dynamicVerticesCount = staticVerticesCount = 0;
	error = lpD3DDevice->CreateVertexBuffer(10000 * sizeof(CD3D9RenderVertex),D3DUSAGE_DYNAMIC  | D3DUSAGE_WRITEONLY,0,D3DPOOL_DEFAULT,&dynamicVertices,NULL);
	if(FAILED(error))
	{
		OutputDebugString("Failed to create vertex buffer");
	}
	error = lpD3DDevice->CreateIndexBuffer(10000 * sizeof(CD3D9RenderVertex),D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,D3DFMT_INDEX16,D3DPOOL_DEFAULT,&dynamicIndices,NULL);
	if(FAILED(error))
	{
		OutputDebugString("Failed to create index buffer");
	}
	error = lpD3DDevice->CreateVertexBuffer(10000 * sizeof(CD3D9RenderVertex),D3DUSAGE_WRITEONLY ,0,D3DPOOL_MANAGED,&staticVertices,NULL);
	if(FAILED(error))
	{
		OutputDebugString("Failed to create vertex buffer");
	}
	error = lpD3DDevice->CreateIndexBuffer(10000 * sizeof(CD3D9RenderVertex),D3DUSAGE_WRITEONLY,D3DFMT_INDEX32,D3DPOOL_MANAGED,&staticIndices,NULL);
	if(FAILED(error))
	{
		OutputDebugString("Failed to create index buffer");
	}

}

void CD3D9RenderList::AddRenderItem(CD3D9RenderItem * renderItem)
{
	dynamicRenderItems.push_back(renderItem);
}


void CD3D9RenderList::AddDynamicVertexData(CD3D9RenderItem * renderItem , void * vertexData, size_t vertexSize, unsigned nOfVertices)
{
	CD3D9RenderVertex * vertices;
	// Put our vertices in place
	HRESULT error = dynamicVertices->Lock(dynamicVerticesCount * vertexSize,nOfVertices * vertexSize,(void **)&vertices,NULL);
	if(FAILED(error))
	{
		OutputDebugString("Failed to update vertex buffer");
	}
	// Copies into the vertex buffer's pointer
	memcpy(vertices,vertexData,nOfVertices * vertexSize);
	// Unlock this buffer for drawing
	error = dynamicVertices->Unlock();
	if(FAILED(error))
	{
		OutputDebugString("Failed to unlock vertex buffer");
	}
	// DirectX expects some interesting values for it's DrawIndexedPrimitve
	// These are the offset of the start of the index buffer to our indexes, the offset
	// of the vertex buffer to our 1st vertex and where in the offset of the index buffer
	// we will start to draw (case we want to skip triangles I believe...)
	renderItem->SetIndexRegion(dynamicIndicesCount,nOfVertices,dynamicVerticesCount);
	// Updates the vertices count
	dynamicVerticesCount += nOfVertices;
	// As number of vertices increased by 1, number of indices increase by 3
	// this is a  linear function for calculating that (given that we have
	// more than 3 vertices to form at least 1 triangle)
	uint32_t nOfIndices = 3 + ((nOfVertices-3) * 3); 
	// Creates a pointer to the index array
	uint16_t * indexArray;
	// Gets a pointer to the index array
	error = dynamicIndices->Lock(dynamicIndicesCount * sizeof(uint16_t), nOfIndices * sizeof(uint16_t),(void **)&indexArray,NULL);
	if(FAILED(error))
	{
		OutputDebugString("Failed to update index buffer");
	}
	uint32_t localOffset = 0;
	// Yeah we'll get a better solution for that
	switch (nOfVertices)
	{
		default:
			OutputDebugString("Asked to render unexpected number of verts in CreateIndices");
			break;
		case 0:
			OutputDebugString("Asked to render 0 verts in CreateIndices");
			break;
		case 3:
		{
			AddDynamicIndices(indexArray, 0,2,1, 3,localOffset);
			break;
		}
		case 4:
		{
			// winding order for main avp rendering system
			AddDynamicIndices(indexArray, 0,1,2, 4,localOffset);
			AddDynamicIndices(indexArray, 0,2,3, 4,localOffset);
			break;
		}
		case 5:
		{
			AddDynamicIndices(indexArray, 0,1,4, 5,localOffset);
			AddDynamicIndices(indexArray, 1,3,4, 5,localOffset);
			AddDynamicIndices(indexArray, 1,2,3, 5,localOffset);
			break;
		}
		case 6:
		{
			AddDynamicIndices(indexArray, 0,4,5, 6,localOffset);
			AddDynamicIndices(indexArray, 0,3,4, 6,localOffset);
			AddDynamicIndices(indexArray, 0,2,3, 6,localOffset);
			AddDynamicIndices(indexArray, 0,1,2, 6,localOffset);
			break;
		}
		case 7:
		{
			AddDynamicIndices(indexArray, 0,5,6, 7,localOffset);
			AddDynamicIndices(indexArray, 0,4,5, 7,localOffset);
			AddDynamicIndices(indexArray, 0,3,4, 7,localOffset);
			AddDynamicIndices(indexArray, 0,2,3, 7,localOffset);
			AddDynamicIndices(indexArray, 0,1,2, 7,localOffset);
			break;
		}
		case 8:
		{
			AddDynamicIndices(indexArray, 0,6,7, 8,localOffset);
			AddDynamicIndices(indexArray, 0,5,6, 8,localOffset);
			AddDynamicIndices(indexArray, 0,4,5, 8,localOffset);
			AddDynamicIndices(indexArray, 0,3,4, 8,localOffset);
			AddDynamicIndices(indexArray, 0,2,3, 8,localOffset);
			AddDynamicIndices(indexArray, 0,1,2, 8,localOffset);
			break;
		}
	}
	error = dynamicIndices->Unlock();
	if(FAILED(error))
	{
		OutputDebugString("Failed to unlock vertex buffer");
	}
	
}
/** 
 * Copied from BJD code, adds a triangle to the index array and increments dynamicIndices count
 */
void CD3D9RenderList::AddDynamicIndices(uint16_t *indexArray, uint32_t a, uint32_t b, uint32_t c, uint32_t n, uint32_t& localOffset)
{
	// Notice that, unlike BJD's code, we have an offset pointer so we don't need 
	// to use dynamicIndicesCount as offset
	indexArray[localOffset+0] = dynamicVerticesCount - n + a;
	indexArray[localOffset+1] = dynamicVerticesCount - n + b;
	indexArray[localOffset+2] = dynamicVerticesCount - n + c;
	dynamicIndicesCount+=3;
	localOffset+=3;
	/*
	indexArray[this->indexCount]   = (vertexCount - (n) + (a));
	indexArray[this->indexCount+1] = (vertexCount - (n) + (b));
	indexArray[this->indexCount+2] = (vertexCount - (n) + (c));
	this->indexCount+=3;
	*/
}

CD3D9RenderItem * CD3D9RenderList::GetNextDynamicRenderItem()
{
	CD3D9RenderItem * ri = dynamicRenderItems.back();
	dynamicRenderItems.pop_back();
	return ri;
}
void CD3D9RenderList::DrawRenderItem(CD3D9RenderItem * ri)
{
	lpD3DDevice->SetStreamSource(0,dynamicVertices,0,sizeof(CD3D9RenderVertex));
	lpD3DDevice->SetIndices(dynamicIndices);
	// Draw an indexed primitive
	lpD3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,dynamicVerticesCount,ri->GetIndexOffset(),ri->GetNumberOfPrimitives());
	// Now we no longer need this (for dynamic indices of course)
	delete ri;
}

uint32_t CD3D9RenderList::NumberOfDynamicIndices()
{
	return dynamicRenderItems.size();
}

void CD3D9RenderList::PrepareForNextFrame()
{
	dynamicIndicesCount = 0;
	dynamicVerticesCount = 0;
	dynamicRenderItems.clear();
}

bool RenderItemSort(CD3D9RenderItem * d1, CD3D9RenderItem * d2)
{
	return d1->GetIndexOffset() > d2->GetIndexOffset();
}

void CD3D9RenderList::SortAndOptimizeRenderItems()
{
	if(!dynamicRenderItems.size()) return;
	// Sort all render items based on their texture ids	
	std::sort(dynamicRenderItems.begin(),dynamicRenderItems.end(),RenderItemSort);
	return;
	// And then... we sort
	uint16_t * indices;
	// Get a pointer to our dynamic indices
	dynamicIndices->Lock(0,dynamicIndicesCount * sizeof(uint16_t),(void **)&indices,NULL);
	// Ugh? just for test
	uint16_t * sortedIndices = new uint16_t[dynamicIndicesCount];

	// 1st we get a sample of the 1st index
	std::vector<CD3D9RenderItem *>::iterator currentBaseRenderItem = dynamicRenderItems.begin();
	
	std::vector<CD3D9RenderItem *>::iterator ri = dynamicRenderItems.begin();
	
	uint32_t sortedOffset = 0;
	uint32_t nOfTotalIndices = 0;
	uint32_t drawOffset = 0;
	// While we have render items
	while(true)
	{
		// Get it's offset in the index array
		uint32_t currentOffset = (*ri)->GetIndexOffset();
		// Get it's number of indices
		uint32_t numberOfVertices = (*ri)->GetNumberOfVertices();
		uint32_t numberOfIndices = 3 + ((numberOfVertices-3) * 3); 
		// Copy it to the beginning of the array
		memcpy(sortedIndices+sortedOffset,indices+currentOffset,numberOfIndices * sizeof(uint16_t));
		
		// If we have a different RI, delete all the similar ones and 
		// advance
		if(!(*ri)->isEqual(*currentBaseRenderItem))
		{
			dynamicRenderItems.erase((currentBaseRenderItem+1),ri);
			(*currentBaseRenderItem)->SetIndexRegion(drawOffset,nOfTotalIndices,0);
			nOfTotalIndices = 0;
			currentBaseRenderItem++;
			ri = currentBaseRenderItem+1;
			drawOffset = sortedOffset;
			
		}
		else // Else, just advance
		{
			ri++;
		}
		// Add the copied indices to the current offset
		sortedOffset += numberOfIndices;
		nOfTotalIndices += numberOfIndices;

		if(ri == dynamicRenderItems.end())
		{
			dynamicRenderItems.erase((currentBaseRenderItem+1),ri);
			(*currentBaseRenderItem)->SetIndexRegion(sortedOffset,nOfTotalIndices,0);
			break;
		}
	}
	

	memcpy(indices,sortedIndices,dynamicIndicesCount * sizeof(uint16_t));
	delete[] sortedIndices;
	dynamicIndices->Unlock();
	return;
}