# libriff
A C (C99) library for parsing [RIFF (Resource Interchange File Format)](https://en.wikipedia.org/wiki/Resource_Interchange_File_Format) files with a C++17 wrapper. Some common examples RIFF file types are WAV, AVI, ANI, MIDI SMF, MIDI, DLS.

## Features

- Helps to stroll around in the chunk tree structure
  provides functions like: `openFile()`, `enterList()`, `leaveList()`, `nextChunk()`
- Not specialized in or limited to any specific RIFF form type
- Supports input wrappers for file access via function pointers; wrappers for C file, and memory already present
- Can be seen as simple example for a file format library supporting user defined input wrappers
- Memory-safe, easy to understand C++ wrapper
- `std::fstream` in the C++ wrapper
- CMake support
  - Toggleable support for writing functions (to reduce bloat if not used)
  - Toggleable inclusion of the C++ wrapper (again, to reduce bloat if not used)

See [`riff.h`](riff.h) and [`riff.hpp`](riff.hpp) for further info.

TODO:
Make inline C documentation Doxygen compatible
