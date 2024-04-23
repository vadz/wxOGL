wxWidgets Objects Graphics Library
==================================

This is a slightly modernized version of the original wxOGL library written by
Julian Smart in the last millennium, i.e. a version which can be compiled with
a modern C++ compiler and use wxWidgets 3.x or later.

Some _incompatible_ changes have been made to the original library, see the
corresponding section below for more details.


Building
--------

Under Unix, use CMake to build as usual. The library can be built as either a
static or shared library using the standard CMake `BUILD_SHARED_LIBS` variable.

Under Windows, open `build/msvs/ogl.sln` with MSVS 2022 and build. Only static
libraries are supported under this platform currently.


Using
-----

See ogledit sample. Some documentation is available in LaTeX format.

If you have existing code using wxOGL, please note that this version is not
100% compatible with the original. Notably:

- `wxLineShape::GetLineControlPoints()` returns `std::vector<wxRealPoint>`
  instead of legacy `wxList` container.
