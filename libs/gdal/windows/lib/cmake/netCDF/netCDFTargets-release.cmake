#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "netcdf" for configuration "Release"
set_property(TARGET netcdf APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(netcdf PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/netcdf.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/netcdf.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS netcdf )
list(APPEND _IMPORT_CHECK_FILES_FOR_netcdf "${_IMPORT_PREFIX}/lib/netcdf.lib" "${_IMPORT_PREFIX}/bin/netcdf.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
