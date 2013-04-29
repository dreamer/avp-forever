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

#include "FileStream.h"
#include "FastFile.h"
#include "utilities.h"

template <class T>
T swapBytes(T a) {
	
	T result;
	int32_t nBytes = sizeof(a);

	uint8_t *src  = reinterpret_cast<uint8_t*>(&a) + (nBytes - 1);
	uint8_t *dest = reinterpret_cast<uint8_t*>(&result);

	while (nBytes) {
		*dest++ = *src--;
		nBytes--;
	}

	return result;
}

FileStream::~FileStream()
{
	Close();
}

bool FileStream::IsGood()
{
	return isGood;
}

bool FileStream::Open(const std::string &fileName, eAccess accessType, eFastFileCheck skipFastFileCheck)
{
	// TODO - write this properly. Also, add write append mode
	std::string realFileName;

	// handle write mode
	if (accessType == FileStream::FileWrite) {
		if (!OpenActual(fileName, accessType)) {
			fileHandle = 0;
			return false;
		}

		isGood = true;
		return true;
	}

	// check for local version first
	if (DoesFileExist(fileName)) {
		realFileName = fileName;
	}
	else
	{
		// we haven't found a local copy of the file. Do we do the fast file check?
		if (SkipFastFileCheck == skipFastFileCheck) {
			return false;
		}

		// try find the file in a fast file archive
		FastFileHandle *ffHandle = FF_Find(fileName);
		if (ffHandle)
		{
			fromFastFile = true;
			fileOffset = ffHandle->fileOffset;
			fileSize   = ffHandle->fileSize;

			// a .FFL file name
			realFileName = FF_GetFastFileName(ffHandle->fastFileIndex);
		}
		else {
			// no local or fast file copy of the file exists
			return false;
		}
	}

	// open the .ffl archive containing the file we want to access
	if (!OpenActual(realFileName, accessType)) {
		fileHandle = 0;
		return false;
	}

	if (fromFastFile) {
		// seek to the start of the file we want within the fast file archive
		Seek(fileOffset, SeekStart);
	}

	// set flag to indicate read/write is now ok to do
	isGood = true;
	
	return true;
}

bool FileStream::OpenActual(const std::string &fileName, eAccess accessType)
{
	DWORD desiredAccess;       // GENERIC_READ, GENERIC_WRITE, or both (GENERIC_READ | GENERIC_WRITE). 
	DWORD creationDisposition; // An action to take on a file or device that exists or does not exist.

	switch (accessType)
	{
		case FileRead:
			desiredAccess = GENERIC_READ;
			creationDisposition = OPEN_EXISTING;
			break;
		case FileWrite:
			desiredAccess = GENERIC_WRITE;
			creationDisposition = CREATE_ALWAYS;
			break;
		case FileReadWrite:
			desiredAccess = GENERIC_READ | GENERIC_WRITE;
			creationDisposition = CREATE_ALWAYS;
			break;
		default:
			return false;
			break;
	}

#ifdef _XBOX
	std::string tempString("D:\\");
	tempString += fileName;
	Util::FtoBslash(tempString);

	fileHandle = ::CreateFile(tempString.c_str(), desiredAccess, 0, NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);

#else
	// try open the file
	fileHandle = ::CreateFile(fileName.c_str(), desiredAccess, 0, NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
#endif

	if (INVALID_HANDLE_VALUE == fileHandle)
	{
		fileHandle = 0;
		return false;
	}

	return true;
}

void FileStream::Close()
{
	// avoid closing handle twice if user calls Close() and then destructor runs
	if (fileHandle)
	{
		CloseHandle(fileHandle);
		fileHandle = 0;
	}

	isGood = false;
}

int64_t FileStream::GetCurrentPos()
{
	LARGE_INTEGER theOffset;
	theOffset.QuadPart = 0;

	LARGE_INTEGER theOffset2;

	if (::SetFilePointerEx(fileHandle, theOffset, &theOffset2, FILE_CURRENT) == 0) {
		return -1;
	}

	return theOffset2.QuadPart;
}

bool FileStream::Seek(int64_t offset, eSeek seekType)
{
	DWORD moveMethod = FILE_CURRENT;

	switch (seekType)
	{
		case SeekCurrent:
			moveMethod = FILE_CURRENT;
			break;
		case SeekStart:
			moveMethod = FILE_BEGIN;
			break;
		case SeekEnd:
			moveMethod = FILE_END;
			break;
		default:
			return 0;
			break;
	}

	// TODO: check this is correct
	LARGE_INTEGER theOffset;
	theOffset.QuadPart = offset;

	if (::SetFilePointerEx(fileHandle, theOffset, 0, moveMethod) == 0) {
		return false;
	}

	return true;
}

bool FileStream::WriteBytes(const uint8_t *buffer, size_t nBytes)
{
	DWORD nBytesWritten = 0;

	if (::WriteFile(fileHandle, static_cast<LPCVOID>(buffer), nBytes, &nBytesWritten, NULL) == 0) {
		return false;
	}

	return true;
}

DWORD FileStream::ReadBytes(uint8_t *buffer, size_t nBytes)
{
	DWORD nBytesRead = 0;

	if (::ReadFile(fileHandle, static_cast<LPVOID>(buffer), nBytes, &nBytesRead, NULL) == 0) {
		return 0;
	}

	return nBytesRead;
}

// writes an STL string to the file
bool FileStream::WriteString(const std::string &theString)
{
	return WriteBytes(reinterpret_cast<const uint8_t*>(theString.c_str()), theString.size()); 
}

// writes a C style string to the file
bool FileStream::WriteString(const char *theString)
{
	return WriteBytes(reinterpret_cast<const uint8_t*>(theString), strlen(theString));
}

uint32_t FileStream::GetFileSize()
{
	if (fromFastFile) {
		return fileSize;
	}
	else {
		return ::GetFileSize(fileHandle, NULL);
	}
}

// Read
uint8_t FileStream::GetByte()
{
	uint8_t ret;
	ReadBytes(&ret, sizeof(ret));
	return ret;
}

uint16_t FileStream::GetUint16LE()
{
	uint16_t ret;
	ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof(ret));
	return ret;
}

