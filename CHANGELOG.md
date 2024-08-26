# 1.1.0 - the release with major improvements

This release is the first one to have code by @ADM228. It contains multiple quality of life improvements, as well as several new things.

## _libriff-X_: A new name

Since this fork will be extended a lot more (and also due to 2 other libraries existing with the same name), I have decided to rename the library to **_libriff-X_** to represent the fact that it has extended features. This name is purely external, as every file and namespace is still `riff`.

## C++ wrapper

The biggest new thing in this release is the C++ wrapper, with separate [header](src/riff.hpp) and [implementation](src/riff.cpp) files:

- Has a wide compatibility spectrum by being written in C++11
- It provides an easy to use `RIFFFile` class that automatically allocates a `riff_handle` for itself
  - It also automatically allocates a new `riff_handle` when being copied.
  - ERRATA: It allocates a new `riff_handle`, but not a new file / memory object
- Every C function is explicitly or implicitly (the allocator/deallocator) mirrored in the C++ wrappe
- The C++ wrapper stores the latest error code of its methods
  - This allows the programmer to not constantly write `err = riff_...()` - it's done automatically
  - The latest error can be accessed with the `RIFF::latestError` method, and printed with `RIFF::latestErrorToString`
- File handling improvements:
  - A new way to open files is via `std::fstream` via the `RIFF::openFstream` method
  - C FILE objects and memory blocks are still supported via `RIFF::openCFILE` and `RIFF::openMemory` respectively
  - `RIFFFile` can also open its own C FILE and `std::fstream` objects - you just supply the filename and the work is done for you
    - `const char *` and `std::string` arguments are always available
    - `std::filesystem::path` argument is toggleable on/off as a C++17-only feature
- A convenient `RIFFFile::readChunkData` method that reads the entire chunk's data in one go
- The `example.c` file was also ported to C++ to create `example.cpp`

## CMake API

A very easy to use CMake API is now available:

- Can be used with `FetchContent` as well as just being a subdirectory
- Has several useful options:
  - `RIFF_STATIC_LIBRARIES` allows libriff to compile as a static or dynamic library
  - `RIFF_CXX_WRAPPER` allows disabling the C++ wrapper
  - `RIFF_CXX_STD_FILESYSTEM_PATH` allows enabling/disabling `std::filesystem::path` arguments for `RIFFFile::open_...` methods
  - `RIFF_CXX_PRINT_ERRORS` controls whether error messages will be printed from from the C++ wrapper to stdout
  
## Base C library changes

### The files have moved

Instead of just being in the root folder, [riff.c](src/riff.c) and [riff.h](src/riff.h) now reside in the [src](src/) folder. The [riff.cpp](src/riff.cpp) and [riff.hpp](src/riff.hpp) files are also located there.

### Bugfixes

- Memory reading/seeking fixed by taking `riff_handle->pos` into account
  - The internal `riff_handle` functions `fp_read` and `fp_seek` have had their first argument changed from `void *fh` to `struct riff_handle *rh` to make this happen (this is incompatible but idgaf since these are purely internal)
- Every `riff_...` function now checks if the `riff_handle` is valid
- A `malloc` has been replaced with a `calloc` for better level stack allocation
- Multiple `strcpy`s and `strcmp`s replaced with `memcpy`s and `memcmp`s for better security
  
### New features

