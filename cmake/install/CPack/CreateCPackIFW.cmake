# ============================================================================
# CreateCPackIFW.cmake
# Qt Installer Framework (IFW) package generator for cross-platform installers
# ============================================================================

include(CreateCPackCommon)

# ----------------------------------------------------------------------------
# Qt Installer Framework Detection
# ----------------------------------------------------------------------------
# CPackIFW searches PATH by default. Add a Qt online-installer root hint only
# when the environment provides one; an empty QT_ROOT_DIR must not become '/'.
if(DEFINED ENV{QT_ROOT_DIR})
    if(NOT "$ENV{QT_ROOT_DIR}" STREQUAL "")
        get_filename_component(_qgc_ifw_root "$ENV{QT_ROOT_DIR}/../.." ABSOLUTE)
        set(CPACK_IFW_ROOT "${_qgc_ifw_root}")
        set(QTIFWDIR "${_qgc_ifw_root}")
    endif()
endif()

include(CPackIFW)

# ----------------------------------------------------------------------------
# IFW Generator Configuration
# ----------------------------------------------------------------------------
set(CPACK_GENERATOR "IFW")
set(CPACK_BINARY_IFW ON)

# Debug output
set(CPACK_IFW_VERBOSE ON)

# ----------------------------------------------------------------------------
# Package Appearance
# ----------------------------------------------------------------------------
set(CPACK_IFW_PACKAGE_TITLE "${CMAKE_PROJECT_NAME}")
set(CPACK_IFW_PACKAGE_PUBLISHER "${QGC_ORG_NAME}")
set(CPACK_IFW_PRODUCT_URL "${CMAKE_PROJECT_HOMEPAGE_URL}")
set(CPACK_IFW_PACKAGE_ICON "${QGC_APP_ICON}")
set(CPACK_IFW_PACKAGE_WINDOW_ICON "${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png")
set(CPACK_IFW_PACKAGE_LOGO "${CMAKE_SOURCE_DIR}/resources/QGCLogoFull.svg")
set(CPACK_IFW_PACKAGE_WATERMARK "${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png")
set(CPACK_IFW_PACKAGE_WIZARD_STYLE "Modern")

# ----------------------------------------------------------------------------
# Platform-Specific Install Directories
# ----------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CPACK_IFW_TARGET_DIRECTORY "@HomeDir@/${CMAKE_PROJECT_NAME}")
    set(CPACK_IFW_ADMIN_TARGET_DIRECTORY "@HomeDir@/${CMAKE_PROJECT_NAME}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CPACK_IFW_TARGET_DIRECTORY "@HomeDir@\\${CMAKE_PROJECT_NAME}")
    set(CPACK_IFW_ADMIN_TARGET_DIRECTORY "@HomeDir@\\${CMAKE_PROJECT_NAME}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CPACK_IFW_TARGET_DIRECTORY "@ApplicationsDir@/${CMAKE_PROJECT_NAME}")
    set(CPACK_IFW_ADMIN_TARGET_DIRECTORY "@ApplicationsDir@/${CMAKE_PROJECT_NAME}")
endif()

cpack_ifw_configure_component(
    Runtime ESSENTIAL FORCED_INSTALLATION
    NAME "${CMAKE_PROJECT_NAME}"
    VERSION "${CMAKE_PROJECT_VERSION}"
    DESCRIPTION "Welcome to the ${CMAKE_PROJECT_NAME} installer."
    LICENSES "GPL LICENSE" "${CPACK_RESOURCE_FILE_LICENSE}"
    SCRIPT "${CMAKE_SOURCE_DIR}/deploy/installer/packages/org.mavlink.qgroundcontrol/meta/installscript.js"
)

include(CPackIFWConfigureFile)

include(CPack)
