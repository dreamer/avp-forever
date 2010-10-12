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

#include "shaderManager.h"
#include <vector>
#include "console.h"

const int nullID = 999;

// add an already created vertexShader_t struct to our pool.
uint32_t VertexShaderPool::Add(r_VertexShader newShader)
{
	// see if it already exists, and return the ID to it if it does
	uint32_t shaderID = GetShaderByName(newShader.shaderName);

	if (shaderID != nullID)
	{
		return shaderID;
	}

	// it's not in the list, add it
	this->shaderList.push_back(newShader);

	// it's at the end, so our ID is the last array position
	return this->shaderList.size() - 1;
}

// remove the shader at ID position passed in
void VertexShaderPool::Remove(uint32_t shaderID)
{
	this->shaderList.erase(this->shaderList.begin() + shaderID);
}

// set the shader as active (eg SetVertexShader() in D3D)
bool VertexShaderPool::SetActive(uint32_t shaderID)
{
	if (this->currentSetShaderID == shaderID) // already set
	{
		return true;
	}

	if (R_SetVertexShader(this->shaderList[shaderID]))
	{
		currentSetShaderID = shaderID;
		return true;
	}

	return false;
}

// pass a shader name in and see if it exists in the list. If it does, return
// the position ID of that shader. Otherwise, return nullID to signify it's not
// in the list (so we can then add it)
uint32_t VertexShaderPool::GetShaderByName(const std::string &shaderName)
{
	for (uint32_t i = 0; i < this->shaderList.size(); i++)
	{
		if (this->shaderList[i].shaderName == shaderName)
		{
			return i;
		}
	}

	return nullID;
}

bool VertexShaderPool::SetMatrix(uint32_t shaderID, const char* constant, R_MATRIX &matrix)
{
	return R_SetVertexShaderMatrix(shaderList[shaderID], constant, matrix);
}

bool VertexShaderPool::SetInt(uint32_t shaderID, const char* constant, int32_t n)
{
	return R_SetVertexShaderInt(shaderList[shaderID], constant, n);
}

// add an already created pixelShader_t struct to our pool.
uint32_t PixelShaderPool::Add(r_PixelShader newShader)
{
	// see if it already exists, and return the ID to it if it does
	uint32_t shaderID = GetShaderByName(newShader.shaderName);

	if (shaderID != nullID)
	{
		return shaderID;
	}

	// it's not in the list, add it
	this->shaderList.push_back(newShader);

	// it's at the end, so our ID is the last array position
	return this->shaderList.size() - 1;
}

// remove the shader at ID position passed in
void PixelShaderPool::Remove(uint32_t shaderID)
{
	this->shaderList.erase(this->shaderList.begin() + shaderID);
}

// set the shader as active (eg SetPixelShader() in D3D)
bool PixelShaderPool::SetActive(uint32_t shaderID)
{
	if (this->currentSetShaderID == shaderID) // already set
	{
		return true;
	}

	if (R_SetPixelShader(this->shaderList[shaderID]))
	{
		currentSetShaderID = shaderID;
		return true;
	}

	return false;
}

// pass a shader name in and see if it exists in the list. If it does, return
// the position ID of that shader. Otherwise, return nullID to signify it's not
// in the list (so we can then add it)
uint32_t PixelShaderPool::GetShaderByName(const std::string &shaderName)
{
	for (uint32_t i = 0; i < this->shaderList.size(); i++)
	{
		if (this->shaderList[i].shaderName == shaderName)
		{
			return i;
		}
	}

	return nullID;
}

EffectManager::EffectManager()
{
}

bool EffectManager::SetActive(effectID_t effectID)
{
	vsPool.SetActive(effectList[effectID].vertexShaderID);
	psPool.SetActive(effectList[effectID].pixelShaderID);

	return true;
}

bool EffectManager::SetMatrix(effectID_t effectID, const char* constant, R_MATRIX &matrix)
{
	// just call another SetMatrix function within the vertex shader class
	// until I find a tidier way to do this
	vsPool.SetMatrix(effectList[effectID].vertexShaderID, constant, matrix);
	return true;
}

bool EffectManager::SetInt(effectID_t effectID, const char* constant, int32_t n)
{
	// just call another SetInt function within the vertex shader class
	// until I find a tidier way to do this
	vsPool.SetInt(effectList[effectID].vertexShaderID, constant, n);
	return true;
}

effectID_t EffectManager::AddEffect(const std::string &effectName, const std::string &vertexShaderName, const std::string &pixelShaderName, const VertexDeclaration *vertexDeclaration)
{
	shaderID_t vertexID = nullID;
	shaderID_t pixelID = nullID;
	effectID_t effectID = nullID;

	// see if we already have an effect with that name
	for (uint32_t i = 0; i < this->effectList.size(); i++)
	{
		if (effectList[i].effectName == effectName && effectList[i].isValid)
		{
			Con_PrintMessage("Effect " + effectName + " already in use. reusing");
			return i;
		}

		// while we're here, lets just grab a free slot to use later if needed
		if (effectList[i].isValid == false)
		{
			effectID = i;
		}
	}

	// see if the vertex shader is already loaded.
	vertexID = vsPool.GetShaderByName(vertexShaderName);

	// see if the pixel shader is already loaded.
	pixelID = psPool.GetShaderByName(pixelShaderName);

	// load a vertex shader if required
	if (vertexID == nullID) // we have no slot or we don't already exist
	{
		r_VertexShader newVertexShader;
		newVertexShader.isValid = false;

		if (R_CreateVertexShader(vertexShaderName, newVertexShader))
		{
			// ok, store it
			newVertexShader.isValid = true;
			newVertexShader.shaderName = vertexShaderName;

			vertexID = vsPool.Add(newVertexShader);
		}
	}

	// load the pixel shader if required
	if (pixelID == nullID) // we have no slot or we don't already exist
	{
		r_PixelShader newPixelShader;
		newPixelShader.isValid = false;

		if (R_CreatePixelShader(pixelShaderName, newPixelShader))
		{
			// ok, store it
			newPixelShader.isValid = true;
			newPixelShader.shaderName = pixelShaderName;

			pixelID = psPool.Add(newPixelShader);
		}
	}

	// now the effect itself
	effect_t newEffect;
	newEffect.isValid = true;
	newEffect.effectName = effectName;
	newEffect.vertexShaderID = vertexID;
	newEffect.pixelShaderID = pixelID;

	// before we add it, increment reference counters
	vsPool.AddRef(vertexID);
	psPool.AddRef(pixelID);

	// do we already have an effect slot?
	if (effectID != nullID)
	{
		effectList[effectID] = newEffect;
		return effectID;
	}
	else // no slot, add to end of list
	{
		effectList.push_back(newEffect);
		return effectList.size() - 1;
	}

	// if we get here.. I dunno
	return nullID;
}
