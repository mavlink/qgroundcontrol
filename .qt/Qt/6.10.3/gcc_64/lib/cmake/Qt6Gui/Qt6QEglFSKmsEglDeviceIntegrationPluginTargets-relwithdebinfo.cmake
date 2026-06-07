#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt6::QEglFSKmsEglDeviceIntegrationPlugin" for configuration "RelWithDebInfo"
set_property(TARGET Qt6::QEglFSKmsEglDeviceIntegrationPlugin APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt6::QEglFSKmsEglDeviceIntegrationPlugin PROPERTIES
  IMPORTED_COMMON_LANGUAGE_RUNTIME_RELWITHDEBINFO ""
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/plugins/egldeviceintegrations/libqeglfs-kms-egldevice-integration.so"
  IMPORTED_NO_SONAME_RELWITHDEBINFO "TRUE"
  )

list(APPEND _cmake_import_check_targets Qt6::QEglFSKmsEglDeviceIntegrationPlugin )
list(APPEND _cmake_import_check_files_for_Qt6::QEglFSKmsEglDeviceIntegrationPlugin "${_IMPORT_PREFIX}/plugins/egldeviceintegrations/libqeglfs-kms-egldevice-integration.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
