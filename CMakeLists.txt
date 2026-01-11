# ============================================================================
# QGroundControl CMake Build Configuration
# ============================================================================

cmake_minimum_required(VERSION 3.25)

# ----------------------------------------------------------------------------
# CMake Module Paths
# ----------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    "${CMAKE_SOURCE_DIR}/cmake/CPack"
    "${CMAKE_SOURCE_DIR}/cmake/find-modules"
    "${CMAKE_SOURCE_DIR}/cmake/install"
    "${CMAKE_SOURCE_DIR}/cmake/install/CPack"
    "${CMAKE_SOURCE_DIR}/cmake/modules"
    "${CMAKE_SOURCE_DIR}/cmake/platform"
)

# Include helper functions early
include(Helpers)

# ----------------------------------------------------------------------------
# Default Build Type
# ----------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug or Release)" FORCE)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available configuration types" FORCE)
    message(STATUS "No build type specified. Defaulting to Release.")
endif()

# ----------------------------------------------------------------------------
# CMake Policy Configuration
# ----------------------------------------------------------------------------
set(CMAKE_REQUIRED_QUIET ON)
set(CMAKE_POLICY_VERSION_MINIMUM ${CMAKE_MINIMUM_REQUIRED_VERSION})

# ============================================================================
# Custom Build Configuration
# ============================================================================

# Load default options and allow custom builds to override
include(CustomOptions)

if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/custom")
    message(STATUS "QGC: Custom build directory detected")
    set(QGC_CUSTOM_BUILD ON)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/custom/cmake")
    include(CustomOverrides)
endif()

# Disable testing if not explicitly enabled
if(NOT QGC_BUILD_TESTING)
    set(BUILD_TESTING OFF CACHE INTERNAL "Disable testing by default" FORCE)
endif()

# ============================================================================
# Project Configuration
# ============================================================================

# ----------------------------------------------------------------------------
# Apple-Specific Pre-Project Settings
# ----------------------------------------------------------------------------
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
    # iOS builds: set(CMAKE_OSX_SYSROOT "iphoneos")
    if(QGC_MACOS_UNIVERSAL_BUILD)
        # Universal binary for both Intel and Apple Silicon
        set(CMAKE_OSX_ARCHITECTURES "x86_64h;arm64")
    endif()
endif()

# ----------------------------------------------------------------------------
# Git Integration & Version Extraction
# ----------------------------------------------------------------------------
include(Git)

# ----------------------------------------------------------------------------
# Compiler Caching Configuration
# ----------------------------------------------------------------------------
if(QGC_USE_CACHE)
    qgc_config_caching()
endif()

# ----------------------------------------------------------------------------
# Project Declaration
# ----------------------------------------------------------------------------
project(${QGC_APP_NAME}
    VERSION ${QGC_APP_VERSION}
    DESCRIPTION ${QGC_APP_DESCRIPTION}
    HOMEPAGE_URL "https://${QGC_ORG_DOMAIN}"
    LANGUAGES C CXX
)

# ============================================================================
# CMake & Build System Configuration
# ============================================================================

# Standard CMake modules
include(GNUInstallDirs)
include(FetchContent)
include(CMakePrintHelpers)

# ----------------------------------------------------------------------------
# CPM (CMake Package Manager) Configuration
# ----------------------------------------------------------------------------
include(CPM)
if(NOT CPM_SOURCE_CACHE)
    set(CPM_SOURCE_CACHE "${CMAKE_BINARY_DIR}/cpm_modules")
endif()

# ----------------------------------------------------------------------------
# Toolchain Configuration (Compiler, Linker, Build Settings)
# ----------------------------------------------------------------------------
include(Toolchain)

# ----------------------------------------------------------------------------
# Output Directories
# ----------------------------------------------------------------------------
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
if(ANDROID)
    set(CMAKE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
else()
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()

# ----------------------------------------------------------------------------
# CMake Policies
# ----------------------------------------------------------------------------
# CMP0168: Modern FetchContent integration
if(POLICY CMP0168)
    cmake_policy(SET CMP0168 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0168 NEW)
endif()

# CMP0141: MSVC debug information format
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0141 NEW)
endif()

# CMP0075: Include files in CheckIncludeFile
if(POLICY CMP0075)
    cmake_policy(SET CMP0075 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0075 NEW)
endif()

# ============================================================================
# Qt6 Configuration
# ============================================================================

set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(Qt6
    ${QGC_QT_MINIMUM_VERSION}...${QGC_QT_MAXIMUM_VERSION}
    REQUIRED
    COMPONENTS
        Charts
        Concurrent
        Core
        Core5Compat
        Gui
        HttpServer
        LinguistTools
        Location
        LocationPrivate
        Multimedia
        Network
        Positioning
        Qml
        QmlIntegration
        Quick
        QuickControls2
        QuickWidgets
        Sensors
        Sql
        Svg
        TextToSpeech
        Widgets
        Xml
        StateMachine
    OPTIONAL_COMPONENTS
        Bluetooth
        MultimediaQuickPrivate
        OpenGL
        Quick3D
        SerialPort
        Test
)

if(LINUX)
    find_package(Qt6 COMPONENTS WaylandClient)
endif()

qt_standard_project_setup(
    REQUIRES ${QGC_QT_MINIMUM_VERSION}
    SUPPORTS_UP_TO ${QGC_QT_MAXIMUM_VERSION}
    I18N_SOURCE_LANGUAGE en
)

