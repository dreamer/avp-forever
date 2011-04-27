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

#include "ShaderManager.h"
#include <vector>
#include "console.h"
#include <sstream>

// add an already created vertexShader_t struct to our pool.
shaderID_t VertexShaderPool::Add(r_VertexShader &newShader)
{
	// see if it already exists, and return the ID to it if it does
	shaderID_t shaderID = GetShaderByName(newShader.shaderName);

	if (shaderID != kNullShaderID)
	{
		return shaderID;
	}

	// it's not in the list, add it
	this->shaderList.push_back(newShader);

	// it's at the end, so our ID is the last array position
	shaderID = this->shaderList.size() - 1;

	return shaderID;
}

// remove the shader at ID position passed in
void VertexShaderPool::Remove(shaderID_t shaderID)
{
/*
	std::stringstream ss;
	ss << "about to remove vertex shader " << shaderList[shaderID].shaderName << " with id " << shaderID << " and refcount " << shaderList[shaderID].refCount << std::endl;
	OutputDebugString(ss.str().c_str());
*/
	shaderList[shaderID].refCount--;
	if (shaderList[shaderID].refCount <= 0)
	{
/*
		ss.str("");
		ss << "removing shader " << shaderList[shaderID].shaderName << std::endl;
		OutputDebugString(ss.str().c_str());
*/
		R_ReleaseVertexShader(shaderList[shaderID]);

		this->shaderList[shaderID].isValid = false;
	}
}

// set the shader as active (eg SetVertexShader() in D3D)
bool VertexShaderPool::SetActive(shaderID_t shaderID)
{
	if (this->currentSetShaderID == shaderID) // already set
	{
		return true;
	}

	if (R_SetVertexShader(shaderList[shaderID]))
	{
		currentSetShaderID = shaderID;
		return true;
	}

	return false;
}

// pass a shader name in and see if it exists in the list. If it does, return
// the position ID of that shader. Otherwise, return nullID to signify it's not
// in the list (so we can then add it)
shaderID_t VertexShaderPool::GetShaderByName(const std::string &shaderName)
{
	for (uint32_t i = 0; i < this->shaderList.size(); i++)
	{
		if (this->shaderList[i].shaderName == shaderName)
		{
			return i;
		}
	}

	return kNullShaderID;
}

bool VertexShaderPool::SetVertexShaderConstant(shaderID_t shaderID, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData)
{
	return R_SetVertexShaderConstant(shaderList[shaderID], registerIndex, type, constantData);
}


// add an already created pixelShader_t struct to our pool.
shaderID_t PixelShaderPool::Add(r_PixelShader &newShader)
{
	// see if it already exists, and return the ID to it if it does
	shaderID_t shaderID = GetShaderByName(newShader.shaderName);

	if (shaderID != kNullShaderID)
	{
		return shaderID;
	}

	// it's not in the list, add it
	this->shaderList.push_back(newShader);

	// it's at the end, so our ID is the last array position
	shaderID = this->shaderList.size() - 1;

	return shaderID;
}

// remove the shader at ID position passed in
void PixelShaderPool::Remove(shaderID_t shaderID)
{
/*
	std::stringstream ss;
	ss << "about to remove pixel shader " << shaderList[shaderID].shaderName << " with id " << shaderID << " and refcount " << shaderList[shaderID].refCount << std::endl;
	OutputDebugString(ss.str().c_str());
*/
	shaderList[shaderID].refCount--;
	if (shaderList[shaderID].refCount <= 0)
	{
/*
		ss.str("");
		ss << "removing shader " << shaderList[shaderID].shaderName << std::endl;
		OutputDebugString(ss.str().c_str());
*/
		R_ReleasePixelShader(shaderList[shaderID]);

		this->shaderList[shaderID].isValid = false;
	}
}

// set the shader as active (eg SetPixelShader() in D3D)
bool PixelShaderPool::SetActive(shaderID_t shaderID)
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
shaderID_t PixelShaderPool::GetShaderByName(const std::string &shaderName)
{
	for (uint32_t i = 0; i < this->shaderList.size(); i++)
	{
		if (this->shaderList[i].shaderName == shaderName)
		{
			return i;
		}
	}

	return kNullShaderID;
}


// Effect Manager
EffectManager::EffectManager()
{
}

EffectManager::~EffectManager()
{
	// clear the list
	for (uint32_t i = 0; i < effectList.size(); i++)
	{
		Remove(i);
	}
}

bool EffectManager::SetActive(effectID_t effectID)
{
	vsPool.SetActive(effectList[effectID].vertexShaderID);
	psPool.SetActive(effectList[effectID].pixelShaderID);
	return true;
}

bool EffectManager::SetVertexShaderConstant(effectID_t effectID, uint32_t registerIndex, enum SHADER_CONSTANT type, const void *constantData)
{
	return vsPool.SetVertexShaderConstant(effectList[effectID].vertexShaderID, registerIndex, type, constantData);
}

void EffectManager::Remove(effectID_t effectID)
{
	// remove VS if required
	vsPool.Remove(effectList[effectID].vertexShaderID);

	// remove PS if required
	psPool.Remove(effectList[effectID].pixelShaderID);

	// remove effect?
}

effectID_t EffectManager::Add(const std::string &effectName, const std::string &vertexShaderName, const std::string &pixelShaderName, VertexDeclaration *vertexDeclaration)
{
	shaderID_t vertexID = kNullShaderID;
	shaderID_t pixelID  = kNullShaderID;
	effectID_t effectID = kNullEffectID;

	// see if we already have an effect with that name
	for (uint32_t i = 0; i < this->effectList.size(); i++)
	{
		if ((effectList[i].effectName == effectName) && effectList[i].isValid)
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

	// load a vertex shader if required
	if (vertexID == kNullShaderID) // we have no slot or we don't already exist
	{
		r_VertexShader newVertexShader;
		newVertexShader.isValid  = false;
		newVertexShader.refCount = 0;

		if (R_CreateVertexShader(vertexShaderName, newVertexShader, vertexDeclaration))
		{
			// ok, store it
			newVertexShader.isValid = true;
			newVertexShader.shaderName = vertexShaderName;

			vertexID = vsPool.Add(newVertexShader);
		}
	}

	// see if the pixel shader is already loaded.
	pixelID = psPool.GetShaderByName(pixelShaderName);

	// load the pixel shader if required
	if (pixelID == kNullShaderID) // we have no slot or we don't already exist
	{
		r_PixelShader newPixelShader;
		newPixelShader.isValid  = false;
		newPixelShader.refCount = 0;

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
	newEffect.isValid        = true;
	newEffect.effectName     = effectName;
	newEffect.vertexShaderID = vertexID;
	newEffect.pixelShaderID  = pixelID;

	// before we add it, increment reference counters
	vsPool.AddRef(vertexID);
	psPool.AddRef(pixelID);

	// do we already have an effect slot?
	if (effectID != kNullShaderID)
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
	return kNullShaderID;
}
