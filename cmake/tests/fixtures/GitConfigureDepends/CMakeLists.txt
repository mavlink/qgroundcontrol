cmake_minimum_required(VERSION 3.25)

project(QGCGitConfigureDepends LANGUAGES NONE)

if(NOT QGC_MODULE_DIR)
    message(FATAL_ERROR "QGC_MODULE_DIR is required")
endif()

include("${QGC_MODULE_DIR}/modules/Git.cmake")

get_property(
    _depends
    DIRECTORY
    PROPERTY CMAKE_CONFIGURE_DEPENDS
)
file(WRITE "${CMAKE_BINARY_DIR}/configure-depends.txt" "${_depends}")
# The version string, the project version and the Android versionCode's Dev field.
file(WRITE "${CMAKE_BINARY_DIR}/versions.txt" "${QGC_APP_VERSION_STR} ${QGC_APP_VERSION} ${QGC_APP_VERSION_DEV}")
