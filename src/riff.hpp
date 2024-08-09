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
#include <cstring>
extern "C" {
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

/**
 * @brief A lightweight wrapper class around riff_handle
 * 
 * This class allows you to forget about the difficulties of manually managing the riff_handle's memory, while still providing very direct access to it (as well as a few wrapper functions).
 * 
 * @todo Copy/move constructors/assignment from riff_handle
 * @todo fstream/memory open constructors
 * @todo Internal errors
 */
class RIFFFile {
    public:
        /**
         * @brief Construct a new RIFFFile object.
         * 
         * Constructs a new RIFFFile object, allocates a riff_handle for it.
         */
        RIFFFile();

        /**
         * @brief Copy-construct a new RIFFFile object.
         * 
         * Constructs a new RIFFFile object, copies the other RIFFFile object's data (and allocates a new riff_handle for it).
         * 
         * @note While it copies the riff_handle data, the file / memory pointer stays the exact same as the old one, meaning 2 RIFFFile objects are accessing the same data!
         * 
         * @param rhs The RIFFFile object to copy.
         */
        RIFFFile(const RIFFFile &rhs);

        /**
         * @brief Copy RIFFFile object data.
         * 
         * Copies the other RIFFFile object's data (and allocates a new riff_handle for it).
         * 
         * @note While it copies the riff_handle data, the file / memory pointer stays the exact same as the old one, meaning 2 RIFFFile objects are accessing the same data!
         * 
         * @param rhs The RIFFFile object to copy.
         */
        RIFFFile & operator = (const RIFFFile &rhs);

        /**
         * @brief Move-construct a new RIFFFile object
         * 
         * Constructs a new RIFFFile object, moves the other RIFFFile object's data to the new one, sets the other one to factory settings.
         * 
         * @param rhs The RIFFFile object to move.
         */
        RIFFFile (RIFFFile &&rhs) noexcept;

        /**
         * @brief Move RIFFFile object data.
         * 
         * Moves the other RIFFFile object's data to the new one, sets the other one to factory settings.
         * 
         * @param rhs The RIFFFile object to move.
         */
        RIFFFile & operator = (RIFFFile &&rhs) noexcept;

        /**
         * @brief Destroy the RIFFFile object.
         * 
         * Deallocates riff_handle, closes the file, destroys the RIFFFile object.
         */
        ~RIFFFile();

        /**
         * @defgroup RIFF_CPP C++ RIFF functions
         * @{
         */

        /**
         * @name Opening/closing methods
         * @{
         */

        /**
         * @brief Open a RIFF file with C's `fopen()`.
         * 
         * Since it uses C's fopen(), the filename is implementation defined.
         * 
         * @param filename Filename in fopen()'s format.
         * @param detectSize Whether to detect the size of the file or leave it unknown to the RIFF handle.
         * 
         * @return RIFF error code.
         */
        int openCFILE(const char* filename, bool detectSize = true);
        /**
         * @brief Open a RIFF file with C's `fopen()`.
         * 
         * Since it uses C's fopen(), the filename is implementation defined.
         * 
         * @param filename Filename in fopen()'s format.
         * @param detectSize Whether to detect the size of the file or leave it unknown to the RIFF handle.
         * 
         * @return RIFF error code.
         */
        inline int openCFILE(const std::string& filename, bool detectSize = true) 
            {return openCFILE(filename.c_str(), detectSize);};
        #if RIFF_CXX17_SUPPORT
        /**
         * @brief Open a RIFF file with C's `fopen()`.
         * 
         * Since it uses C's fopen(), the filename is implementation defined.
         * 
         * @param filename Filename in fopen()'s format.
         * @param detectSize Whether to detect the size of the file or leave it unknown to the RIFF handle.
         * 
         * @return RIFF error code.
         */
        inline int openCFILE(const std::filesystem::path& filename, bool detectSize = true)
            {return openCFILE(filename.c_str(), detectSize);};
        #endif

        /**
         * @brief Open a RIFF file with C++'s std::fstream.
         * 
         * @param filename Filename in std::fstream's format.
         * @param detectSize Whether to detect the size of the file or leave it unknown to the RIFF handle.
         * 
         * @return RIFF error code.
         */
        int openFstream(const char* filename, bool detectSize = true);
        /**
         * @brief Open a RIFF file with C++'s std::fstream.
         * 
         * @param filename Filename in std::fstream's format.
         * @param detectSize Whether to detect the size of the file or leave it unknown to the RIFF handle.
         * 
         * @return RIFF error code.
         */
        int openFstream(const std::string& filename, bool detectSize = true);
        #if RIFF_CXX17_SUPPORT
        /**
         * @brief Open a RIFF file with C++'s std::fstream.
         * 
         * @param filename Filename in std::fstream's format.
         * @param detectSize Whether to detect the size of the file or leave it unknown to the RIFF handle.
         * 
         * @return RIFF error code.
         */
        int openFstream(const std::filesystem::path& filename, bool detectSize = true);
        #endif

        /**
         * @brief Open a RIFF file from an existing C FILE object.
         * 
         * @note Since the file object was opened by the user, the close() function of the class will not close the file object.
         * 
         * @param file The C FILE object.
         * @param size The expected size of the file, leave blank if unknown.
         * 
         * @return RIFF error code.
         */
        int openCFILE(std::FILE & file, size_t size = 0);
        /**
         * @brief Open a RIFF file from an existing std::fstream object.
         * 
         * @note Since the file object was opened by the user, the close() function of the class will not close the file object.
         * 
         * @param file The std::fstream object.
         * @param size The expected size of the file, leave blank if unknown.
         * 
         * @return RIFF error code.
         */
        int openFstream(std::fstream & file, size_t size = 0);
        /**
         * @brief Get RIFF data from a memory pointer.
         * 
         * @param mem_ptr Pointer to the memory buffer with RIFF data.
         * @param size The expected size of the data, leave at 0 (or don't specify) if unknown.
         * 
         * @return RIFF error code.
         */
        int openMemory(const void * mem_ptr, size_t size = 0);

        /**
         * @brief Closes the file.
         * 
         * @note Only actually closes the file if it was opened automatically (if it was opened by the user, the user must close it).
         */
        void close();

        ///@}

        /**
         * @name Parsing methods
         * @{
         */

        /**
         * @brief Read in current chunk.
         * 
         * Does not read beyond end of chunk.
         * 
         * Does not read pad byte.
         * 
         * @param to Buffer to read into.
         * @param size Amount of data to read.
         * 
         * @return size_t Amount of data read successfully.
         */
        inline size_t readInChunk (void * to, size_t size) {return riff_readInChunk(rh, to, size);};
        /**
         * @brief Read current chunk's data.
         * 
         * @note Returns an empty vector if an error occurred.
         * 
         * @return std::vector<uint8_t> with the data.
         */
        std::vector<uint8_t> readChunkData ();
        /**
         * @brief Seek in current chunk.
         * 
         * @note Only counts the actual data section of the chunk - position 0 is first byte after chunk size (chunk offset 8).
         * 
         * @param size Amount of data to skip.
         * 
         * @return RIFF error code.
         */
        inline int seekInChunk (size_t size) {return riff_seekInChunk(rh, size);};

        ///@}

        /**
         * @name Chunk seeking methods
         * @{
         */

        /**
         * @brief Seek to start of next chunk within current level.
         * 
         * ID and size are read automatically.
         *
         * @return RIFF error code.
         */
        inline int seekNextChunk () {return riff_seekNextChunk(rh);};
        /**
         * @brief Seek back to data start of current chunk.
         * 
         * @return RIFF error code.
         */
        inline int seekChunkStart () {return riff_seekChunkStart (rh);};
        /**
         * @brief Seek to the very first chunk of the file
         * 
         * Rewinds to the same position as just after opening via the `open()` method.
         * 
         * @return RIFF error code.
         */
        inline int rewind () {return riff_rewind(rh);};
        /**
         * @brief Seek to the beginning of the current level.
         * 
         * Seeks to the first data byte of the first chunk in current level.
         * 
         * @return RIFF error code.
         */
        inline int seekLevelStart () {return riff_seekLevelStart (rh);};

        ///@}

        /**
         * @name Level seeking methods
         * @{
         */

        /**
         * @brief Go to sub level chunk.
         * 
         * @note Automatically seeks to the start of parent chunk's data.
         * 
         * @return RIFF error code.
         */
        inline int seekLevelSub () {return riff_seekLevelSub(rh);};
        /**
         * @brief Step back from sub list level.
         * 
         * @note The position does not change, you are still inside the data section of the parent list chunk!
         * 
         * @return RIFF error code.
         */
        inline int levelParent () {return riff_levelParent(rh);};
        /**
         * @brief Validate chunk level structure.
         * 
         * Seeks to the first byte of the current level, then header to header inside of the current chunk level.
         * 
         * To check all sub lists you need recursion.
         * 
         * File position is changed by this function.
         * 
         * @return RIFF error code.
         */
        inline int levelValidate () {return riff_levelValidate(rh);};

        ///@}

        /**
         * @brief Return error string with position.
         * 
         * Includes the position where the problem occured.
         * 
         * @param errorCode The error code to print.
         * 
         * @return Error string with position.
         */
        std::string errorToString (int errorCode);

        /**
         * @brief Access the riff_handle object.
         * 
         * @return The riff_handle.
         */
        inline const riff_handle & operator() () { return *rh; }

        ///@}

        /**
         * @brief File pointer
         * 
         * The pointer to the file object as provided by open() methods
         */
        void * file = nullptr;

    private:
        riff_handle * rh = nullptr;

        int type = CLOSED;

        int openFstreamCommon(size_t);
        void setAutomaticFstream();
        size_t detectFstreamSize(bool);

        void die();
        void reset();
};

}       // namespace RIFF

#endif  // __RIFF_HPP__