# ----------------------------------------------------------------------------
# QML Linting Configuration
# ----------------------------------------------------------------------------
if(QGC_ENABLE_QMLLINT)
    set(QT_QML_GENERATE_QMLLINT ON CACHE BOOL "Enable automatic QML linting" FORCE)
else()
    set(QT_QML_GENERATE_QMLLINT OFF CACHE BOOL "Disable automatic QML linting" FORCE)
endif()

# ----------------------------------------------------------------------------
# Qt Policies (Enable modern Qt6 behaviors)
# ----------------------------------------------------------------------------
qt_policy(
    SET QTP0001 NEW  # Modern resource handling
    SET QTP0002 NEW  # New behavior for QML module URI
    SET QTP0003 NEW  # Better QML type registration
    SET QTP0004 NEW  # Improved translation handling
    SET QTP0005 NEW  # Enhanced deployment handling
)

# ============================================================================
# Custom Build Subdirectory
# ============================================================================

if(QGC_CUSTOM_BUILD)
    add_subdirectory(custom)
endif()

# ============================================================================
# QGroundControl Resources
# ============================================================================

# Note: Resources added to executable (not library) to avoid needing Q_INIT_RESOURCE()

list(APPEND QGC_RESOURCES
    "${CMAKE_SOURCE_DIR}/qgcimages.qrc"
    "${CMAKE_SOURCE_DIR}/qgcresources.qrc"
    "${CMAKE_SOURCE_DIR}/resources/InstrumentValueIcons/InstrumentValueIcons.qrc"
)

# ============================================================================
# QGroundControl Executable Target
# ============================================================================

# Create the main executable
qt_add_executable(${CMAKE_PROJECT_NAME}
    WIN32          # Windows GUI application
    MACOSX_BUNDLE  # macOS application bundle
    ${QGC_RESOURCES}
)

# ----------------------------------------------------------------------------
# Platform-Specific Configuration
# ----------------------------------------------------------------------------
if(WIN32)
    include(Windows)
elseif(APPLE)
    include(Apple)
elseif(ANDROID)
    include(Android)
elseif(LINUX)
    include(Linux)
endif()

# ----------------------------------------------------------------------------
# QML Module Configuration
# ----------------------------------------------------------------------------
qt_add_qml_module(${CMAKE_PROJECT_NAME}
    URI QGC
    VERSION 1.0
    RESOURCE_PREFIX /qml
    IMPORTS
        QtQuick
        QtQuick.Controls
        QtQuick.Dialogs
        QtQuick.Layouts
)

# ----------------------------------------------------------------------------
# Source & Test Subdirectories
# ----------------------------------------------------------------------------
add_subdirectory(src)

if(QGC_BUILD_TESTING)
    add_subdirectory(test)
    # Exclude test directory from translation scanning
    set_property(DIRECTORY test PROPERTY QT_EXCLUDE_FROM_TRANSLATION ON)
endif()

# ----------------------------------------------------------------------------
# Translation/Localization Configuration
# ----------------------------------------------------------------------------
file(GLOB TS_SOURCES "${CMAKE_SOURCE_DIR}/translations/qgc_*.ts")
set_source_files_properties(${TS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_BINARY_DIR}/i18n")

qt_add_translations(${CMAKE_PROJECT_NAME}
    TS_FILES ${TS_SOURCES}
    RESOURCE_PREFIX "/"
    LUPDATE_OPTIONS -no-obsolete
    TS_FILE_DIR "${CMAKE_SOURCE_DIR}/translations"
    TS_FILE_BASE ${CMAKE_PROJECT_NAME}
    # TS_FILES_OUTPUT_VARIABLE _ts_files_generated
    # TS_OUTPUT_DIRECTORY "{CMAKE_SOURCE_DIR}/translations"
    # QM_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/i18n"
    # MERGE_QT_TRANSLATIONS
)

# ----------------------------------------------------------------------------
# Qt Quick Controls Configuration
# ----------------------------------------------------------------------------
qgc_set_qt_resource_alias("${CMAKE_SOURCE_DIR}/resources/qtquickcontrols2.conf")

qt_add_resources(${CMAKE_PROJECT_NAME} "qgcresources_cmake"
    PREFIX "/"
    FILES "${CMAKE_SOURCE_DIR}/resources/qtquickcontrols2.conf"
)

# ----------------------------------------------------------------------------
# Qt Plugin Configuration
# ----------------------------------------------------------------------------
qt_import_plugins(${CMAKE_PROJECT_NAME}
    INCLUDE Qt6::QSvgPlugin
    EXCLUDE_BY_TYPE geoservices  # Use built-in location plugins
    INCLUDE_BY_TYPE sqldrivers Qt6::QSQLiteDriverPlugin
)

# Linux-specific Qt platform plugins
if(LINUX)
    qt_import_plugins(${CMAKE_PROJECT_NAME}
        INCLUDE
            Qt6::QWaylandIntegrationPlugin      # Wayland support
            Qt6::QXcbIntegrationPlugin          # X11 support
            Qt6::QEglFSIntegrationPlugin        # Embedded GL support
            Qt6::QWaylandEglPlatformIntegrationPlugin
    )
endif()

# ============================================================================
# Installation & Packaging
# ============================================================================

include(Install)

# ============================================================================
# Build Summary
# ============================================================================

include(PrintSummary)
