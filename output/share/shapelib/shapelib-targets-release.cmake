#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "shapelib::GeographicLib_STATIC" for configuration "Release"
set_property(TARGET shapelib::GeographicLib_STATIC APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(shapelib::GeographicLib_STATIC PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/GeographicLib.lib"
  )

list(APPEND _cmake_import_check_targets shapelib::GeographicLib_STATIC )
list(APPEND _cmake_import_check_files_for_shapelib::GeographicLib_STATIC "${_IMPORT_PREFIX}/lib/GeographicLib.lib" )

# Import target "shapelib::shp" for configuration "Release"
set_property(TARGET shapelib::shp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(shapelib::shp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C;CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/shp.lib"
  )

list(APPEND _cmake_import_check_targets shapelib::shp )
list(APPEND _cmake_import_check_files_for_shapelib::shp "${_IMPORT_PREFIX}/lib/shp.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
