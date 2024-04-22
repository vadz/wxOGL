# CMake makefile for OGL studio sample.

add_executable(oglstudio)

target_link_libraries(oglstudio PRIVATE wxogl ${wxWidgets_LIBRARIES})

target_sources(oglstudio PRIVATE
    cspalette.cpp
    csprint.cpp
    dialogs.cpp
    doc.cpp
    mainfrm.cpp
    project.cpp
    shapes.cpp
    studio.cpp
    symbols.cpp
    view.cpp
)
