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

#ifndef _ShaderManager_h_
#define _ShaderManager_h_

#include "renderer.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <utility>

const shaderID_t kNullShaderID = -1;
const effectID_t kNullEffectID = -1;

// shader constant types
enum SHADER_CONSTANT
{
	CONST_INT,
	CONST_FLOAT,
	CONST_VECTOR3,
	CONST_VECTOR4,
	CONST_MATRIX
	// etc etc
};

// class version of above
class VertexShaderPool
{
	private:
		shaderID_t currentSetShaderID;

	public:
		VertexShaderPool()
		{
			currentSetShaderID = kNullShaderID;
		}

		~VertexShaderPool();

		std::vector<r_VertexShader> shaderList;

		shaderID_t Add(r_VertexShader &newShader);
		bool SetActive(int32_t shaderID);
		shaderID_t GetShaderByName(const std::string &shaderName);
		void Remove(shaderID_t shaderID);
		void AddRef(shaderID_t shaderID) { shaderList[shaderID].refCount++; }
		bool SetVertexShaderConstant(shaderID_t shaderID, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData);
};

class PixelShaderPool
{
	private:
		shaderID_t currentSetShaderID;

	public:
		PixelShaderPool()
		{
			currentSetShaderID = kNullShaderID;
		}

		~PixelShaderPool();

		std::vector<r_PixelShader> shaderList;

		shaderID_t Add(r_PixelShader &newShader);
		bool SetActive(shaderID_t shaderID);
		shaderID_t GetShaderByName(const std::string &shaderName);
		void Remove(shaderID_t shaderID);
		void AddRef(shaderID_t shaderID) { shaderList[shaderID].refCount++; }
		bool SetPixelShaderConstant(shaderID_t shaderID, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData);
};

struct effect_t
{
	bool isValid;
	std::string effectName;
	shaderID_t vertexShaderID;
	shaderID_t pixelShaderID;
};

class EffectManager
{
	private:
		std::vector<effect_t> effectList;
		VertexShaderPool vsPool;
		PixelShaderPool  psPool;

	public:
		EffectManager();
		~EffectManager();
		bool SetActive(effectID_t effectID);
		bool SetVertexShaderConstant(effectID_t effectID, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData);
		bool SetPixelShaderConstant(effectID_t effectID, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData);
		effectID_t Add(const std::string &effectName, const std::string &vertexShaderName, const std::string &pixelShaderName, class VertexDeclaration *vertexDeclaration);
		void Remove(effectID_t effectID);
};

#endif
