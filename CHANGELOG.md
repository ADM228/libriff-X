# 1.0.0 - The inital "release" of libriff

This is the state of libriff as it was left my the original creator, Markus Wolf. I count this as the initial release, since it is what you have to use if not for this fork.

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
