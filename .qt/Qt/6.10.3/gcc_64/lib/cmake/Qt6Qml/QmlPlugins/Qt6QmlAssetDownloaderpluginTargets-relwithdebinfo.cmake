#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::QmlAssetDownloaderplugin" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::QmlAssetDownloaderplugin APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::QmlAssetDownloaderplugin PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/qml/Qt/labs/assetdownloader/libqmlassetdownloaderplugin.a"
  )

list(APPEND _cmake_import_check_targets Qt6::QmlAssetDownloaderplugin )
list(APPEND _cmake_import_check_files_for_Qt6::QmlAssetDownloaderplugin "${_IMPORT_PREFIX}/qml/Qt/labs/assetdownloader/libqmlassetdownloaderplugin.a" )

# Import target "Qt6::QmlAssetDownloaderplugin_init" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::QmlAssetDownloaderplugin_init APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::QmlAssetDownloaderplugin_init PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_RELWITHDEBINFO ""
  IMPORTED_OBJECTS_RELWITHDEBINFO "${_IMPORT_PREFIX}/qml/Qt/labs/assetdownloader/objects-RelWithDebInfo/QmlAssetDownloaderplugin_init/QmlAssetDownloaderplugin_init.cpp.o"
  )

list(APPEND _cmake_import_check_targets Qt6::QmlAssetDownloaderplugin_init )
list(APPEND _cmake_import_check_files_for_Qt6::QmlAssetDownloaderplugin_init "${_IMPORT_PREFIX}/qml/Qt/labs/assetdownloader/objects-RelWithDebInfo/QmlAssetDownloaderplugin_init/QmlAssetDownloaderplugin_init.cpp.o" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