- `<stdint.h>` is now used for more consistency across platforms
- Now dependent header files (`<stddef.h>`, `<stdio.h>` and `<stdint.h>`) are automatically included in [riff.h](src/riff.h)
- Preliminary support for 64-bit sized files
  - Documentation can be found [here](https://ww.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.2088-1-201910-I!!PDF-E.pdf)
  - libriff now works with files and chunk lists with the ID `BW64` in addition to `RIFF`
  - If the first chunk is `ds64` and the file size is specified to be in this chunk, the 64-bit real file size is read automatically
- New functions for more convenience:
  - `int riff_seekLevelParentStart(struct riff_handle *rh)` seeks to the start of the parent level
  - `int riff_seekLevelParentNext(struct riff_handle *rh)` seeks to the parent level and then to the next chunk
  - `int riff_fileValidate(struct riff_handle *rh)` validates the entire RIFF file _recursively_
  - `int32_t riff_amountOfChunksInLevel(struct riff_handle *rh)` counts the amount of chunks in a chunk level
  - `int32_t riff_amountOfChunksInLevelWithID(struct riff_handle *rh, const char * id)` counts the amount of chunks with a matching ID in a chunk level
  - All of these functions are also available in the C++ wrapper

## Doxygen-compatible documentation

This was one of the TODOs done originally specified by the original creator @murkymark back in 2016 - and it has finally been implemented. The [riff.h](src/riff.h) and [riff.hpp](src/riff.hpp) files contain a Doxygen-compatible documentation entry for _everything_. There is also a [Doxyfile](Doxyfile) in the repository which can be used to generate preliminary Doxygen documentation. The new [docs.h](src/docs.h) file also exists to provide some more useful documenation.

# 1.0.0 - The inital "release" of libriff

This is the state of libriff as it was left my the original creator, @murkymark. I count this as the initial release, since it is what you have to use if not for this fork.

## Class definitions

- `struct riff_handle`, aliased to type `riff_handle` (confusingly, both nomenclatures are used)
- `struct riff_levelStackE`

## Function list

- `riff_handle *riff_handleAllocate();` - Allocate, initialize and return handle
- `void riff_handleFree(riff_handle *rh);` - Free allocated handle memory
- `size_t riff_readInChunk(riff_handle *rh, void *to, size_t size);` - read in current chunk, returns RIFF_ERROR_EOC if end of chunk is reached
- `int riff_seekInChunk(riff_handle *rh, size_t c_pos);` - seek in current chunk, returns RIFF_ERROR_EOC if end of chunk is reached, pos 0 is first byte after chunk size (chunk offset 8)
- `int riff_seekNextChunk(struct riff_handle *rh);` - seek to start of next chunk within current level, ID and size is read automatically, return
- `int riff_seekChunkStart(struct riff_handle *rh);` - seek back to data start of current chunk
- `int riff_rewind(struct riff_handle *rh);` - seek back to very first chunk of file at level 0, the position just after opening via riff_open_...()
- `int riff_seekLevelStart(struct riff_handle *rh);` - goto start of first data byte of first chunk in current level (seek backward)
- `int riff_seekLevelSub(struct riff_handle *rh);` - goto sub level chunk (auto seek to start of parent chunk if not already there); "LIST" chunk typically contains a list of sub chunks
- `int riff_levelParent(struct riff_handle *rh);` - step back from sub list level; position doesn't change and you are still inside the data section of the parent list chunk (not at the beginning of it!); returns != RIFF_ERROR_NONE, if we are at level 0 already and can't go back any further
- `int riff_levelValidate(struct riff_handle *rh);` - validate chunk level structure, seeks to the first byte of the current level, seeks from chunk header to chunk header; to check all sub lists you need to define a recursive function; file position is changed by function
- `const char *riff_errorToString(int e);` - return string to error code; the current position (h->pos) tells you where in the file the problem occured
- `int riff_open_file(riff_handle *h, FILE *f, size_t size);` - create and return initialized RIFF handle, FPs are set up for file access; file position must be at the start of RIFF file, which can be nested in another file (file pos > 0); Since the file was opened by the user, it must be closed by the user; size: must be exact if > 0, pass 0 for unknown size (the correct size helps to identify file corruption)
- `int riff_open_mem(riff_handle *h, void *memptr, size_t size);` - create and return initialized RIFF handle, FPs are set up to default for memory access; If memory was allocated by the user, it must be deallocated by the user after use; size: must be > 0
- `int riff_readHeader(riff_handle *h);` read RIFF file header, return error code; to be called from user IO functions

## Issues

- Memory handling is completely glitched out, disregarding the position in the memory block
