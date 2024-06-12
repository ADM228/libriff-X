project(riff LANGUAGES C CXX)

option(RIFF_CXX_WRAPPER "If set to TRUE, will enable the C++ wrapper for libriff. Default is FALSE." FALSE)

add_library(riff "${CMAKE_CURRENT_SOURCE_DIR}/riff.c")
target_include_directories(riff PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(riff PRIVATE c_std_99)
if (RIFF_CXX_WRAPPER)
	target_sources(riff "${CMAKE_CURRENT_SOURCE_DIR}/riff.c" "${CMAKE_CURRENT_SOURCE_DIR}/riff.cpp")
endif()
