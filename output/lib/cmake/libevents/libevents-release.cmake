#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libevents_parser" for configuration "Release"
set_property(TARGET libevents_parser APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libevents_parser PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libevents_parser.lib"
  )

list(APPEND _cmake_import_check_targets libevents_parser )
list(APPEND _cmake_import_check_files_for_libevents_parser "${_IMPORT_PREFIX}/lib/libevents_parser.lib" )

# Import target "libevents_health_and_arming_checks" for configuration "Release"
set_property(TARGET libevents_health_and_arming_checks APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(libevents_health_and_arming_checks PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libevents_health_and_arming_checks.lib"
  )

list(APPEND _cmake_import_check_targets libevents_health_and_arming_checks )
list(APPEND _cmake_import_check_files_for_libevents_health_and_arming_checks "${_IMPORT_PREFIX}/lib/libevents_health_and_arming_checks.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
