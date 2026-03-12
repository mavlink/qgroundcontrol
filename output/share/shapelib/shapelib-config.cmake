# Configure shapelib
#
# Set
#  shapelib_FOUND = 1
#  shapelib_INCLUDE_DIRS = /usr/local/include
#  shapelib_LIBRARIES = shapelib::shp
#  shapelib_LIBRARY_DIRS = /usr/local/lib
#  shapelib_BINARY_DIRS = /usr/local/bin
#  shapelib_VERSION = 1.5.0 (for example)

message (STATUS "Reading ${CMAKE_CURRENT_LIST_FILE}")
# shapelib_VERSION is set by version file
message (STATUS
  "shapelib configuration, version ${shapelib_VERSION}")

# Tell the user project where to find our headers and libraries
get_filename_component (_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
get_filename_component (_ROOT "${_DIR}/../.." ABSOLUTE)
set (shapelib_INCLUDE_DIRS "${_ROOT}/include")
set (shapelib_LIBRARY_DIRS "${_ROOT}/lib")
set (shapelib_BINARY_DIRS "${_ROOT}/bin")

set (shapelib_LIBRARIES shapelib::shp)
# Read in the exported definition of the library
include ("${_DIR}/shapelib-targets.cmake")

unset (_ROOT)
unset (_DIR)

