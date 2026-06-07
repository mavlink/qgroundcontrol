#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::PacketProtocolPrivate" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::PacketProtocolPrivate APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::PacketProtocolPrivate PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libQt6PacketProtocol.a"
  )

list(APPEND _cmake_import_check_targets Qt6::PacketProtocolPrivate )
list(APPEND _cmake_import_check_files_for_Qt6::PacketProtocolPrivate "${_IMPORT_PREFIX}/lib/libQt6PacketProtocol.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
