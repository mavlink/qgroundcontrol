# Configure GeographicLib
#
# Set
#  GeographicLib_FOUND = TRUE
#  GeographicLib_INCLUDE_DIRS = /usr/local/include
#  GeographicLib_SHARED_LIBRARIES = GeographicLib_SHARED (or empty)
#  GeographicLib_STATIC_LIBRARIES = GeographicLib_STATIC (or empty)
#  GeographicLib_LIBRARY_DIRS = /usr/local/lib
#  GeographicLib_BINARY_DIRS = /usr/local/bin
#  GeographicLib_VERSION = 1.34 (for example)
#  GEOGRAPHICLIB_DATA = /usr/local/share/GeographicLib (for example)
#  Depending on GeographicLib_USE_STATIC_LIBS
#    GeographicLib_LIBRARIES = ${GeographicLib_SHARED_LIBRARIES}, if OFF
#    GeographicLib_LIBRARIES = ${GeographicLib_STATIC_LIBRARIES}, if ON
#  If only one of the libraries is provided, then
#    GeographicLib_USE_STATIC_LIBS is ignored.
#
# Since cmake 2.8.11 or later, there's no need to include
#   include_directories (${GeographicLib_INCLUDE_DIRS})
# The variables are retained for information.
#
# The following variables are only relevant if the library has been
# compiled with a default precision different from double:
#  GEOGRAPHICLIB_PRECISION = the precision of the library (usually 2)
#  GeographicLib_HIGHPREC_LIBRARIES = the libraries need for high precision

message (STATUS "Reading ${CMAKE_CURRENT_LIST_FILE}")
# GeographicLib_VERSION is set by version file
message (STATUS
  "GeographicLib configuration, version ${GeographicLib_VERSION}")

# Tell the user project where to find our headers and libraries
get_filename_component (_DIR ${CMAKE_CURRENT_LIST_FILE} PATH)
if (IS_ABSOLUTE "C:/Users/Henok/qgroundcontrol/_deps/geographiclib-build")
  # This is an uninstalled package (still in the build tree)
  set (_ROOT "C:/Users/Henok/qgroundcontrol/_deps/geographiclib-build")
  set (GeographicLib_INCLUDE_DIRS "C:/Users/Henok/qgroundcontrol/_deps/geographiclib-build/include;C:/Users/Henok/qgroundcontrol/.cache/CPM/geographiclib/1fbf/include")
  set (GeographicLib_LIBRARY_DIRS "${_ROOT}/src")
  set (GeographicLib_BINARY_DIRS "${_ROOT}/tools")
else ()
  # This is an installed package; figure out the paths relative to the
  # current directory.
  get_filename_component (_ROOT "${_DIR}/C:/Users/Henok/qgroundcontrol/_deps/geographiclib-build" ABSOLUTE)
  set (GeographicLib_INCLUDE_DIRS "${_ROOT}/include")
  set (GeographicLib_LIBRARY_DIRS "${_ROOT}/lib")
  set (GeographicLib_BINARY_DIRS "${_ROOT}/bin")
endif ()
set (GEOGRAPHICLIB_DATA "C:/ProgramData/GeographicLib")
set (GEOGRAPHICLIB_PRECISION 2)
set (GeographicLib_HIGHPREC_LIBRARIES "")

set (GeographicLib_SHARED_LIBRARIES )
set (GeographicLib_STATIC_LIBRARIES GeographicLib::GeographicLib_STATIC)
# Read in the exported definition of the library
include ("${_DIR}/geographiclib-targets.cmake")

# For interoperability with older installations of GeographicLib and
# with packages which depend on GeographicLib, GeographicLib_LIBRARIES
# etc. still point to the non-namespace variables.  Tentatively plan to
# transition to namespace variables as follows:
#
# * namespace targets were introduced with version 1.47 (2017-02-15)
# * switch GeographicLib_LIBRARIES to point to namespace variable after
#   2020-02
# * remove non-namespace variables after 2023-02

unset (_ROOT)
unset (_DIR)

if ((NOT GeographicLib_SHARED_LIBRARIES) OR
    (GeographicLib_USE_STATIC_LIBS AND GeographicLib_STATIC_LIBRARIES))
  set (GeographicLib_LIBRARIES ${GeographicLib_STATIC_LIBRARIES})
  message (STATUS "  \${GeographicLib_LIBRARIES} set to static library")
else ()
  set (GeographicLib_LIBRARIES ${GeographicLib_SHARED_LIBRARIES})
  message (STATUS "  \${GeographicLib_LIBRARIES} set to shared library")
endif ()

# Check for the components requested.  This only supports components
# STATIC and SHARED by checking the value of
# GeographicLib_${comp}_LIBRARIES.  No need to check if the component
# is required or not--the version file took care of that.
# GeographicLib_${comp}_FOUND is set appropriately for each component.
if (GeographicLib_FIND_COMPONENTS)
  foreach (comp ${GeographicLib_FIND_COMPONENTS})
    if (GeographicLib_${comp}_LIBRARIES)
      set (GeographicLib_${comp}_FOUND TRUE)
      message (STATUS "GeographicLib component ${comp} found")
    else ()
      set (GeographicLib_${comp}_FOUND FALSE)
      message (WARNING "GeographicLib component ${comp} not found")
    endif ()
  endforeach ()
endif ()

# GeographicLib_FOUND is set to TRUE automatically
set (GEOGRAPHICLIB_FOUND TRUE) # for backwards compatibility, deprecated
