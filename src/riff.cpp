#ifndef __RIFF_CPP__
#define __RIFF_CPP__

#include "riff.hpp"


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

int RIFFFile::open (const char* __filename, const char * __mode, size_t __size) {
    auto buffer = std::string(__mode);
    {
        bool hasB = 0;
        for (auto &i : buffer) {if (i == 0x62) hasB = 1;}
        if (!hasB) buffer+="b";
    }
    file = std::fopen(buffer.c_str(), __mode);
    type = C_FILE;
    return riff_open_file(rh, (std::FILE *)file, __size);
}

int RIFFFile::open (std::FILE & __file, size_t __size) {
    file = &__file;
    type = C_FILE|MANUAL;
    return riff_open_file(rh, &__file, __size);
}

#pragma endregion

#pragma region openMem 

int RIFFFile::open (const void * __mem_ptr, size_t __size) {
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

int RIFFFile::open(const char * __filename, std::ios_base::openmode __mode, size_t __size) {
    // Set type
    setAutomaticfstream();
    ((std::fstream*)file)->open(__filename, __mode|std::ios_base::binary);
    return openFstreamCommon();
}

int RIFFFile::open(const std::string & __filename, std::ios_base::openmode __mode, size_t __size) {
    // Set type
    setAutomaticfstream();
    ((std::fstream*)file)->open(__filename, __mode|std::ios_base::binary);
    return openFstreamCommon();
}

#if RIFF_CXX17_SUPPORT
int RIFFFile::open(const std::filesystem::path & __filename, std::ios_base::openmode __mode, size_t __size) {
    // Set type
    setAutomaticfstream();
    ((std::fstream*)file)->open(__filename, __mode|std::ios_base::binary);
    return openFstreamCommon();
}
#endif

void RIFFFile::setAutomaticfstream(){
    type = FSTREAM;
    file = new std::fstream;
}

int RIFFFile::open(std::fstream & __file, size_t __size){
    type = FSTREAM|MANUAL;
    file = &__file;
    return openFstreamCommon();
}

int RIFFFile::openFstreamCommon(){
    auto stream = (std::fstream*)file;
    // My own open function lmfao
    if(rh == NULL)
		return RIFF_ERROR_INVALID_HANDLE;
	rh->fh = file;
	rh->size = 0;
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