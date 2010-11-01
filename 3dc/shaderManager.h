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

#ifndef _shaderManager_h_
#define _shaderManager_h_

#include "renderer.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <utility>

typedef uint32_t effectID_t;
typedef uint32_t shaderID_t;

// shader constant types
enum SHADER_CONSTANT
{
	CONST_INT,
	CONST_FLOAT,
	CONST_MATRIX
	// etc etc
};

// class version of above
class VertexShaderPool
{
	private:
		int32_t currentSetShaderID;
		
	public:
		VertexShaderPool()
		{
			currentSetShaderID = -1;
		}

		std::vector<r_VertexShader> shaderList;

		uint32_t Add(r_VertexShader newShader);
		bool SetActive(uint32_t shaderID);
		uint32_t GetShaderByName(const std::string &shaderName);
		void Remove(uint32_t shaderID);
		void AddRef(uint32_t shaderID) { shaderList[shaderID].refCount++; }
//		bool SetMatrix(uint32_t shaderID, const char* constant, R_MATRIX &matrix);
//		bool SetFloat(uint32_t shaderID, const char* constant, float n);
//		bool SetInt(uint32_t shaderID, const char* constant, int32_t n);
		bool SetVertexShaderConstant(uint32_t shaderID, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData);
};

class PixelShaderPool
{
	private:
		int32_t currentSetShaderID;

	public:
		PixelShaderPool()
		{
			currentSetShaderID = -1;
		}

		std::vector<r_PixelShader> shaderList;

		uint32_t Add(r_PixelShader newShader);
		bool SetActive(uint32_t shaderID);
		uint32_t GetShaderByName(const std::string &shaderName);
		void Remove(uint32_t shaderID);
		void AddRef(uint32_t shaderID) { shaderList[shaderID].refCount++; }
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
		PixelShaderPool	 psPool;

	public:
		EffectManager();
		bool SetActive(effectID_t effectID);
		bool SetVertexShaderConstant(effectID_t effectID, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData);
//		bool SetMatrix(effectID_t effectID, const char* constant, R_MATRIX &matrix);
//		bool SetFloat(effectID_t effectID, const char* constant, float n);
//		bool SetInt(effectID_t effectID, const char* constant, int32_t n);
		effectID_t AddEffect(const std::string &effectName, const std::string &vertexShaderName, const std::string &pixelShaderName, class VertexDeclaration *vertexDeclaration);
};

#endif
