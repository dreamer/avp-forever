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
#include "utilities.h"
#include "console.h"
#include <algorithm>
#include "MurmurHash3.h" 

// Fast file signature
const int RFFL = 'RFFL';

const int hashTableSize = 2600;

// hash table to store fast file entries
std::vector<std::vector<FastFileHandle> > fastFileHandles;

// store the names of all .fll files available
std::vector<std::string> fastFiles;
static const std::string directoryName = "fastfile/";

// the size of the main fast file header, consisting of DWORDS (signature, version, nFiles, nHeaderBytes, nDataBytes)
const int kMainHeaderSize = 20;

// total number of files found within all fast files
static uint32_t fileCount = 0;

static bool isInited = false;

static bool FF_Open(const std::string &fastFileName, int32_t fastFileIndex);

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

	// hash the file name
	uint32_t hash;
	MurmurHash3_x86_32(lowercaseFileName.c_str(), lowercaseFileName.length(), 0, &hash);

	// mod with table size
	hash = hash % hashTableSize;

	int32_t theSize = fastFileHandles[hash].size();

	// no entry in hash table for this file
	if (theSize == 0) {
		return NULL;
	}
	else
	{
		/* 
		 * We use separate chaining to resolve collisions.
		 * The first entry at a hash location won't contain the file's name as a string. If there's a collision
		 * and a new entry is added to the location, we *do* add the file name for that entry. So, search backwards 
		 * through the chain, checking for a matching string. If we've reached entry 0, we have no string to check.
		 * So, this is a possible point of failure in the system. 
		 */
		for (int i = (theSize-1); i >= 0; i--)
		{
			if (i == 0) { // no string to check as first entry, just return it and assume it's ok.
				return &fastFileHandles[hash][i];
			}

			if (fastFileHandles[hash][i].fileName == lowercaseFileName)
			{
				return &fastFileHandles[hash][i];
			}
		}

		// shouldn't get here?
		return NULL;
	}
}

const std::string & FF_GetFastFileName(int32_t fastFileIndex)
{
	return fastFiles[fastFileIndex];
}

// initialise the fast file system. searches the current folder for all .FFL files and parses their
// contents for all files available within.
// TODO: should be passing a string containing the folder to search within, eg standard AvP folder "fastfile" ?
bool FF_Init()
{
	Con_PrintMessage("Initialising Fast File system...");

	fastFileHandles.resize(hashTableSize);

	if (FindFiles(directoryName + "*.FFL", fastFiles))
	{
		// common.ffl isn't a real fast file, so lets remove it
		if (fastFiles[0] == "common.ffl") {
			fastFiles.erase(fastFiles.begin());
		}

		for (size_t i = 0; i < fastFiles.size(); i++)
		{
			fastFiles[i] = directoryName + fastFiles[i];
			FF_Open(fastFiles[i], i);
		}

		// only set to true if some fast files were found and loaded
		isInited = true;

		Con_PrintMessage("Found " + Util::IntToString(fastFiles.size()) + " Fast Files, containing " + Util::IntToString(fileCount) + " files");

		return true;
	}

	Con_PrintMessage("No Fast Files found");

	// if we got here, we found no fast files to load
	return false;
}

// takes in the file name of a fast file, opens it and parses header information and adds all file entries to an std::map
static bool FF_Open(const std::string &fastFileName, int32_t fastFileIndex)
{
	FileStream file;
	file.Open(fastFileName, FileStream::FileRead, true);

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

	fileCount += nFiles;

	uint32_t nHeaderBytes = file.GetUint32LE(); // total bytes of file data header in fast file
	uint32_t nDataBytes   = file.GetUint32LE(); // we don't need this

	std::string requestedFile;

	for (uint32_t i = 0; i < nFiles; i++)
	{
		// read file header
		uint32_t dataOffset = file.GetUint32LE();
		uint32_t fileLength = file.GetUint32LE(); // size of the file data

		// read in the wav file name until we hit the null terminator
		while (file.PeekByte() != '\0')
		{
			requestedFile += file.GetByte();
		}

		// skip string null terminator
		file.GetByte();

		size_t stringLength = requestedFile.size();
		uint32_t alignedStringLength = (stringLength + 4) &~3;
		uint32_t bytesToSkip = alignedStringLength - stringLength - 1;

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
		newHandle.fastFileIndex = fastFileIndex;
		newHandle.fileOffset = realFileOffset;
		newHandle.fileSize   = fileLength;

		// make the whole filename string lowercase and change '\\' to '/'
		std::transform(requestedFile.begin(), requestedFile.end(), requestedFile.begin(), Util::LowercaseAndChangeSlash);

		// hash the file name
		uint32_t hash;
		MurmurHash3_x86_32(requestedFile.c_str(), requestedFile.length(), 0, &hash);

		// mod with table size
		hash = hash % hashTableSize;

		// if size > 0, we already have an item at this location, so we've got a collision
		if (fastFileHandles[hash].size())
		{
			// collision detected. store file name for collision resolution
			newHandle.fileName = requestedFile;
		}

		fastFileHandles[hash].push_back(newHandle);

		requestedFile.clear();
	}

	return true;
}
