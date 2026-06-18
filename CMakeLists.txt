cmake_minimum_required(VERSION 3.12)
project(myMalloc VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are disabled.")
endif()

include_directories(src/allocator)

add_subdirectory(src)
add_subdirectory(tests)
