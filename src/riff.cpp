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

RIFFFile::RIFFFile() {
    rh = riff_handleAllocate();
    #if !RIFF_CXX_PRINT_ERRORS
        rh->fp_printf = NULL;
    #endif
}

// copy assignment
RIFFFile & RIFFFile::operator = (const RIFFFile &rhs) {
    if (&rhs == this)
		return *this;

    // Copy the riff_handle
    auto newrh = (riff_handle *)try_calloc(1, sizeof(riff_handle), "riff_handle, aborting copy assignment of RIFFFile");
    if (newrh == nullptr) return *this;
    memcpy(rh, rhs.rh, sizeof(riff_handle));

    if (newrh->ls) {
        newrh->ls = (struct riff_levelStackE *)try_calloc(rh->ls_size, sizeof(struct riff_levelStackE), "riff level stack, aborting copy assignment of RIFFFile");
        if (newrh->ls == nullptr) return *this;
        memcpy(newrh->ls, rhs.rh->ls, rh->ls_size * sizeof(struct riff_levelStackE));
    }

    if (rh) die();

    // Copy the data
    memcpy(this, &rhs, sizeof(RIFFFile));
    rh = newrh;

    return *this;
}

// copy constructor
RIFFFile::RIFFFile(const RIFFFile &rhs) {
    // Copy the data
    memcpy(this, &rhs, sizeof(RIFFFile));

    // Copy the riff_handle
    rh = (riff_handle *)try_calloc(1, sizeof(riff_handle), "riff_handle, aborting copy assignment of RIFFFile");
    if (rh == nullptr) return;
    memcpy(rh, rhs.rh, sizeof(riff_handle));

    if (rh->ls) {
        rh->ls = (struct riff_levelStackE *)try_calloc(rh->ls_size, sizeof(struct riff_levelStackE), "riff level stack, aborting copy assignment of RIFFFile");
        if (rh->ls == nullptr) return;
        memcpy(rh->ls, rhs.rh->ls, rh->ls_size * sizeof(struct riff_levelStackE));
    }
}

// move assignment
RIFFFile & RIFFFile::operator = (RIFFFile &&rhs) noexcept {
    if (&rhs == this)
		return *this;

    if (rh) die();

    memcpy(this, &rhs, sizeof(RIFFFile));

    rhs.reset();

    return *this;
}

// move constructor
RIFFFile::RIFFFile (RIFFFile &&rhs) noexcept {
    memcpy(this, &rhs, sizeof(RIFFFile));

    rhs.reset();
}


RIFFFile::~RIFFFile() {
    die();
}

void RIFFFile::die() {
    riff_handleFree(rh);
    close();
}

void RIFFFile::reset() {
    // Reset the 4 internal variables
    file = nullptr;
    rh = nullptr;
    type = CLOSED;
    __latestError = RIFF_ERROR_NONE;
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
	rh->cl_pos_start = stream->tellg(); //current file offset of stream considered as start of RIFF file
	
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

std::string RIFFFile::latestErrorToString () {
    #define posStrSize 1+2+1+3+1+2+(2*sizeof(size_t))+1

    if (__latestError == RIFF_ERROR_NONE) return "";

    std::string outString(riff_errorToString(__latestError));
    char buffer[posStrSize];
    std::snprintf(buffer, posStrSize, " at pos 0x%zX", rh->pos);
    std::string posString (buffer);
    outString += posString;
    return outString;

    #undef posStrSize
}

std::vector<uint8_t> RIFFFile::readChunkData() {
    __latestError = seekChunkStart(); 
    if (__latestError || rh->c_size == 0) {
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