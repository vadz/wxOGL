# Top level CMake makefile.

# Earlier and later versions might work too but were not tested.
cmake_minimum_required(VERSION 3.17...3.25)

project(wxOGL)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message(FATAL_ERROR "Building wxOGL using CMake on Windows is not supported.")
endif()

# Check if we're being built as a top-level project or a dependency of some
# other project (using add_subdirectory).
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(WXOGL_IS_TOP_PROJECT TRUE)
else()
    set(WXOGL_IS_TOP_PROJECT FALSE)
endif()

# Top project is responsible for providing wx::core etc alias targets.
if(WXOGL_IS_TOP_PROJECT)
    find_package(wxWidgets REQUIRED COMPONENTS core base html)
    message(STATUS "Using wxWidgets version ${wxWidgets_VERSION_STRING}")

    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter ${CMAKE_CXX_FLAGS}")

    option(WITH_FATAL_WARNINGS "Make compile warnings fatal" OFF)
    if(WITH_FATAL_WARNINGS)
        message(STATUS "Enabling fatal warnings for C++ code")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
    endif()
endif()

option(BUILD_SHARED_LIBS "Build wxOGL as shared library" ON)

add_subdirectory(src)

if(NOT WXOGL_IS_TOP_PROJECT)
    add_library(wx::ogl ALIAS wxogl)
endif()

if(WXOGL_IS_TOP_PROJECT)
    add_subdirectory(samples)
endif()
