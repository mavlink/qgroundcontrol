#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::QuickParticlesPrivate" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::QuickParticlesPrivate APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::QuickParticlesPrivate PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELWITHDEBINFO "Qt6::Qml"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libQt6QuickParticles.so.6.10.3"
  IMPORTED_SONAME_RELWITHDEBINFO "libQt6QuickParticles.so.6"
  )

list(APPEND _cmake_import_check_targets Qt6::QuickParticlesPrivate )
list(APPEND _cmake_import_check_files_for_Qt6::QuickParticlesPrivate "${_IMPORT_PREFIX}/lib/libQt6QuickParticles.so.6.10.3" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
