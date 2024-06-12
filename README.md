# libriff

A C99 library for parsing [RIFF (Resource Interchange File Format)](https://en.wikipedia.org/wiki/Resource_Interchange_File_Format) files with a C++11 wrapper (with optional C++17 features). Some common examples RIFF file types are WAV, AVI, ANI, MIDI SMF, MIDI, DLS.

## Features

- Helps to stroll around in the chunk tree structure
  provides functions like: `openFile()`, `enterList()`, `leaveList()`, `nextChunk()`
- Not specialized in or limited to any specific RIFF form type
- Supports input wrappers for file access via function pointers; wrappers for C file, and memory already present
- Can be seen as simple example for a file format library supporting user defined input wrappers
- Memory-safe, easy to understand C++ wrapper
  - `std::fstream` support
- CMake support
  - Toggleable inclusion of the C++ wrapper
  - Toggleable error printing from the C++ wrapper
  - Toggleable support for `std::filesystem::path` arguments

See [`riff.h`](riff.h) and [`riff.hpp`](riff.hpp) for further info.

TODO:
Make inline C documentation Doxygen compatible
