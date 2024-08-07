/*
libriff

Author/copyright: Markus Wolf, alexmush
License: zlib (https://opensource.org/licenses/Zlib)


To read any RIFF files.
Not specialized to specific types like AVI or WAV.
Special chunks (e.g. "LIST") can contain a nested sub list of chunks


Example structure of RIFF file:

chunk list start ("RIFF") - ID,size,type
  type (of parent chunk header)
  chunk block - list level 0
  chunk block ("LIST") - level 0 - ID,size,type
    type (of parent chunk header)
    chunk block - level 1
    chunk block - level 1
  chunk block - level 0


Usage:
Use a default open-function (file, mem) or create your own
  The required function pointers for reading and seeking are set here
  When creating your own open-function, take a look at the default function code as template
After the file is opened we are in the first chunk at list level 0
 You can freely read and seek within the data of the current chunk
 Use riff_nextChunk() to go to the first data byte of the next chunk (chunk header is read already then)
If the current chunk contains a list of sub level chunks:
 Call riff_seekLevelSub() to be positioned at the first data byte of the first chunk in the sub list level
 Call riff_levelParent() to leave the sub list without changing the file position
Read members of the riff_handle to get all info about current file position, current chunk, etc.

May not work for RIFF files larger than 2GB.
*/

/**
 * @authors Markus Wolf, alexmush
 * @copyright Markus Wolf (2016), alexmush (2023-2024)
 * @version 1.1.0
 * @date 2016-2024
 * 
 * @include README.md
 * 
 */




#ifndef _RIFF_H_
#define _RIFF_H_

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

/**
 * @brief Size of RIFF file header and RIFF/LIST chunks that contain subchunks.
 */
#define	RIFF_HEADER_SIZE		12
/**
 * @brief The offset of data compared to the start of the chunk, equals size of chunk ID + chunk size field.
 */
#define	RIFF_CHUNK_DATA_OFFSET	8

/**
 * @defgroup Errors Error codes
 * 
 * Error codes yielded by most `riff_...()` functions.
 * 
 * @note Do not hardcode these in your software as the value mapping may change in the future.
 * @{
 */

/**
 * @name Non-critical errors
 * @{
 */

/**
 * @brief No error.
 */
#define RIFF_ERROR_NONE		0
/**
 * @brief End of current chunk.
 * 
 * Occurs when trying to read/seek beyond end of current chunk data.
 */
#define RIFF_ERROR_EOC		1
/**
 * @brief End of chunk list.
 * 
 * Occurs when trying to seek the next chunk while already at the last chunk in a chunk list.
 */
#define RIFF_ERROR_EOCL		2
/**
 * @brief Excess bytes at end of chunk list level.
 * 
 * Not critical, the excess data is simply ignored (1-7 bytes inside list, otherwise a following chunk is expected - more at file level possible), should never occur.
 */
#define RIFF_ERROR_EXDAT	3

///@}

/**
 * @name Critical errors
 * @{
 */

/**
 * @brief First critical error code.
 * 
 * A way to check if an error is critical - just use @c ( @c >= @c RIFF_ERROR_CRITICAL @c ).
 */
#define RIFF_ERROR_CRITICAL	4

/**
 * @brief Illegal ID.
 * 
 * Occurs when ID (or type) contains not printable or non-ASCII characters or is wrong.
 */
#define RIFF_ERROR_ILLID	4
/**
 * @brief Invalid chunk size value in chunk header.
 * 
 * Occurs when the chunk size is too small or exceeds list level or file.
 * 
 * Indicates corruption or cut off file.
 */
#define RIFF_ERROR_ICSIZE	5
/**
 * @brief Unexpected end of RIFF file.
 * 
 * Indicates corruption (wrong chunk size field), a cut off file or the size argument for the opening function having been wrong (too small).
 */
#define RIFF_ERROR_EOF		6
/**
 * @brief Access error.
 * 
 * Indicates that the file is not accessible (could happen due to permissions, invalid file handle, etc.).
 */
#define RIFF_ERROR_ACCESS	7

/**
 * @brief riff_handle is not set up or is NULL.
 */
#define RIFF_ERROR_INVALID_HANDLE	8

///@}

/**
 * @brief The last RIFF_ERROR code.
 */
#define RIFF_ERROR_MAX 8

///@}

/**
 * @brief Level stack entry struct.
 *
 * Needed to retrace from sublevel (list) chunks.
 * 
 * (sorta) Contains header info of the parent level.
 */
struct riff_levelStackE {
	/**
	 * @brief Absolute parent chunk position in file stream.
	 */
	size_t c_pos_start;
	/**
	 * @brief ID of parent chunk.
	 *
	 * The first 4 bytes of RIFF/LIST chunk data.
	 */
	char c_id[5];
	/**
	 * @brief Parent chunk size.
	 *
	 * Without header (contains value as stored in RIFF file).
	 */
	size_t c_size;
	/**
	 * @brief Type ID of parent chunk.
	 * 
	 * Should either be RIFF, LIST or BW64.
	 */
	char c_type[5];
};

