// Copyright (C) 2011 Barry Duncan. All Rights Reserved.
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
// THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// DEVELOPERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "FileStream.h"
#include <string>
#include <vector>
#include <assert.h>

struct BitMapHeader
{
	uint16_t width;
	uint16_t height;                 // raster width & height in pixels
	int16_t  x, y;                   // pixel position for this image
	uint8_t  nPlanes;                // # source bitplanes
	uint8_t  masking;
	uint8_t  compression;
	uint8_t  flags;
	uint16_t transparentColor;       // transparent "color number" (sort of)
	uint8_t  xAspect, yAspect;       // pixel aspect, a ratio width : height
	int16_t  pageWidth, pageHeight;  // source "page" size in pixels
};

// size of BitMapHeader struct
const int kBMHDSize = 22;

// compression types
const int kCompressionNone     = 0;
const int kCompressionByteRun1 = 1;

// mask types
const int kMaskNone = 0;
const int kMask = 1;
const int kMaskTransparentColour = 2;
const int kMaskLasso = 3;

struct ColourRegister
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

// chunk IDs
const int LIST = 'LIST';
const int FORM = 'FORM';
const int CAT  = 'CAT ';

const int ILBM = 'ILBM';
const int BMHD = 'BMHD';
const int CMAP = 'CMAP';
const int BODY = 'BODY';
const int PROP = 'PROP';
const int TRAN = 'TRAN';

class RimLoader
{
	public:
		RimLoader::RimLoader();
		RimLoader::~RimLoader();
		bool Open(const std::string &fileName);
		bool Decode(uint8_t *dest, uint32_t destPitch);
		void GetDimensions(uint32_t &width, uint32_t &height);

	private:
		uint32_t GetUnpackedBytes();
		uint32_t GetBody();
		uint32_t GetScanLine();
		uint32_t GetUncompressedBytes();
		uint32_t GetRow();
		bool HandleBMHD();
		bool HandleCMAP();
		void OddChunkSizeCheck();

		FileStream file;

		uint32_t chunkSize;
		uint32_t chunkID;

		BitMapHeader header;
		ColourRegister *colourMap;
		uint8_t  *src;
		uint8_t  *destRowStart;
		uint32_t rowBytes;

		std::vector<uint8_t> destRow;
		uint32_t destRowIndex;

		// change this when code works
		uint8_t  *dest;
		uint32_t destPitch;
};
