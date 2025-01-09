# Look for GeographicLib
#
# Set
#  GeographicLib_FOUND = TRUE
#  GeographicLib_INCLUDE_DIRS = /usr/local/include
#  GeographicLib_LIBRARIES = /usr/local/lib/libGeographicLib.so
#  GeographicLib_LIBRARY_DIRS = /usr/local/lib

find_library (GeographicLib_LIBRARIES GeographicLib
  PATHS "${CMAKE_INSTALL_PREFIX}/../GeographicLib/lib")

if (GeographicLib_LIBRARIES)
  get_filename_component (GeographicLib_LIBRARY_DIRS
    "${GeographicLib_LIBRARIES}" PATH)
  get_filename_component (_ROOT_DIR "${GeographicLib_LIBRARY_DIRS}" PATH)
  set (GeographicLib_INCLUDE_DIRS "${_ROOT_DIR}/include")
  set (GeographicLib_BINARY_DIRS "${_ROOT_DIR}/bin")
  if (NOT EXISTS "${GeographicLib_INCLUDE_DIRS}/GeographicLib/Config.h")
    # On Debian systems the library is in e.g.,
    #   /usr/lib/x86_64-linux-gnu/libGeographicLib.so
    # so try stripping another element off _ROOT_DIR
    get_filename_component (_ROOT_DIR "${_ROOT_DIR}" PATH)
    set (GeographicLib_INCLUDE_DIRS "${_ROOT_DIR}/include")
    set (GeographicLib_BINARY_DIRS "${_ROOT_DIR}/bin")
    if (NOT EXISTS "${GeographicLib_INCLUDE_DIRS}/GeographicLib/Config.h")
      unset (GeographicLib_INCLUDE_DIRS)
      unset (GeographicLib_LIBRARIES)
      unset (GeographicLib_LIBRARY_DIRS)
      unset (GeographicLib_BINARY_DIRS)
    endif ()
  endif ()
  unset (_ROOT_DIR)
endif ()

# This sets GeographicLib_FOUND (and GEOGRAPHICLIB_FOUND, deprecated) to TRUE
include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (GeographicLib DEFAULT_MSG
  GeographicLib_LIBRARY_DIRS GeographicLib_LIBRARIES
  GeographicLib_INCLUDE_DIRS)
mark_as_advanced (GeographicLib_LIBRARY_DIRS GeographicLib_LIBRARIES
  GeographicLib_INCLUDE_DIRS)
