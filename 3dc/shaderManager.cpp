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

void VertexShader::Release()
{
	refCount--;
	if (!refCount)
	{
		R_ReleaseVertexShader(vertexShader);
	}
}

bool VertexShader::SetInt(const char* constant, uint32_t n)
{
	return true;
}

bool VertexShader::SetMatrix(const char* constant, struct R_MATRIX &matrix)
{
	return true;
}

EffectManager::EffectManager()
{
}

bool EffectManager::Set(effectID_t effectID)
{
	R_SetVertexShader(this->vertexShaderList[this->effectList[effectID].vertexShaderID]);
	R_SetPixelShader(this->pixelShaderList[this->effectList[effectID].pixelShaderID]);
	return true;
}

/*
void EffectManager::Release(effectID_t effectID)
{
	// release vertex shader if we're the only one with a reference to it
	vertexShaderList[effectList[effectID].vertexShaderID].refCount--;

	if (vertexShaderList[effectList[effectID].vertexShaderID].refCount)
	{
		
	}
}

void EffectManager::Release(effectID_t effectID)
{
	vertexShaderList[effectList[effectID].vertexShaderID].refCount--;
	if (!vertexShaderList[effectList[effectID].vertexShaderID].refCount)
	{
		R_ReleaseVertexShader(vertexShaderList[effectList[effectID].vertexShaderID]);
	}
}
*/

effectID_t EffectManager::AddEffect(const std::string &effectName, const std::string &vertexShaderName, const std::string &pixelShaderName)
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
	for (uint32_t i = 0; i < this->vertexShaderList.size(); i++)
	{
		if (vertexShaderList[i].vertexShaderName == vertexShaderName && vertexShaderList[i].isValid)
		{
			// no need to report message, just grab the id
			vertexID = i;
			break;
		}

		// while we're here, grab a free ID
		if (vertexShaderList[i].isValid == false)
		{
			vertexID = i;
		}
	}

	// see if the pixel shader is already loaded.
	for (uint32_t i = 0; i < this->pixelShaderList.size(); i++)
	{
		if (pixelShaderList[i].pixelShaderName == pixelShaderName && pixelShaderList[i].isValid)
		{
			// no need to report message, just grab the id
			pixelID = i;
			break;
		}

		// while we're here, grab a free ID
		if (pixelShaderList[i].isValid == false)
		{
			pixelID = i;
		}
	}

	// load a vertex shader if required
	if (vertexID != nullID) // we have a slot
	{
		if (vertexShaderList[vertexID].isValid == false) // but its not valid (loaded) so lets load the vertex shader
		{
			vertexShader_t newVertexShader;
			newVertexShader.isValid = false;

			if (R_CreateVertexShader(vertexShaderName, newVertexShader))
			{
				// ok, store it
				newVertexShader.isValid = true;
				newVertexShader.vertexShaderName = vertexShaderName;

				// we have a lot, so reuse
				vertexShaderList[vertexID] = newVertexShader;
			}
		}
	}
	else if (vertexID == nullID) // we have no slot
	{
		vertexShader_t newVertexShader;
		newVertexShader.isValid = false;

		if (R_CreateVertexShader(vertexShaderName, newVertexShader))
		{
			// ok, store it
			newVertexShader.isValid = true;
			newVertexShader.vertexShaderName = vertexShaderName;

			// we have no slot, so add to back of list
			vertexShaderList.push_back(newVertexShader);

			// we need to record the slot
			vertexID = vertexShaderList.size() - 1;
		}
	}

	// load the pixel shader if required
	if (pixelID != nullID) // we have a slot
	{
		if (pixelShaderList[pixelID].isValid == false) // but its not valid (loaded) so lets load the pixel shader
		{
			pixelShader_t newPixelShader;
			newPixelShader.isValid = false;

			if (R_CreatePixelShader(pixelShaderName, newPixelShader))
			{
				// ok, store it
				newPixelShader.isValid = true;
				newPixelShader.pixelShaderName = pixelShaderName;

				// we have a lot, so reuse
				pixelShaderList[pixelID] = newPixelShader;
			}
		}
	}
	else if (pixelID == nullID) // we have no slot
	{
		pixelShader_t newPixelShader;
		newPixelShader.isValid = false;

		if (R_CreatePixelShader(pixelShaderName, newPixelShader))
		{
			// ok, store it
			newPixelShader.isValid = true;
			newPixelShader.pixelShaderName = pixelShaderName;

			// we have no slot, so add to back of list
			pixelShaderList.push_back(newPixelShader);

			// we need to record the slot
			pixelID = pixelShaderList.size() - 1;
		}
	}

	// now the effect itself
	effect_t newEffect;
	newEffect.isValid = true;
	newEffect.effectName = effectName;
	newEffect.vertexShaderID = vertexID;
	newEffect.pixelShaderID = pixelID;

	// before we add it, increment reference counters
	vertexShaderList[vertexID].refCount++;
	pixelShaderList[pixelID].refCount++;

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











#if 0

class Shader
{
	private:
		uint32_t GetFreeVertexID();
		uint32_t GetFreePixelID();

	public:
		uint32_t CreateVertexShader(const std::string &fileName);
		uint32_t CreatePixelShader(const std::string &fileName);
		bool SetVertexShader(shaderID_t shaderID);
		bool SetPixelShader(shaderID_t shaderID);
};

// vertex shaders ===================================================================================================

uint32_t Shader::GetFreeVertexID()
{
	for (uint32_t i = 0; i < vertexShaderList.size(); i++)
	{
		if (vertexShaderList[i].isValid == false)
		{
			return i;
		}
	}

	// no free slot, add to the end of the list
	return static_cast<uint32_t> (vertexShaderList.size());
}

uint32_t Shader::CreateVertexShader(const std::string &fileName)
{
	shaderID_t shaderID = GetFreeVertexID();
	vertexShader_t	newVertexShader;
	newVertexShader.isValid = false;

	// ask api backend to load the vertex shader
	if (!R_CreateVertexShader(fileName, newVertexShader))
	{
		Con_PrintError("Can't load vertex shader");
		return nullID;
	}

	// we're ok, add to the array
	vertexShaderList[shaderID] = newVertexShader;

	// set shader status as valid
	newVertexShader.isValid = true;

	return shaderID;
}

bool Shader::SetVertexShader(shaderID_t shaderID)
{
	 return R_SetVertexShader(vertexShaderList[shaderID]);
}


// pixel shaders ===================================================================================================

uint32_t Shader::GetFreePixelID()
{
	for (uint32_t i = 0; i < pixelShaderList.size(); i++)
	{
		if (pixelShaderList[i].isValid == false)
		{
			return i;
		}
	}

	// no free slot, add to the end of the list
	return static_cast<uint32_t> (pixelShaderList.size());
}

uint32_t Shader::CreatePixelShader(const std::string &fileName)
{
	shaderID_t shaderID = GetFreePixelID();
	pixelShader_t	newPixelShader;
	newPixelShader.isValid = false;

	// ask api backend to load the vertex shader
	if (!R_CreatePixelShader(fileName, newPixelShader))
	{
		Con_PrintError("Can't load pixel shader");
		return nullID;
	}

	// we're ok, add to the array
	pixelShaderList[shaderID] = newPixelShader;

	// set shader status as valid
	newPixelShader.isValid = true;

	return shaderID;
}

bool Shader::SetPixelShader(shaderID_t shaderID)
{
	 return R_SetPixelShader(pixelShaderList[shaderID]);
}
#endif