/*
    C++ wrapper for libriff

    (C) 2023-2024 alexmush
    License: zlib

    The C++ libriff wrapper is a memory-safe, class-based wrapper around the C-based libriff. It adds support for std::ifstream/ofstream, automatic allocation of filestreams (both C-based and if/ofstream) from filenames/filepaths, as well as some additional functions for reading/writing entire chunks at once and setting chunk types/IDs.
*/

#ifndef __RIFF_HPP__
#define __RIFF_HPP__


#include <cstdint>
#include <cstddef>
extern "C" {
    #include <stddef.h>
    #include <stdio.h>
    #include "riff.h"
}
#include <fstream>
#include <vector>
#if RIFF_CXX17_SUPPORT
#include <filesystem>
#endif

namespace RIFF {

enum fileTypes : int {
    C_FILE      = 0,
    FSTREAM,
    MEM_PTR     = 0x10,
    MANUAL      = 0x800000, // For manually opened files
    CLOSED      = -1
};

class RIFFFile {
    public:
        RIFFFile();
        ~RIFFFile();
        /**
         * @brief Open a RIFF file with the filename and mode provided
         * Uses C's fopen(), so the filename is implementation defined
         * @note Always forces binary mode
         * @param filename Filename in fopen()'s format
         * @param mode Modes in fopen()'s format
         * @param size The expected size of the file, leave at 0 (or don't specify) if unknown
         * @return Error code
         */
        int open(const char* __filename, const char * __mode, size_t __size = 0);
        inline int open(const std::string& __filename, const char * __mode, size_t __size = 0) 
            {return open(__filename.c_str(), __mode, __size);};
        #if RIFF_CXX17_SUPPORT
        inline int open(const std::filesystem::path& __filename, const char * __mode, size_t __size = 0)
            {return open(__filename.c_str(), __mode, __size);};
        #endif

        /**
         * @brief Get RIFF data from a memory pointer
         * 
         * @param mem_ptr Pointer to the memory buffer with RIFF data
         * @param size The expected size of the data, leave at 0 (or don't specify) if unknown
         * @return Error code
         */
        int open(const void * __mem_ptr, size_t __size = 0);

        /**
         * @brief Open a RIFF file with the filename and mode provided
         * Uses fstream, and always forces binary mode
         * @param filename 
         * @param mode 
         * @return Error code
         */
        int open(const char* __filename, std::ios_base::openmode __mode = std::ios_base::in, size_t __size = 0);
        int open(const std::string& __filename, std::ios_base::openmode __mode = std::ios_base::in, size_t __size = 0);
        #if RIFF_CXX17_SUPPORT
        int open(const std::filesystem::path& __filename, std::ios_base::openmode __mode = std::ios_base::in, size_t __size = 0);
        #endif

        /**
         * @brief Open a RIFF file from an existing file object
         * @note The close() function of the class will not close the file object
         * @param file The file object 
         * @param size The expected size of the file, leave blank if unknown
         * @return Error code
         */
        int open(std::FILE & __file, size_t __size = 0);
        int open(std::fstream & __file, size_t __size = 0);

        void close();

        /**
         * @brief Read in current chunk
         * @note Returns RIFF_ERROR_EOC if end of chunk is reached
         * 
         * @param to Buffer to read into
         * @param size Amount of data to read
         * @return size_t Amount of data read successfully
         */
        inline size_t readInChunk (void * to, size_t size) {return riff_readInChunk(rh, to, size);};
        /**
         * @brief Read current chunk's data
         * @note Returns nullptr if an error occurred
         * 
         * @return std::vector<uint8_t> with the data
         */
        std::vector<uint8_t> readChunkData ();
        /**
         * @brief Seek in current chunk
         * @note Returns RIFF_ERROR_EOC if end of chunk is reached
         * @note pos 0 is first byte after chunk size (chunk offset 8)
         * 
         * @param size Amount of data to skip
         * @return Error code
         */
        inline int seekInChunk (size_t size) {return riff_seekInChunk(rh, size);};
        /**
         * @brief Seek to start of next chunk within current level
         * @note ID and size are read automatically
         *
         * @return Error code 
         */
        inline int seekNextChunk () {return riff_seekNextChunk(rh);};
        /**
         * @brief Seek back to data start of current chunk
         * 
         * @return Error code 
         */
        inline int seekChunkStart () {return riff_seekChunkStart (rh);};
        /**
         * @brief Seek back to very first chunk of file at level 0
         * Seek back to very first chunk of file at level 0 aka the position just after opening
         * 
         * @return Error code 
         */
        inline int rewind () {return riff_rewind(rh);};
        /**
         * @brief Go to start of first data byte of first chunk in current level
         * 
         * @return Error code  
         */
        inline int seekLevelStart () {return riff_seekLevelStart (rh);};

        /**
         * @brief Go to sub level chunk
         * Go to sub level chunk (auto seek to start of parent chunk if not already there); "LIST" chunk typically contains a list of sub chunks
         * @return Error code  
         */
        inline int seekLevelSub () {return riff_seekLevelSub(rh);};
        /**
         * @brief Step back from sub list level
         * Step back from sub list level; position doesn't change and you are still inside the data section of the parent list chunk (not at the beginning of it!)
         * Returns != RIFF_ERROR_NONE if we are at level 0 already and can't go back any further
         * @return Error code  
         */
        inline int levelParent () {return riff_levelParent(rh);};
        /**
         * @brief Validate chunk level structure
         * Validate chunk level structure, seeks to the first byte of the current level, seeks from chunk header to chunk header
         * To check all sub lists you need to define a recursive function
         * File position is changed by function
         * @return Error code  
         */
        inline int levelValidate () {return riff_levelValidate(rh);};

        /**
         * @brief Return string to error code
         * 
         * @param errorCode 
         * @return Error string, with position at first in hex
         */
        std::string errorToString (int errorCode);

        inline riff_handle & operator() () { return *rh; }

        void * file;

    private:
        riff_handle * rh;

        int type = CLOSED;

        int openFstreamCommon();
        void setAutomaticfstream();
};

}       // namespace RIFF

#endif  // __RIFF_HPP__
