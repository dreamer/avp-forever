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

#include "FastFile.h"
#include "FileStream.h"
#include <algorithm>

fastFileHandles fastFileMap;

static bool isInited = false;

// the size of the main fast file header, consisting of DWORDS (signature, version, nFiles, nHeaderBytes, nDataBytes)
const int kMainHeaderSize = 20;

static bool FF_Open(const std::string &fastFileName);

// pass this function the name of a file within a fast file you want to find (eg comp1.wav) and it'll return
// a handle to the file if it exists in a known fast file, or a null handle if file doesn't exist in a known fast file,
// or also null if the fast file system hasn't been initialised
FastFileHandle* FF_Find(const std::string &fileName)
{
	if (!isInited)
		return NULL;

	// there can be many variations on the case of a filename (either hardcoded in code or in a RIF file, so we always force our filenames to lowercase
	// before we store them in our file list, and then lowercase our serch filename so we'll get a match regardless of the cases.
	// e.g. so 'TEST.RIM' gets changed to 'test.rim' will match our stored 'test.rim' in the file list map but might actually have been "Test.Rim"
	std::string lowercaseFileName = fileName;
	std::transform(lowercaseFileName.begin(), lowercaseFileName.end(), lowercaseFileName.begin(), tolower);

	fastFileHandles::iterator ffIt = fastFileMap.find(lowercaseFileName);

	if (ffIt == fastFileMap.end())
	{
		// the requested file wasn't found within our fast files
		return NULL;
	}
	else
	{
		// we found the file. return a pointer to the fast file handle
		return &fastFileMap[lowercaseFileName];
	}
}

// initialise the fast file system. searches the current folder for all .FFL files and parses their
// contents for all files available within.
// TODO: should be passing a string containing the folder to search within, eg standard AvP folder "fastfile"
bool FF_Init()
{
	// search fast file folder for list of fast files
	std::vector<std::string> fastFiles;

	if (FindFiles("fastfile\\*.FFL", fastFiles))
	{
		for (size_t i = 0; i < fastFiles.size(); i++)
		{
			FF_Open("fastfile\\" + fastFiles[i]);
		}

		// only set to true if some fast files were found and loaded
		isInited = true;
		return true;
	}

	// if we got here, we found no fast files to load
	return false;
}

// takes in the file name of a fast file, opens it and parses header information and adds all file entries to an std::map
static bool FF_Open(const std::string &fastFileName)
{
	FileStream file;
	file.Open(fastFileName, FileStream::FileRead);

	if (!file.IsGood())
	{
		return false;
	}

	// read and check file signature
	uint32_t signature = file.GetUint32BE();
	if (signature != RFFL)
	{
		// report error
		return false;
	}

	uint32_t version = file.GetUint32LE();
	if (version > 0)
	{
		// we don't support versions other than 0 (and I don't think they actually exist..)
		return false;
	}

	// number of files available within the fast file
	uint32_t nFiles = file.GetUint32LE();

	uint32_t nHeaderBytes = file.GetUint32LE(); // total bytes of file data header in fast file
	uint32_t nDataBytes   = file.GetUint32LE(); // we don't need this

	char fileName[MAX_PATH];

	for (uint32_t i = 0; i < nFiles; i++)
	{
		// read file header
		uint32_t dataOffset = file.GetUint32LE();
		uint32_t fileLength = file.GetUint32LE(); // size of the file data

		// read the file name
		fileName[0] = '\0';

		bool gotTerminator = false;

		uint8_t *destChar = (uint8_t*)&fileName[0];

		while (!gotTerminator)
		{
			file.ReadBytes(destChar, 1);
			if (*destChar == '\0')
			{
				gotTerminator = true;
				break;
			}

			destChar++;
		}

		// strings are 4 byte aligned in file. determine how much pad to skip vs what we actually read
		uint32_t alignedStringLength = (strlen(fileName) + 4) &~3;
		uint32_t bytesToSkip = alignedStringLength-strlen(fileName)-1;

		// skip the 4 byte align pad bytes
		while (bytesToSkip)
		{
			file.GetByte();
			bytesToSkip--;
		}

		// the actual data offset - skip past main header plus total bytes for all file headers
		uint32_t realFileOffset = kMainHeaderSize + nHeaderBytes + dataOffset;

		// add file handle to map
		FastFileHandle newHandle;
		newHandle.sourceFile = fastFileName;
		newHandle.fileOffset = realFileOffset;
		newHandle.fileSize   = fileLength;

		// make the whole filename string lowercase
		std::string requestedFile = fileName;
		std::transform(requestedFile.begin(), requestedFile.end(), requestedFile.begin(), tolower);

		fastFileMap.insert(std::make_pair(requestedFile, newHandle));
	}

	return true;
}
