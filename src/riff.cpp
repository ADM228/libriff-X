/*
	C++ wrapper for libriff

	(C) 2023-2025 alexmush
	License: zlib

	The C++ libriff wrapper is a memory-safe, OOP-compliant wrapper around the C-based libriff. It adds support for std::ifstream/ofstream, automatic allocation of filestreams (both C-based and if/ofstream) from filenames/filepaths, as well as some additional functions for reading/writing entire chunks at once and setting chunk types/IDs.
*/

#ifndef __RIFF_CPP__
#define __RIFF_CPP__

#include "riff.hpp"

namespace RIFF {

#pragma region condes

void * try_calloc(size_t nmemb, size_t size, const char * const objName) {
	int cnt = 16;
	void * ptr = nullptr;
	while (cnt != 0 && ptr == nullptr) {
		ptr = calloc(nmemb, size);
	}
	if (ptr == nullptr) {
		fprintf(stderr, "Could not allocate %s\n", objName);
	}

	return ptr;
}

RIFFHandle::RIFFHandle() {
	rh = riff_handleAllocate();
	#if !RIFF_CXX_PRINT_ERRORS
		rh->fp_printf = NULL;
	#endif
}

// copy assignment
RIFFHandle & RIFFHandle::operator = (const RIFFHandle &rhs) {
	if (&rhs == this)
		return *this;

	// Copy the riff_handle
	auto newrh = (riff_handle *)try_calloc(1, sizeof(riff_handle), "riff_handle, aborting copy assignment of RIFFHandle");
	if (newrh == nullptr) return *this;
	memcpy(rh, rhs.rh, sizeof(riff_handle));

	if (newrh->ls) {
		newrh->ls = (riff_levelStackEntry *)try_calloc(rh->ls_size, sizeof(riff_levelStackEntry), "riff level stack, aborting copy assignment of RIFFHandle");
		if (newrh->ls == nullptr) return *this;
		memcpy(newrh->ls, rhs.rh->ls, rh->ls_size * sizeof(riff_levelStackEntry));
	}

	if (rh) die();

	// Copy the data
	memcpy(this, &rhs, sizeof(RIFFHandle));
	rh = newrh;

	return *this;
}

// copy constructor
RIFFHandle::RIFFHandle(const RIFFHandle &rhs) {
	// Copy the data
	memcpy(this, &rhs, sizeof(RIFFHandle));

	// Copy the riff_handle
	rh = (riff_handle *)try_calloc(1, sizeof(riff_handle), "riff_handle, aborting copy assignment of RIFFHandle");
	if (rh == nullptr) return;
	memcpy(rh, rhs.rh, sizeof(riff_handle));

	if (rh->ls) {
		rh->ls = (riff_levelStackEntry *)try_calloc(rh->ls_size, sizeof(riff_levelStackEntry), "riff level stack, aborting copy assignment of RIFFHandle");
		if (rh->ls == nullptr) return;
		memcpy(rh->ls, rhs.rh->ls, rh->ls_size * sizeof(riff_levelStackEntry));
	}
}

// move assignment
RIFFHandle & RIFFHandle::operator = (RIFFHandle &&rhs) noexcept {
	if (&rhs == this)
		return *this;

	if (rh) die();

	memcpy(this, &rhs, sizeof(RIFFHandle));

	rhs.reset();

	return *this;
}

// move constructor
RIFFHandle::RIFFHandle (RIFFHandle &&rhs) noexcept {
	memcpy(this, &rhs, sizeof(RIFFHandle));

	rhs.reset();
}


RIFFHandle::~RIFFHandle() {
	die();
}

void RIFFHandle::die() {
	riff_handleFree(rh);
	close();
}

void RIFFHandle::reset() {
	// Reset the 4 internal variables
	file = nullptr;
	rh = nullptr;
	type = CLOSED;
	__latestError = RIFF_ERROR_NONE;
}

#pragma endregion

#pragma region openCfile

int RIFFHandle::openCFILE (const char* __filename, bool __detectSize) {
	file = std::fopen(__filename, "rb");
	FILE * __file = (FILE *)file;
	// Detect file size
	riff_ufs_t __size = 0;
	if (__detectSize) {
		std::fseek(__file, 0, SEEK_END);
		__size = std::ftell(__file);
		std::fseek(__file, 0, SEEK_SET);
	}
	type = C_FILE;
	return riff_open_file(rh, (std::FILE *)file, __size);
}

int RIFFHandle::openCFILE (std::FILE & __file, riff_ufs_t __size) {
	file = &__file;
	type = C_FILE|MANUAL;
	return riff_open_file(rh, &__file, __size);
}

#pragma endregion

#pragma region openMem 

int RIFFHandle::openMemory (const void * __mem_ptr, riff_ufs_t __size) {
	file = nullptr;
	type = MEM_PTR;
	return riff_open_mem(rh, __mem_ptr, __size);
}

#pragma endregion 

#pragma region fstreamHandling

size_t read_fstream(riff_handle *rh, void *ptr, size_t size){
	auto stream = ((std::fstream *)rh->fh);
	riff_ufs_t oldg = stream->tellg();
	stream->read((char *)ptr, size);
	riff_ufs_t newg = stream->tellg();
	return newg-oldg;
}

riff_ufs_t seek_fstream(riff_handle *rh, riff_ufs_t pos){
	auto stream = ((std::ifstream *)rh->fh);
	stream->seekg(pos);
	return stream->tellg();
}

int RIFFHandle::openFstream(const char * __filename, bool __detectSize) {
	// Set type
	setAutomaticFstream();
	auto & stream = *(std::fstream*)file;
	stream.open(__filename, std::ios_base::in|std::ios_base::binary);
	return openFstreamCommon(detectFstreamSize(__detectSize));
}

int RIFFHandle::openFstream(const std::string & __filename, bool __detectSize) {
	// Set type
	setAutomaticFstream();
	auto & stream = *(std::fstream*)file;
	stream.open(__filename, std::ios_base::in|std::ios_base::binary);
	return openFstreamCommon(detectFstreamSize(__detectSize));
}

#if RIFF_CXX17_SUPPORT
int RIFFHandle::openFstream(const std::filesystem::path & __filename, bool __detectSize) {
	// Set type
	setAutomaticFstream();
	auto & stream = *(std::fstream*)file;
	stream.open(__filename, std::ios_base::in|std::ios_base::binary);
	return openFstreamCommon(detectFstreamSize(__detectSize));
}
#endif

riff_ufs_t RIFFHandle::detectFstreamSize(bool __detectSize) {
	if (!__detectSize) return 0;

	auto & stream = *(std::fstream*)file;
	stream.seekg(0, std::ios_base::end);
	riff_ufs_t output = stream.tellg();
	stream.seekg(0, std::ios_base::beg);
	return output;
}

void RIFFHandle::setAutomaticFstream(){
	type = FSTREAM;
	file = new std::fstream;
}

int RIFFHandle::openFstream(std::fstream & __file, riff_ufs_t __size){
	type = FSTREAM|MANUAL;
	file = &__file;
	return openFstreamCommon(__size);
}

int RIFFHandle::openFstreamCommon(riff_ufs_t __size){
	auto stream = (std::fstream*)file;
	// My own open function lmfao
	if(rh == NULL)
		return RIFF_ERROR_INVALID_HANDLE;
	rh->fh = file;
	rh->size = __size;
	rh->cl_pos_start = stream->tellg(); //current file offset of stream considered as start of RIFF file
	
	rh->fp_read = &read_fstream;
	rh->fp_seek = &seek_fstream;
	
	return riff_readHeader(rh);
}

#pragma endregion

void RIFFHandle::close () {
	if (!(type & MANUAL)) { // Must be automatically allocated to close
		if (type == C_FILE) {
			std::fclose((std::FILE *)file);
			free (file);
		} else if (type == FSTREAM) {
			((std::fstream *)file)->close();
			free (file);
		}
	}
	type = CLOSED;
}

std::string RIFFHandle::latestErrorToString () {
	#define posStrSize 1+2+1+3+1+2+(2*sizeof(riff_ufs_t))+1

	if (__latestError == RIFF_ERROR_NONE) return "";

	std::string outString(riff_errorToString(__latestError));
	char buffer[posStrSize];
	std::snprintf(buffer, posStrSize, " at pos 0x%" __RIFF_FS_FMT "X", rh->pos);
	std::string posString (buffer);
	outString += posString;
	return outString;

	#undef posStrSize
}

std::vector<uint8_t> RIFFHandle::readChunkData() {
	__latestError = seekChunkStart(); 
	if (__latestError || rh->c_size == 0) {
		return std::vector<uint8_t>(0);
	}
	auto outVec = std::vector<uint8_t>(rh->c_size);
	riff_ufs_t totalSize = 0, succSize;
	do {
		succSize = readInChunk(outVec.data()+totalSize, rh->c_size);
		totalSize += succSize;
	} while (succSize != 0);
#if RIFF_CXX_PRINT_ERRORS
	if (totalSize != rh->c_size && rh->fp_printf) {
		rh->fp_printf("Couldn't read the entire chunk for some reason. Successfully read %zu bytes out of %" __RIFF_FS_FMT "\n", totalSize, rh->c_size);
	} 
#endif
	return outVec;
}

// This is reimplemented in C++ to capture the latest error
riff_sfs_t RIFFHandle::amountOfChunksInLevel () {
	if(rh == NULL) {
		__latestError = RIFF_ERROR_INVALID_HANDLE;
		return -1;
	}

	riff_sfs_t counter = 0;
	//seek to start of current list
	if ((__latestError = riff_seekLevelStart(rh)) != RIFF_ERROR_NONE)
		return -1;
	
	//seek all chunks of current list level
	while (__latestError == RIFF_ERROR_NONE){
		counter++;
		__latestError = riff_seekNextChunk(rh);
	}
	if (__latestError == RIFF_ERROR_EOCL){
		__latestError = RIFF_ERROR_NONE;
		// Just the end of the level
		return counter;
	} else if (__latestError == RIFF_ERROR_EXDAT){
		// Just the end of the level, but with some extra data - keep the error
		return counter;
	}
	// Otherwise, an error occured
	return -1;
};

// This is reimplemented in C++ to capture the latest error
riff_sfs_t RIFFHandle::amountOfChunksInLevelWithID (const char * id) {
	if(rh == NULL) {
		__latestError = RIFF_ERROR_INVALID_HANDLE;
		return -1;
	}

	riff_sfs_t counter = 0;
	//seek to start of current list
	if ((__latestError = riff_seekLevelStart(rh)) != RIFF_ERROR_NONE)
		return -1;
	
	//seek all chunks of current list level
	while (__latestError == RIFF_ERROR_NONE){
		if (!memcmp(rh->c_id, id, 4)) counter++;
		__latestError = riff_seekNextChunk(rh);
	}
	if (__latestError == RIFF_ERROR_EOCL){
		__latestError = RIFF_ERROR_NONE;
		// Just the end of the level
		return counter;
	} else if (__latestError == RIFF_ERROR_EXDAT){
		// Just the end of the level, but with some extra data - keep the error
		return counter;
	}
	// Otherwise, an error occured
	return -1;
};

}   // namespace RIFF

#endif  // __RIFF_CPP__