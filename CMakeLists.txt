project(libriff LANGUAGES C CXX)

add_library(libriff STATIC "${CMAKE_CURRENT_SOURCE_DIR}/riff.c")
target_include_directories(libriff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})