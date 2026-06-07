#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::QuickControls2Fusion" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::QuickControls2Fusion APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::QuickControls2Fusion PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELWITHDEBINFO "Qt6::Core;Qt6::Gui;Qt6::Qml;Qt6::QuickControls2FusionStyleImpl;Qt6::QuickControls2;Qt6::Quick;Qt6::QuickTemplates2;Qt6::QuickControls2Basic"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libQt6QuickControls2Fusion.so.6.10.3"
  IMPORTED_SONAME_RELWITHDEBINFO "libQt6QuickControls2Fusion.so.6"
  )

list(APPEND _cmake_import_check_targets Qt6::QuickControls2Fusion )
list(APPEND _cmake_import_check_files_for_Qt6::QuickControls2Fusion "${_IMPORT_PREFIX}/lib/libQt6QuickControls2Fusion.so.6.10.3" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
