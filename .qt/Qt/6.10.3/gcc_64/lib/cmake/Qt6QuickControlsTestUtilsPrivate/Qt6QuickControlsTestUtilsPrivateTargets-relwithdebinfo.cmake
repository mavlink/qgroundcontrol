#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::QuickControlsTestUtilsPrivate" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::QuickControlsTestUtilsPrivate APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::QuickControlsTestUtilsPrivate PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libQt6QuickControlsTestUtils.a"
  )

list(APPEND _cmake_import_check_targets Qt6::QuickControlsTestUtilsPrivate )
list(APPEND _cmake_import_check_files_for_Qt6::QuickControlsTestUtilsPrivate "${_IMPORT_PREFIX}/lib/libQt6QuickControlsTestUtils.a" )

# Import target "Qt6::QuickControlsTestUtilsPrivate_resources_1" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::QuickControlsTestUtilsPrivate_resources_1 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::QuickControlsTestUtilsPrivate_resources_1 PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_RELWITHDEBINFO ""
  IMPORTED_OBJECTS_RELWITHDEBINFO "${_IMPORT_PREFIX}/qml/Qt/test/controls/objects-RelWithDebInfo/QuickControlsTestUtilsPrivate_resources_1/.qt/rcc/qrc_qmake_Qt_test_controls_init.cpp.o"
  )

list(APPEND _cmake_import_check_targets Qt6::QuickControlsTestUtilsPrivate_resources_1 )
list(APPEND _cmake_import_check_files_for_Qt6::QuickControlsTestUtilsPrivate_resources_1 "${_IMPORT_PREFIX}/qml/Qt/test/controls/objects-RelWithDebInfo/QuickControlsTestUtilsPrivate_resources_1/.qt/rcc/qrc_qmake_Qt_test_controls_init.cpp.o" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
