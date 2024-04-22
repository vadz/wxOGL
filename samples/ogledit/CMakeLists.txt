# CMake makefile for ogledit sample.

add_executable(ogledit)

target_link_libraries(ogledit PRIVATE wxogl ${wxWidgets_LIBRARIES})

target_sources(ogledit PRIVATE
    doc.cpp
    ogledit.cpp
    palette.cpp
    view.cpp
)
