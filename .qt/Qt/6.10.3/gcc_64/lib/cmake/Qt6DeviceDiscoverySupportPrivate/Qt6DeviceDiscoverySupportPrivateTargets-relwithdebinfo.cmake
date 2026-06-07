#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::DeviceDiscoverySupportPrivate" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::DeviceDiscoverySupportPrivate APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::DeviceDiscoverySupportPrivate PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libQt6DeviceDiscoverySupport.a"
  )

list(APPEND _cmake_import_check_targets Qt6::DeviceDiscoverySupportPrivate )
list(APPEND _cmake_import_check_files_for_Qt6::DeviceDiscoverySupportPrivate "${_IMPORT_PREFIX}/lib/libQt6DeviceDiscoverySupport.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
