#ifndef __RIFF_CPP__
#define __RIFF_CPP__

#include "riff.hpp"
#include <ios>


namespace RIFF {

#pragma region condes

RIFFFile::RIFFFile() {
    rh = riff_handleAllocate();
    #if !RIFF_CXX_PRINT_ERRORS
        rh->fp_printf = NULL;
    #endif
}

RIFFFile::~RIFFFile() {
    riff_handleFree(rh);
    close();
}

#pragma endregion

#pragma region openCfile

int RIFFFile::openCFILE (const char* __filename, bool __detectSize) {
    file = std::fopen(__filename, "rb");
    FILE * __file = (FILE *)file;
    // Detect file size
    size_t __size = 0;
    if (__detectSize) {
        std::fseek(__file, 0, SEEK_END);
        __size = std::ftell(__file);
        std::fseek(__file, 0, SEEK_SET);
    }
    type = C_FILE;
    return riff_open_file(rh, (std::FILE *)file, __size);
}

int RIFFFile::openCFILE (std::FILE & __file, size_t __size) {
    file = &__file;
    type = C_FILE|MANUAL;
    return riff_open_file(rh, &__file, __size);
}

#pragma endregion

#pragma region openMem 

int RIFFFile::openMemory (const void * __mem_ptr, size_t __size) {
    file = nullptr;
    type = MEM_PTR;
    return riff_open_mem(rh, __mem_ptr, __size);
}

#pragma endregion 

#pragma region fstreamHandling

size_t read_fstream(riff_handle *rh, void *ptr, size_t size){
    auto stream = ((std::fstream *)rh->fh);
    size_t oldg = stream->tellg();
    stream->read((char *)ptr, size);
    size_t newg = stream->tellg();
    return newg-oldg;
}

size_t seek_fstream(riff_handle *rh, size_t pos){
    auto stream = ((std::ifstream *)rh->fh);
    stream->seekg(pos);
	return stream->tellg();
}

int RIFFFile::openFstream(const char * __filename, bool __detectSize) {
    // Set type
    setAutomaticFstream();
    auto & stream = *(std::fstream*)file;
    stream.open(__filename, std::ios_base::in|std::ios_base::binary);
    return openFstreamCommon(detectFstreamSize(__detectSize));
}

int RIFFFile::openFstream(const std::string & __filename, bool __detectSize) {
    // Set type
    setAutomaticFstream();
    auto & stream = *(std::fstream*)file;
    stream.open(__filename, std::ios_base::in|std::ios_base::binary);
    return openFstreamCommon(detectFstreamSize(__detectSize));
}

#if RIFF_CXX17_SUPPORT
int RIFFFile::openFstream(const std::filesystem::path & __filename, bool __detectSize) {
    // Set type
    setAutomaticFstream();
    auto & stream = *(std::fstream*)file;
    stream.open(__filename, std::ios_base::in|std::ios_base::binary);
    return openFstreamCommon(detectFstreamSize(__detectSize));
}
#endif

size_t RIFFFile::detectFstreamSize(bool __detectSize) {
    if (!__detectSize) return 0;

    auto & stream = *(std::fstream*)file;
    stream.seekg(0, std::ios_base::end);
    size_t output = stream.tellg();
    stream.seekg(0, std::ios_base::beg);
    return output;
}

void RIFFFile::setAutomaticFstream(){
    type = FSTREAM;
    file = new std::fstream;
}

int RIFFFile::openFstream(std::fstream & __file, size_t __size){
    type = FSTREAM|MANUAL;
    file = &__file;
    return openFstreamCommon(__size);
}

int RIFFFile::openFstreamCommon(size_t __size){
    auto stream = (std::fstream*)file;
    // My own open function lmfao
    if(rh == NULL)
		return RIFF_ERROR_INVALID_HANDLE;
	rh->fh = file;
	rh->size = __size;
	rh->pos_start = stream->tellg(); //current file offset of stream considered as start of RIFF file
	
	rh->fp_read = &read_fstream;
	rh->fp_seek = &seek_fstream;
	
	return riff_readHeader(rh);
}

#pragma endregion

void RIFFFile::close () {
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

std::string RIFFFile::errorToString (int errorCode) {
    #define posStrSize 2+1+3+1+2+(2*sizeof(size_t))+1

    std::string outString(riff_errorToString(errorCode));
    char buffer[posStrSize];
    std::snprintf(buffer, posStrSize, "at pos 0x%zX", rh->pos);
    std::string posString (buffer);
    outString += posString;
    return outString;

    #undef posStrSize
}

std::vector<uint8_t> RIFFFile::readChunkData() {
    int errCode;
    errCode = seekChunkStart(); 
    if (errCode || rh->c_size == 0) {
        return std::vector<uint8_t>(0);
    }
    auto outVec = std::vector<uint8_t>(rh->c_size);
    size_t totalSize = 0, succSize;
    do {
        succSize = readInChunk(outVec.data()+totalSize, rh->c_size);
        totalSize += succSize;
    } while (succSize != 0);
#if RIFF_CXX_PRINT_ERRORS
    if (totalSize != rh->c_size && rh->fp_printf) {
        rh->fp_printf("Couldn't read the entire chunk for some reason. Successfully read %zu bytes out of %zu\n", totalSize, rh->c_size);
    } 
#endif
    return outVec;
}

}   // namespace RIFF

#endif  // __RIFF_CPP__