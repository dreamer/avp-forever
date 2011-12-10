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

#ifndef _FileStream_h_
#define _FileStream_h_

// open - read and write mode (text or just binary?)

// read and write functions

// search directories?

// handle SaveFolderPath for win32?

// append "D:/" for xbox?

// delete file?

#ifdef _WIN32
	#define NOMINMAX
	#include <windows.h>
#endif

#include <string>
#include <vector>
#include <stdint.h>

class FileStream
{
	public:
		
		enum eAccess
		{
			FileRead,
			FileWrite,
			FileReadWrite
		};

		enum eSeek
		{
			SeekCurrent,
			SeekStart,
			SeekEnd
		};

		FileStream()
		{
			fileHandle = 0;
			isGood = false;
			fromFastFile = false;
			fileOffset = 0;
			fileSize = 0;
		}
		~FileStream();

		// pass true for skipFastFileCheck to skip fast file check, for accessing files we know aren't in a fast file
		bool FileStream::Open(const std::string &fileName, eAccess accessType, bool skipFastFileCheck = false);
		void Close();
		bool IsGood();

		DWORD ReadBytes(uint8_t *buffer, size_t nBytes);
		bool WriteBytes(const uint8_t *buffer, size_t nBytes);

		uint8_t  GetByte();
		uint16_t GetUint16LE();
		uint16_t GetUint16BE();
		uint32_t GetUint32LE();
		uint32_t GetUint32BE();
		uint64_t GetUint64LE();
		uint64_t GetUint64BE();

		uint8_t PeekByte();

		void PutByte(uint8_t value);
		void PutUint16LE(uint16_t value);
		void PutUint16BE(uint16_t value);
		void PutUint32LE(uint32_t value);
		void PutUint32BE(uint32_t value);
		void PutUint64LE(uint64_t value);
		void PutUint64BE(uint64_t value);

		bool WriteString(const std::string &theString);
		bool WriteString(const char *theString);
		uint32_t GetFileSize();
		bool Seek(int64_t offset, eSeek seekType);
		bool SkipBytes(int64_t nBytes);
		int64_t FileStream::GetCurrentPos();

	private:
		HANDLE fileHandle;
		bool isGood;

		// fast file specific stuff
		bool fromFastFile;
		uint32_t fileOffset;
		uint32_t fileSize;
};

bool DoesFileExist(const std::string &fileName);
bool FindFiles(const std::string &fileName, std::vector<std::string> &filesFound);

#endif
