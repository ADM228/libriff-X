/**
 * @mainpage libriff - a C99 library for reading RIFF files
 *
 * @authors Markus Wolf, alexmush
 * @copyright zlib license
 *
 * The library is intended for reading RIFF files - any RIFF files, not just specializing in a certain type like AVI or WAV.
 *
 * @section RIFFDesc A simplified description of RIFF structure
 *
 * A RIFF file consists of chunks, each one marked with an ID and a 4-byte size. There are special "LIST" chunks that can contain a nested sub list of chunks. The entire RIFF file is also considered such a chunk with an ID "RIFF".
 *
 * Here's an example structure of RIFF file:
 *
 * ```
 * chunk list start ("RIFF") - ID, size
 * data:
 *   type (of parent chunk header)
 *   chunk block - list level 0 - ID, size, data
 *   chunk block ("LIST") - level 0 - ID, size
 *   data:
 *     type (of parent chunk header)
 *     chunk block - level 1 - ID, size, data
 *     chunk block - level 1 - ID, size, data
 *   chunk block - level 0 - ID, size, data
 * ```
 *
 * @section C_Usage Example usage in C
 * First, allocate a riff_handle:
 * ```c
 * #include "riff.h"
 * #include <stdio.h>
 *
 * riff_handle * rh = riff_handleAllocate();
 * ```
 * Then open some RIFF data, either by using a default open-function (C FILE*, memory pointer):
 * ```c
 * FILE * file = fopen(filename, "rb"); // Always use binary mode when opening files!
 *
 *  // Get the size of the file
 * fseek(file, 0, SEEK_END);
 * long fsize = ftell(file);
 * fseek(file, 0, SEEK_SET);
 *
 * riff_open_file(rh, file, fsize);
 * ```
 * or creating your own. The essential function pointers for reading and seeking are set here. When creating your own open-function, take a look at the default function code in src/riff.c as a template.
 *
 * After the file is opened, the riff_handle is located at the first chunk of the first level (level 0). You can freely read and seek within the data of the current chunk:
 * ```c
 * int errCode = riff_seekInChunk(rh, 45);
 * char buffer[72];
 * size_t amountRead = riff_readInChunk(rh, buffer, sizeof(buffer));
 * ```
 * Once you're done with a chunk, you can move on to the next one:
 * ```c
 * errCode = riff_seekNextChunk(rh);
 * ```
 * Seeking to a previous chunk is impossible without making additional data structures, as the chunks only specify their own size, so the best you can do is seek to the start of the chunk and then seek forward:
 * ```c
 * errCode = riff_seekLevelStart(rh);
 * while (whatever || errCode == RIFF_ERROR_EOCL) {errCode = riff_seekNextChunk(rh);};  // check for end of chunk list
 * ```
 * You probably noticed me constantly getting some `errCode` and wondered why that is done. Well, RIFF has a pretty robust error-reporting system:
 * ```c
 * if (errCode) {
 *   fprintf(stderr, "%s", riff_errorToString(errCode)); // Print the error message to stderr
 *   if (errCode >= RIFF_ERROR_CRITICAL) { // This is how you check if an error is critical
 *      return 1;
 *   }
 * }
 * ```
 * If the error isn't critical, you can probably just go on with your life.
 * 
 * Special "LIST" chunks contain subchunks, which you can access:
 * ```c
 * errCode = riff_seekLevelSub(rh);
 * if (!errCode) {
 *   // do whatever you want here
 *   errCode = riff_levelParent(rh); // Step back from sublevel
 * }
 * ```
 * 
 * @section Raw_CPP_Usage Example usage with C++ wrapper
 * 
 * The usage is very similar to what is described in the C section above, but done via the RIFFHandle class in the RIFF namespace. It also contains `open` methods that open the file for you, as well as std::fstream support. Here's the above example rewritten in code for the C++ wrapper:
 * ```cpp
 * #include "riff.hpp"
 * #include <iostream>
 * 
 * RIFF::RIFFHandle rh = RIFF::RIFFHandle(filename); // Automatically detects file size
 * 
 * rh.seekInChunk(45);
 * char buffer[72];
 * size_t amountRead = rh.readInChunk(buffer, sizeof(buffer));
 * 
 * rh.seekNextChunk();
 * 
 * rh.seekLevelStart();
 * while (whatever || rh.latestError() == RIFF_ERROR_EOCL) {rh.seekNextChunk();};  // check for end of chunk list
 * 
 * if (rh.latestError()) {  // Errors are tracked internally, this is how you access the error code
 *   std::cerr << rh.latestErrorToString() << std::endl; // Print the error message to stderr
 *   if (rh.latestError() >= RIFF_ERROR_CRITICAL) { // This is how you check if an error is critical
 *      return 1;
 *   }
 * }
 * 
 * rh.seekLevelSub();
 * if (!rh.latestError()) {
 *   // do whatever you want here
 *   errCode = rh.levelParent(); // Step back from sublevel
 * }
 * ```
 * 
 * @version 1.1.0
 * @date 2016-2024
 * 
 * 
 */
