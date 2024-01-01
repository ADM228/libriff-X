project(riff LANGUAGES C CXX)

option(RIFF_WRITE "If set to TRUE, will enable writing-related functions in libriff. Default is FALSE." FALSE)

add_library(riff STATIC "${CMAKE_CURRENT_SOURCE_DIR}/riff.c")
target_include_directories(riff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(riff PRIVATE c_std_99)
if (RIFF_WRITE)
	target_compile_definitions(riff PUBLIC RIFF_WRITE)
endif()