/**
 * @defgroup riff_handle The RIFF handle
 * @{
 */
/**
 * @brief The RIFF handle.
 * 
 * Members are public and intended for read access (to avoid a plethora of get-functions).
 * 
 * Be careful with the level stack, check riff_handle::ls_size first.
 */
typedef struct riff_handle {
	/**
	 * @name RIFF file header info.
	 * 
	 * Available after the file has been opened.
	 */
	///@{
	/**
	 * @brief Header ID, should be `"RIFF"`.
	 * 
	 * Contains terminator to be printable.
	 */
	char h_id[5];
	/**
	 * @brief Size value given in header.
	 * 
	 * h_size + 8 equals to the file size.
	 */
	size_t h_size;
	/**
	 * @brief Type ID of file.
	 * 
	 * Should contain 4 ASCII characters.
	 * 
	 * Also contains terminator to be printable.
	 */
	char h_type[5];
	/**
	 * @brief Start position of RIFF file.
	 * 
	 * @todo Maybe making it current level would optimize stuff?.
	 */
	size_t pos_start;
	///@}

	/**
	 * @brief Total size of RIFF file.
	 * 
	 * 0 means unspecified.
	 */
	size_t size;
	/**
	 * @brief Current position in data stream.
	 */
	size_t pos;
	
	/**
	 * @name Current chunk's data.
	 */
	///@{
	/**
	 * @brief Absolute start position of current chunk.
	 */
	size_t c_pos_start;
	/**
	 * @brief Position in current chunk.
	 * 
	 * Relative to the start of the chunk's data block.
	 */
	size_t c_pos;
	/**
	 * @brief ID of current chunk.
	 * 
	 * Contains terminator to be printable.
	 */
	char c_id[5];
	/**
	 * @brief Size of current chunk.
	 * 
	 * Excludes chunk header - same value as stored in RIFF file.
	 */
	size_t c_size;
	/**
	 * @brief Pad byte.
	 * 
	 * 1 if c_size is odd, else 0 (indicates unused extra byte at end of chunk).
	 */
	uint8_t pad;
	///@}

	/**
	 * @name Level stack data.
	 */
	///@{
	/**
	 * @brief Level stack pointer.
	 * 
	 * Resizes dynamically.
	 * 
	 * To access the parent chunk data use `ls[ls_level-1]`.
	 */
	struct riff_levelStackE *ls;
	/**
	 * @brief Size of stack in entries.
	 * 
	 * Stack extends automatically if needed.
	 */
	size_t ls_size;
	/**
	 * @brief Current stack level.
	 * 
	 * Starts at 0.
	 */
	int ls_level;
	///@}
	
	/**
	 * @brief Data access handle.
	 * 
	 * File handle or memory address.
	 * 
	 * Only accessed by user FP functions.
	 */
	void *fh;
	
	
	
	/**
	 * @name Internal functions
	 * 
	 * Function pointers for e.g. defining your own input methods
	 */
	///@{
	/**
	 * @brief Read bytes.
	 * 
	 * @note Required for proper operation.
	 */
	size_t (*fp_read)(struct riff_handle *rh, void *ptr, size_t size);
	
	/**
	 * @brief Seek position relative to start pos.
	 * 
	 * @note Required for proper operation.
	 */
	size_t (*fp_seek)(struct riff_handle *rh, size_t pos);
	
	/**
	 * @brief Print error.
	 * 
	 * riff_allocate maps it to `vfprintf(stderr, ...)` by default.\n 
	 * Set this pointer to NULL right after allocation to disable any printing.\n 
	 * This pointer should only be modified right after allocation, and before any other `riff_...()` functions.
	 */
	int (*fp_printf)(const char * format, ... );

	///@}
	
} riff_handle;

///@}

/**
 * @defgroup RIFF_C C RIFF functions
 * @{
 */
/**
 * @name riff_handle allocation/deallocation functions
 * @{
 */
/**
 * @brief Allocate, initialize and return a riff_handle.
 * 
 * @return Pointer to the intialized riff_handle.
 */
riff_handle *riff_handleAllocate();
/**
 * @brief Free the memory allocated to a riff_handle.
 * 
 * @param rh The riff_handle to free.
 */
void riff_handleFree(riff_handle *rh);

///@}


/**
 * @name Parsing functions
 * @{
 */
/**
 * @brief Read to memory block.
 *
 * @param rh The riff_handle to use.
 * @param to The pointer to read data to.
 * @param size The amount of data to read.
 * 
 * @return Amount of successfully read bytes.
 *
 * Does not read beyond end of chunk.
 * 
 * Does not read the pad byte.
 */
size_t riff_readInChunk(riff_handle *rh, void *to, size_t size);
/**
 * @brief Seek in current chunk.
 *
 * @param rh The riff_handle to use.
 * @param c_pos The chunk position to seek to.
 * 
 * @return RIFF error code.
 *
 * @note Only counts the actual data section of the chunk - position 0 is first byte after chunk size (chunk offset 8).
 */
