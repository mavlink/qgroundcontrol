#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::QmlLSPrivate" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::QmlLSPrivate APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::QmlLSPrivate PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libQt6QmlLS.a"
  )

list(APPEND _cmake_import_check_targets Qt6::QmlLSPrivate )
list(APPEND _cmake_import_check_files_for_Qt6::QmlLSPrivate "${_IMPORT_PREFIX}/lib/libQt6QmlLS.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
