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

#include "RimLoader.h"
#include <algorithm>

// Constructor
RimLoader::RimLoader()
{
	chunkSize = 0;
	chunkID   = 0;

	colourMap = 0;
	destRowStart = 0;
	rowBytes = 0;
	destRowIndex = 0;

	// change this when code works
	dest = 0;
	destPitch = 0;
}

// Destructor
RimLoader::~RimLoader()
{
	delete[] colourMap;
}

void RimLoader::GetDimensions(uint32_t &width, uint32_t &height)
{
	width  = header.width;
	height = header.height;
}

bool RimLoader::Decode(uint8_t *dest, uint32_t destPitch)
{
	if (!dest) {
		return false;
	}

	this->dest = dest;
	this->destPitch = destPitch;

	GetBody();

	return true;
}

void RimLoader::OddChunkSizeCheck()
{
	if (chunkSize % 2 != 0)
	{
		// skip a byte
		file.GetByte();
	}
}

bool RimLoader::HandleBMHD()
{
	// read the rest of the header
	chunkSize = file.GetUint32BE();

	header.width  = file.GetUint16BE();
	header.height = file.GetUint16BE();

	header.x = file.GetUint16BE();
	header.y = file.GetUint16BE();
	header.nPlanes = file.GetByte();
	header.masking = file.GetByte();
	header.compression = file.GetByte();
	header.flags       = file.GetByte();
	header.transparentColor = file.GetUint16BE();
	header.xAspect    = file.GetByte();
	header.yAspect    = file.GetByte();
	header.pageWidth  = file.GetUint16BE();
	header.pageHeight = file.GetUint16BE();

	rowBytes = (header.width+7) / 8;

	assert(header.compression != 2); // COMPRESS_S3TC

	return true;
}

bool RimLoader::HandleCMAP()
{
	// read the colour map size
	chunkSize = file.GetUint32BE();

	uint32_t nEntries = chunkSize / 3;

	colourMap = new ColourRegister[nEntries];

	// read the values in
	for (uint32_t i = 0; i < nEntries; i++)
	{
		colourMap[i].red   = file.GetByte();
		colourMap[i].green = file.GetByte();
		colourMap[i].blue  = file.GetByte();
	}

	return true;
}

bool RimLoader::Open(const std::string &fileName)
{
	file.Open(fileName, FileStream::FileRead);
	if (!file.IsGood())
	{
		// unable to open file
		return false;
	}

	// we want to search to the BODY tag
	bool gotBody = false;

	while (!gotBody)
	{
		chunkID = file.GetUint32BE();

		switch (chunkID)
		{
			case LIST:
				chunkSize = file.GetUint32BE();
				// this should be first in the file! CHECKME
				break;
			case ILBM:
			{
				chunkID = file.GetUint32BE();
				switch (chunkID)
				{
					case PROP:
						chunkSize = file.GetUint32BE();
						break;
					case TRAN:
						// skip the tran data
						file.Seek(4+8, FileStream::SeekCurrent);
						break;
					case BMHD:
						HandleBMHD();
						OddChunkSizeCheck();
						break;
					default:
						// ERROR!
						return false;
				}
				break;
			}
			case FORM:
				chunkSize = file.GetUint32BE();
				break;
			case CMAP:
				HandleCMAP();
				OddChunkSizeCheck();
				break;
			case BODY:
				chunkSize = file.GetUint32BE();
				gotBody = true;
				break;
			default:
				// ERROR!
				return false;
		}
	}

	return true;
}

// do the byterun1 decompression here
uint32_t RimLoader::GetUnpackedBytes()
{
	uint32_t bytesDone = 0;
	uint32_t length    = 0;

	while (bytesDone < rowBytes)
	{
		// get the value we'll work with
		int8_t val = file.GetByte();

		if (val == -128)
		{
			// no op
		}
		else if (val >= 0)
		{
			length = val+1;

			length = (std::min)(length, rowBytes-bytesDone);

			// get next n bytes literally
			for (uint32_t i = 0; i < length; i++)
			{
				destRow[destRowIndex++] = file.GetByte();
			}
		}
		else if (val > -128)
		{
			// replicate the next byte -val+1 times
			length = (-val)+1;

			length = (std::min)(length, rowBytes-bytesDone);

			// get next byte and memset it to dest times length
			memset(&destRow[destRowIndex], file.GetByte(), length);
			destRowIndex += length;
		}

		bytesDone += length;
	}

	return bytesDone;
}

uint32_t RimLoader::GetBody()
{
	uint32_t bytesDone = 0;

	// destination row buffer
	destRow.resize(rowBytes * header.nPlanes);

	for (uint16_t y = 0; y < header.height; y++)
	{
		// reset
		destRowIndex = 0;

		bytesDone += GetScanLine();
	}

	return bytesDone;
}

// call once per scanline
uint32_t RimLoader::GetScanLine()
{
	uint32_t bytesDone = 0;

	for (uint8_t p = 0; p < header.nPlanes; p++)
	{
		bytesDone += GetRow();
	}

/*
	after parsing and uncompacting if necessary, you will have N planes of pixel data.  Color register used for
	each pixel is specified by looking at each pixel thru the planes.  I.e., if you have 5 planes, and the bit for
	a particular pixel is set in planes
	0 and 3:

		   PLANE     4 3 2 1 0
		   PIXEL     0 1 0 0 1

	then that pixel uses color register binary 01001 = 9
*/
	uint8_t *thisDest = dest;

	for (uint16_t x = 0; x < header.width; x++)
	{
		uint16_t pixel = 0;

		// 128, then 64, 32, 16 etc (and wrap back to 128)
		uint8_t bitMask = 128 >> (x % 8);

		// look through each plane
		for (uint8_t p = 0; p < header.nPlanes; p++)
		{
			uint8_t src = destRow[(x / 8) + (p * rowBytes)];

			if (src & bitMask)
			{
				// set the bit at position p to 'on'
				pixel |= 1 << p;
			}
		}

		ColourRegister temp = colourMap[pixel];

		// default alpha to 255
		uint8_t alpha = 255;

		if (header.masking == kMaskTransparentColour)
		{
			ColourRegister transColour = colourMap[header.transparentColor];
			if ((transColour.red   == temp.red)   &&
				(transColour.green == temp.green) &&
				(transColour.blue  == temp.blue))
			{
				alpha = 0;
			}
		}

		// write colour values to output buffer (in BGR order)
		(*thisDest++) = temp.blue;
		(*thisDest++) = temp.green;
		(*thisDest++) = temp.red;
		(*thisDest++) = alpha;
	}

	dest += destPitch;
	destRowIndex = 0;

	return bytesDone;
}

// this only exists so I can modify dest buffer without touching the original pointer (due to the copy)
// remove this and handle this properly
uint32_t RimLoader::GetUncompressedBytes()
{
	for (uint32_t i = 0; i < rowBytes; i++)
	{
		destRow[destRowIndex++] = file.GetByte();
	}

	return rowBytes;
}

// called for each bit plane
uint32_t RimLoader::GetRow()
{
	if (!header.compression)
	{
		return GetUncompressedBytes();
	}
	else
	{
		return GetUnpackedBytes();
	}
}
