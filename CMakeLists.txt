project(riff LANGUAGES C CXX)

add_library(riff STATIC "${CMAKE_CURRENT_SOURCE_DIR}/riff.c")
target_include_directories(riff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})