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

typedef uint32_t effectID_t;
typedef uint32_t shaderID_t;

struct vertexShader_t;
struct pixelShader_t;

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
		std::vector<effect_t>	effectList;
		std::vector<vertexShader_t> vertexShaderList;
		std::vector<pixelShader_t>   pixelShaderList;

	public:
		EffectManager();
		effectID_t AddEffect(const std::string &effectName, const std::string &vertexShaderName, const std::string &pixelShaderName);
		bool Set(effectID_t effectID);
};

// keep these separate
struct vertexShader_t
{
	bool 			isValid;
	uint32_t		refCount;
	r_VertexShader	vertexShader;
	LPD3DXCONSTANTTABLE	constantTable;
	std::string 	vertexShaderName;
};

struct pixelShader_t
{
	bool			isValid;
	uint32_t		refCount;
	r_PixelShader	pixelShader;
	std::string 	pixelShaderName;
};

#endif
