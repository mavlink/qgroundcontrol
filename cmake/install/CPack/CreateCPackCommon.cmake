# ============================================================================
# CreateCPackCommon.cmake
# Common CPack configuration shared across all package generators
# ============================================================================

include_guard(GLOBAL)

# ----------------------------------------------------------------------------
# Basic Package Information
# ----------------------------------------------------------------------------
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "${QGC_ORG_NAME}")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION
    "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}"
)
set(CPACK_PACKAGE_DESCRIPTION "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "${PROJECT_HOMEPAGE_URL}")
# Generic maintainer contact; DEB/RPM generators inherit this when their own
# maintainer field is unset (CPackDeb warns without it).
set(CPACK_PACKAGE_CONTACT "Dronecode <dev@dronecode.org>")

# ----------------------------------------------------------------------------
# Package Files and Directories
# ----------------------------------------------------------------------------
if(NOT DEFINED CPACK_SYSTEM_NAME OR CPACK_SYSTEM_NAME STREQUAL "")
    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
endif()
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png")
if(APPLE)
    set(QGC_APP_ICON "${CMAKE_SOURCE_DIR}/deploy/macos/qgroundcontrol.icns")
elseif(WIN32)
    set(QGC_APP_ICON "${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.ico")
else()
    set(QGC_APP_ICON "${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png")
endif()
set(CPACK_PACKAGE_CHECKSUM "SHA256")

# ----------------------------------------------------------------------------
# Resource Files
# ----------------------------------------------------------------------------
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/.github/COPYING.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# ----------------------------------------------------------------------------
# Package Options
# ----------------------------------------------------------------------------
set(CPACK_PACKAGE_EXECUTABLES "${CMAKE_PROJECT_NAME};${CMAKE_PROJECT_NAME}")
set(CPACK_VERBATIM_VARIABLES ON)
set(CPACK_THREADS 0)

# Exclude dependency development files and the separate AppImage component.
set(CPACK_COMPONENTS_ALL Runtime)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