uint16_t FileStream::GetUint16BE()
{
	uint16_t ret;
	ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof(ret));
//	return _byteswap_ushort(ret);
	return swapBytes<uint16_t>(ret);
}

int32_t FileStream::GetInt32LE()
{
	int32_t ret;
	ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof(ret));
	return ret;
}

uint32_t FileStream::GetUint32LE()
{
	uint32_t ret;
	ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof(ret));
	return ret;
}

uint32_t FileStream::GetUint32BE()
{
	uint32_t ret;
	ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof(ret));
//	return _byteswap_ulong(ret);
	return swapBytes<uint32_t>(ret);
}

uint64_t FileStream::GetUint64LE()
{
	uint64_t ret;
	ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof(ret));
	return ret;
}
/*
uint64_t FileStream::GetUint64BE()
{
	uint64_t ret;
	ReadBytes(reinterpret_cast<uint8_t*>(&ret), sizeof(ret));
	return _byteswap_uint64(ret);
}
*/
uint8_t FileStream::PeekByte()
{
	uint8_t ret;
	ReadBytes(&ret, sizeof(ret));
	Seek(-1, SeekCurrent);
	return ret;
}

bool FileStream::SkipBytes(int64_t nBytes)
{
	return Seek(nBytes, SeekCurrent);
}

// Write
void FileStream::PutByte(uint8_t value)
{
	WriteBytes(&value, sizeof(value));
}

void FileStream::PutUint16LE(uint16_t value)
{
	WriteBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
}

void FileStream::PutUint16BE(uint16_t value)
{
//	value = _byteswap_ushort(value);
	value = swapBytes<uint16_t>(value);
	WriteBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
}

void FileStream::PutUint32LE(uint32_t value)
{
	WriteBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
}

void FileStream::PutInt32LE(int32_t value)
{
	WriteBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
}

void FileStream::PutUint32BE(uint32_t value)
{
//	value = _byteswap_ulong(value);
	value = swapBytes<uint32_t>(value);
	WriteBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
}

void FileStream::PutUint64LE(uint64_t value)
{
	WriteBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
}

void FileStream::PutUint64BE(uint64_t value)
{
//	value = _byteswap_uint64(value);
	value = swapBytes<uint64_t>(value);
	WriteBytes(reinterpret_cast<uint8_t*>(&value), sizeof(value));
}

bool DoesFileExist(const std::string &fileName)
{
	DWORD fileAttributes = ::GetFileAttributes(fileName.c_str());

	if (0xffffffff == fileAttributes || FILE_ATTRIBUTE_DIRECTORY & fileAttributes) {
		return false;
	}

	return true;
}

bool FindFiles(const std::string &fileName, std::vector<std::string> &filesFound)
{
	WIN32_FIND_DATA findData;
	HANDLE searchHandle;

#ifdef _XBOX
	std::string tempString("D:\\");
	tempString += fileName;
	Util::FtoBslash(tempString);

	searchHandle = ::FindFirstFile(tempString.c_str(), &findData);

#else
	searchHandle = ::FindFirstFile(fileName.c_str(), &findData);
#endif

	if (INVALID_HANDLE_VALUE == searchHandle)
	{
		// no files found?
		if (ERROR_FILE_NOT_FOUND == GetLastError())
		{
			// report no files found
		}
		else
		{
			// report other error
		}

		return false;
	}

	// we found a file
	filesFound.push_back(findData.cFileName);

	// try find more files
	while (::FindNextFile(searchHandle, &findData))
	{
		filesFound.push_back(findData.cFileName);
	}

	return true;
}