int riff_seekInChunk(riff_handle *rh, size_t c_pos);

///@}

/**
 * @name Chunk seeking functions
 * @{
 */
/**
 * @brief Seek to start of next chunk within current level.
 * 
 * ID and size are read automatically.
 * 
 * @param rh The riff_handle to use.
 * 
 * @return RIFF error code.
 */
int riff_seekNextChunk(struct riff_handle *rh);
//int riff_seekNextChunkID(struct riff_handle *rh, char *id);  //find and go to next chunk with id (4 byte) in current level, fails if not found - position is invalid then -> maybe not needed, the user can do it via simple loop
/**
 * @brief Seek to data start of the current chunk.
 * 
 * @param rh The riff_handle to use.
 * 
 * @return RIFF error code.
 */
int riff_seekChunkStart(struct riff_handle *rh);
/**
 * @brief Seek to the very first chunk of the file.
 * 
 * Rewinds to the same position as just after opening via `riff_open_...()`.
 * 
 * @param rh The riff_handle to use.
 * 
 * @return RIFF error code.
 */
int riff_rewind(struct riff_handle *rh);
/**
 * @brief Seek to the beginning of the current level.
 * 
 * Seeks to the first data byte of the first chunk in current level.
 * 
 * @param rh The riff_handle to use.
 * 
 * @return RIFF error code.
 */
int riff_seekLevelStart(struct riff_handle *rh);

///@}

/**
 * @name Level seeking functions
 * @{
 */

/**
 * @brief Go to sub level chunk.
 * 
 * @note Automatically seeks to the start of parent chunk's data.
 * 
 * @param rh The riff_handle to use.
 * 
 * @return RIFF error code.
 */
int riff_seekLevelSub(struct riff_handle *rh);

/**
 * @brief Step back from sub list level.
 * 
 * @note The position does not change, you are still inside the data section of the parent list chunk!
 * 
 * @param rh The riff_handle to use.
 * 
 * @return RIFF error code.
 */
int riff_levelParent(struct riff_handle *rh);

//int riff_seekLevelParent(struct riff_handle *rh);     //go back from sub level to the start of parent chunk (seek backward)
//int riff_seekLevelParentNext(struct riff_handle *rh); //go back from sub level to the start of the chunk following the parent chunk (seek forward)

/**
 * @brief Validate chunk level structure.
 * 
 * Seeks to the first byte of the current level, then header to header inside of the current chunk level.
 * 
 * To check all sub lists you need recursion.
 * 
 * File position is changed by this function.
 * 
 * @param rh The riff_handle to use.
 * 
 * @return RIFF error code.
 */
int riff_levelValidate(struct riff_handle *rh);

///@}

/**
 * @brief Return error string.
 * 
 * @note The current position (riff_handle::pos) tells you where the problem occured in the file.
 * 
 * @param e The error code.
 * 
 * @return Pointer to string with the name of the error.
 */
const char *riff_errorToString(int e);

/**
 * @todo Validate all, follow LIST chunks
 * @todo Check for duplicate chunk ID in one evel (????????)
 */

/**
 * @name I/O Init functions
 * 
 * Functions used by built-in open functions \n
 * (To be used in your user-made open functions)
 * 
 * @{
 */
/**
 * @brief Read RIFF file header.
 * 
 * To be caled from user I/O functions.
 * 
 * @param rh The riff_handle to use.
 * 
 * @return RIFF error code.
 */
int riff_readHeader(riff_handle *rh);

///@}

/**
 * @name RIFF Open functions
 * 
 * Use the following built in open-functions or make your own.
 * 
 * Only pass a fresh allocated handle.
 * 
 * @{
 */

/**
 * @brief Initialize RIFF handle and set up FPs for C FILE access.
 * 
 * @note File position must be at the start of the RIFF data (it can be nested in another file).
 * @note Since the file was opened by the user, it must be closed by the user.
 * @note The file size must be exact if > 0, use 0 for unknown size \n (the correct size helps to identify file corruption).
 * 
 * @param rh The riff_handle to initialize.
 * @param f The FILE pointer to read from.
 * @param size The file size.
 * 
 * @return RIFF error code.
 */
int riff_open_file(riff_handle *rh, FILE *f, size_t size);

/**
 * @brief Initialize RIFF handle and set up FPs for memory access.
 * 
 * @note Since the memory was allocated by the user, it must be deallocated by the user.
 * 
 * @param rh The riff_handle to initialize.
 * @param f The FILE pointer to read from.
 * @param size The file size, must be > 0.
 * 
 * @return RIFF error code.
 */
int riff_open_mem(riff_handle *rh, const void *memptr, size_t size);


//user open - must handle "riff_handle" allocation and setup
// e.g. for file access via network socket
// see and use "riff_open_file()" definition as template
// "int open_user(riff_handle *h, FOO, size_t size)";

///@}

///@}

#endif // _RIFF_H_
