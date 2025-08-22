cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_SOURCE_DIR}/cmake
    ${CMAKE_SOURCE_DIR}/cmake/CPack
    ${CMAKE_SOURCE_DIR}/cmake/find-modules
    ${CMAKE_SOURCE_DIR}/cmake/install
    ${CMAKE_SOURCE_DIR}/cmake/install/CPack
    ${CMAKE_SOURCE_DIR}/cmake/modules
    ${CMAKE_SOURCE_DIR}/cmake/platform
)

include(Helpers)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Configuration types" FORCE)
endif()

set(CMAKE_REQUIRED_QUIET ON)
set(CMAKE_POLICY_VERSION_MINIMUM ${CMAKE_MINIMUM_REQUIRED_VERSION})

#######################################################
#                Custom Build Configuration
#######################################################

include(CustomOptions)

if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/custom")
    message(STATUS "QGC: Enabling custom build")
    set(QGC_CUSTOM_BUILD ON)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/custom/cmake")
    include(CustomOverrides)
endif()

if(NOT QGC_BUILD_TESTING)
    set(BUILD_TESTING OFF CACHE INTERNAL "" FORCE)
endif()

#######################################################
#                   Project Info
#######################################################

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
    # set(CMAKE_OSX_SYSROOT "iphoneos")
    if(QGC_MACOS_UNIVERSAL_BUILD)
        set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
    endif()
endif()

include(Git)

if(QGC_USE_CACHE)
    qgc_config_caching()
endif()

project(${QGC_APP_NAME}
    VERSION ${QGC_APP_VERSION}
    DESCRIPTION ${QGC_APP_DESCRIPTION}
    HOMEPAGE_URL "https://${QGC_ORG_DOMAIN}"
    LANGUAGES C CXX
)

#######################################################
#            CMake Configuration Options
#######################################################

include(GNUInstallDirs)
include(FetchContent)
include(CMakePrintHelpers)

if(EXISTS "${QGC_CPM_SOURCE_CACHE}")
    set(ENV{CPM_SOURCE_CACHE} "${QGC_CPM_SOURCE_CACHE}")
else()
    set(ENV{CPM_SOURCE_CACHE} "${CMAKE_BINARY_DIR}/cpm_modules")
endif()
include(CPM)

include(Toolchain)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

# https://cmake.org/cmake/help/latest/policy/CMP0168.html#policy:CMP0168
if(POLICY CMP0168)
    cmake_policy(SET CMP0168 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0168 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0141.html#policy:CMP0141
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0141 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0075.html#policy:CMP0075
if(POLICY CMP0075)
    cmake_policy(SET CMP0075 NEW)
    set(CMAKE_POLICY_DEFAULT_CMP0075 NEW)
endif()

#######################################################
#               Qt6 Configuration
#######################################################

find_package(Qt6
    ${QGC_QT_MINIMUM_VERSION}...${QGC_QT_MAXIMUM_VERSION}
    REQUIRED
    COMPONENTS
        Charts
        Concurrent
        Core
        Core5Compat
        Gui
        LinguistTools
        Location
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

qt_policy(
    SET QTP0001 NEW
    SET QTP0002 NEW
    SET QTP0003 NEW
    SET QTP0004 NEW
    SET QTP0005 NEW
)

#######################################################
#                Custom Build Configuration
#######################################################

if(QGC_CUSTOM_BUILD)
    add_subdirectory(custom)
endif()

#######################################################
#                QGroundControl Resources
#######################################################

# Note: Adding Resources to Library instead requires using Q_INIT_RESOURCE(qgcresources)

list(APPEND QGC_RESOURCES
    ${CMAKE_SOURCE_DIR}/qgcimages.qrc
    ${CMAKE_SOURCE_DIR}/qgcresources.qrc
)

list(APPEND QGC_RESOURCES
    ${CMAKE_SOURCE_DIR}/resources/InstrumentValueIcons/InstrumentValueIcons.qrc
)

#######################################################
#               QGroundControl Target
#######################################################

qt_add_executable(${CMAKE_PROJECT_NAME}
    WIN32
    MACOSX_BUNDLE
    ${QGC_RESOURCES}
)

if(WIN32)
    include(Windows)
elseif(APPLE)
    include(Apple)
elseif(ANDROID)
    include(Android)
endif()

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

add_subdirectory(src)
if(QGC_BUILD_TESTING)
    add_subdirectory(test)
    set_property(DIRECTORY test PROPERTY QT_EXCLUDE_FROM_TRANSLATION ON)
endif()

file(GLOB TS_SOURCES ${CMAKE_SOURCE_DIR}/translations/qgc_*.ts)
set_source_files_properties(${TS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_BINARY_DIR}/i18n")

qt_add_translations(${CMAKE_PROJECT_NAME}
    TS_FILES ${TS_SOURCES}
    RESOURCE_PREFIX "/"
    LUPDATE_OPTIONS -no-obsolete
    TS_FILE_DIR "${CMAKE_SOURCE_DIR}/translations"
    TS_FILE_BASE ${CMAKE_PROJECT_NAME}
)

qgc_set_qt_resource_alias(${CMAKE_SOURCE_DIR}/resources/qtquickcontrols2.conf)

qt_add_resources(${CMAKE_PROJECT_NAME} "qgcresources_cmake"
    PREFIX "/"
    FILES
        "${CMAKE_SOURCE_DIR}/resources/qtquickcontrols2.conf"
)

# cmake_print_variables(QT_ALL_PLUGIN_TYPES_FOUND_VIA_FIND_PACKAGE)
qt_import_plugins(${CMAKE_PROJECT_NAME}
    INCLUDE Qt6::QSvgPlugin
    EXCLUDE_BY_TYPE geoservices
    INCLUDE_BY_TYPE sqldrivers Qt6::QSQLiteDriverPlugin
    # INCLUDE_BY_TYPE styles Qt6::qtquickcontrols2basicstyleplugin Qt6::qtquickcontrols2basicstyleimplplugin
)

include(Install)

include(PrintSummary)
