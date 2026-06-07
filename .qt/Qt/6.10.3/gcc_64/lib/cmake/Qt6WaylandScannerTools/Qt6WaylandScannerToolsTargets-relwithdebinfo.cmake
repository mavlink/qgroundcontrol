#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::qtwaylandscanner" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::qtwaylandscanner APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::qtwaylandscanner PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/libexec/qtwaylandscanner"
  )

list(APPEND _cmake_import_check_targets Qt6::qtwaylandscanner )
list(APPEND _cmake_import_check_files_for_Qt6::qtwaylandscanner "${_IMPORT_PREFIX}/libexec/qtwaylandscanner" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
