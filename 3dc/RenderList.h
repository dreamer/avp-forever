// Copyright (C) 2010 Barry Duncan. All Rights Reserved.
// The original author of this code can be contacted at: bduncan22@hotmail.com

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdint.h>
#include <vector>
#include "renderStates.h"
#include "renderer.h"

const int kLayerWorld = 0;
const int kLayerWeapon = 1;
const int kLayerHUD = 2;

const int kCommandZClear = 0;
const int kCommandZWriteEnable = 1;
const int kCommandZWriteDisable = 2;

struct RenderItem
{
	uint32_t	vertStart;
	uint32_t	vertEnd;

	uint32_t	indexStart;
	uint32_t	indexEnd;
	
	/* 
	 * our various rendering states and properties (texture ID, filtering mode etc) are stored in the
	 * below value and are accessed and set using bit masks.
	 */
	uint32_t sortKey;

	// for std::sort - sort on the above sortKey value to allow sorting render states
	bool operator < (const RenderItem& rhs) const {return sortKey < rhs.sortKey;}
};

class RenderList
{
	private:
		std::string listName;

//		size_t		capacity;
		size_t		listIndex;	// used as list size

//		uint32_t	vertexCount;
//		uint32_t	indexCount;

		int layer;

		std::vector<RenderItem> Items;

		bool useIndicesOffset;

		void AddIndices(uint16_t *indexArray, uint32_t a, uint32_t b, uint32_t c, uint32_t n);

		// associated vertex, index and declaration
		VertexBuffer *vb;
		IndexBuffer  *ib;
		VertexDeclaration *decl;

	public:
		RenderList(const std::string &listName, size_t size, VertexBuffer *vb, IndexBuffer *ib, VertexDeclaration *decl);
		~RenderList();

		void Reset();
		void Init(size_t size);

//		size_t GetCapacity() const { return capacity; }
		size_t GetSize()     const { return listIndex; }

		uint32_t GetVertexCount() const { return vb->GetSize(); }
		uint32_t GetIndexCount()  const { return ib->GetSize(); }

		int GetCurrentLayer() const { return layer; }
	
		void AddCommand(int commandType);
		void SetLayer(int layer);

		void Sort();
		void AddTriangle(uint16_t *indexArray, uint32_t a, uint32_t b, uint32_t c, uint32_t n);
		void AddItem(uint32_t numVerts, texID_t textureID, enum TRANSLUCENCY_TYPE translucencyMode, enum FILTERING_MODE_ID filteringMode = FILTERING_BILINEAR_ON, enum TEXTURE_ADDRESS_MODE textureAddress = TEXTURE_WRAP);
		void CreateIndices(uint16_t *indexArray, uint32_t numVerts);
		void CreateOrthoIndices(uint16_t *indexArray);
		void Draw();

		// test
		void EnableIndicesOffset();
		void IncrementIndexCount(uint32_t nI);
};
