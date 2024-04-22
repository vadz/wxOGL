# Top level CMake makefile.

# Earlier and later versions might work too but were not tested.
cmake_minimum_required(VERSION 3.17...3.25)

project(wxOGL)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "Building wxOGL using CMake on Windows is not supported.")
endif()

find_package(wxWidgets REQUIRED COMPONENTS core base html)
message(STATUS "Using wxWidgets version ${wxWidgets_VERSION_STRING}")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter ${CMAKE_CXX_FLAGS}")

option(WITH_FATAL_WARNINGS "Make compile warnings fatal" OFF)
if(WITH_FATAL_WARNINGS)
    message(STATUS "Enabling fatal warnings for C++ code")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

option(BUILD_SHARED_LIBS "Build wxOGL as shared library" ON)

add_subdirectory(src)
add_subdirectory(samples)
