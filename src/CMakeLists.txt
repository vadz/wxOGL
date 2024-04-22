# CMake makefile for building the library itself.

set(wxogl_SOURCES
    basic.cpp
    basic2.cpp
    bmpshape.cpp
    canvas.cpp
    composit.cpp
    constrnt.cpp
    divided.cpp
    drawn.cpp
    lines.cpp
    mfutils.cpp
    ogldiag.cpp
    oglmisc.cpp
)

add_library(wxogl ${wxogl_SOURCES})

target_compile_definitions(wxogl PUBLIC ${wxWidgets_DEFINITIONS})
target_include_directories(wxogl PUBLIC ${wxWidgets_INCLUDE_DIRS})

target_include_directories(wxogl PUBLIC ../include)
