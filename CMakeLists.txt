cmake_minimum_required(VERSION 3.21.1 FATAL_ERROR)

#######################################################
#                   Project Info
#######################################################

project(QGroundControl LANGUAGES C CXX)
# set_target_properties(${PROJECT_NAME} PROPERTIES VERSION 1.0)

#######################################################
#            CMake Configuration Options
#######################################################
# Setting configuration options and conditionally adding compile options

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel;Coverage")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/staging)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(FeatureSummary)

# CMake build type
# Debug Release RelWithDebInfo MinSizeRel Coverage
if (NOT CMAKE_BUILD_TYPE)
    # default to release with debug symbols
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Add folder where are supportive functions
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Stable or daily build option
option(STABLE_BUILD "Stable build option" OFF)

if (STABLE_BUILD)
    message(STATUS "Stable Build")
else ()
    message(STATUS "Daily Build")
    add_definitions(-DDAILY_BUILD)
endif ()

include(CMakeDependentOption)
cmake_dependent_option(BUILD_TESTING "Enable testing" ON "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)

#######################################################
#               OS Configuration
#######################################################
if (LINUX)
    # if USE_WAYLAND == 0, use X11 instead of Wayland
    set(USE_WAYLAND 1)
endif ()

#######################################################
#               Qt6 Configuration
#######################################################

option(QT6_DISABLE_DNSENGINE "Disable DNS Engine" OFF)

# Compilation error for deprecated functions
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x060600)
add_compile_definitions(QT_DEBUG_FIND_PACKAGE=ON)
if (QT6_DISABLE_DNSENGINE)
    message(STATUS "DNS Engine disabled")
    add_compile_definitions(QT6_DISABLE_DNSENGINE=ON)
endif ()
# Configure Qt6 to get necessary variables
include(Qt6QGCConfiguration)
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${QT_VERSION}")
message(STATUS "Qt spec: ${QT_MKSPEC}")


# Find Qt6 libraries
find_package(Qt6
        REQUIRED
        COMPONENTS
                Bluetooth
                Charts
                Concurrent
                Core
                Core5Compat
                Location
                Multimedia
                Network
                Positioning
                Quick
                QuickControls2
                QuickWidgets
                OpenGL
                Sql
                Svg
                Test
                TextToSpeech
                Widgets
                Xml
        OPTIONAL_COMPONENTS
                SerialPort
        HINTS
                ${QT_LIBRARY_HINTS}
)

qt_standard_project_setup(REQUIRES 6.6.0)

# this is required since user can have Qt5 and Qt6 installed at the same time, and then Qt_DIR
# might point to Qt5, but we want to use Qt6
# especially qmdnsengine uses Qt_DIR to choose Qt version
if (Qt6_DIR)
    set(QT_DIR ${Qt6_DIR})
endif ()

qt_policy(
    SET QTP0001 NEW
    SET QTP0002 NEW
)

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH "Install path for QML" FORCE)
set(QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml" CACHE STRING "Extra QML Import Paths" FORCE)

#######################################################
#                QML Configuration
#######################################################

# Pre-compile QML files using the Qt Quick compiler.
option(COMPILE_QML "Pre-compile QML files using the Qt Quick compiler." FALSE)
add_feature_info(COMPILE_QML COMPILE_QML "Pre-compile QML files using the Qt Quick compiler.")
if(COMPILE_QML)
    find_package(Qt6QuickCompiler)
    set_package_properties(Qt6QuickCompiler PROPERTIES
            DESCRIPTION "Pre-compile QML files using the Qt Quick compiler."
            TYPE OPTIONAL
    )
endif()

# Enable QML debugging/profiling support.
option(DEBUG_QML "Build QGroundControl with QML debugging/profiling support." FALSE)
add_feature_info(DEBUG_QML DEBUG_QML "Build QGroundControl with QML debugging/profiling support.")
if(DEBUG_QML)
    message(STATUS "To enable the QML debugger/profiler, run with: '-qmljsdebugger=port:1234'")
    add_definitions(-DQMLJSDEBUGGER)
    add_definitions(-DQT_QML_DEBUG)
endif()


#######################################################
#               QGroundControl Info
#######################################################
# Setting organization related definitions

set(COMPANY "Mavlink")
set(COPYRIGHT "Copyright (c) 2018 QGroundControl. All rights reserved.")
set(IDENTIFIER "io.mavlink.qgroundcontrol")

add_definitions(
        -DQGC_APPLICATION_NAME="QGroundControl"
        -DQGC_ORG_NAME="QGroundControl.org"
        -DQGC_ORG_DOMAIN="org.qgroundcontrol"
)

#######################################################
#                QGroundControl Git Information
#######################################################

include(Git)
message(STATUS "QGroundControl version: ${APP_VERSION_STR}")

#######################################################
#                GStreamer Configuration
#######################################################

option(ENABLE_VIDEOSTREAMING "Enable video streaming" ON)
if (ENABLE_VIDEOSTREAMING)
    message(STATUS "Enabling video streaming support")
    find_package(PkgConfig)

    set(GST_DEPENDENCIES
            gstreamer-1.0>=1.20
            gstreamer-video-1.0>=1.20
            gstreamer-gl-1.0>=1.20
    )

    if (LINUX OR ANDROID)
        set(GST_DEPENDENCIES ${GST_DEPENDENCIES} egl)
    endif ()

    pkg_check_modules(GST
            ${GST_DEPENDENCIES}
    )

    message(STATUS "GStreamer libs: ${GST_LIBRARIES}")
    message(STATUS "GStreamer include dirs: ${GST_INCLUDE_DIRS}")
    message(STATUS "GStreamer link dirs: ${GST_LIBRARY_DIRS}")
    message(STATUS "GStreamer cflags: ${GST_CFLAGS}")
    message(STATUS "GStreamer ldflags: ${GST_LDFLAGS}")
    message(STATUS "GStreamer libs: ${GST_LIBS}")

    if (LINUX OR ANDROID)
        message(STATUS "GStreamer egl libs: ${GST_EGL_LIBRARIES}")
        message(STATUS "GStreamer egl include dirs: ${GST_EGL_INCLUDE_DIRS}")
        message(STATUS "GStreamer egl link dirs: ${GST_EGL_LIBRARY_DIRS}")
        message(STATUS "GStreamer egl cflags: ${GST_EGL_CFLAGS}")
        message(STATUS "GStreamer egl ldflags: ${GST_EGL_LDFLAGS}")
        message(STATUS "GStreamer egl libs: ${GST_EGL_LIBS}")
    endif ()

    message(STATUS "gst found ${GST_FOUND}")
endif ()

option(QGC_AIRLINK_DISABLED "Enable airlink" OFF)

# Sets the default flags for compilation and linking.
include(CompileOptions)

#######################################################
#                QGroundControl Resources
#######################################################
include_directories(
        libs/libevents

        libs/mavlink/include/mavlink/v2.0
        libs/mavlink/include/mavlink/v2.0/all
        libs/mavlink/include/mavlink/v2.0/common

        libs/shapelib
)

add_subdirectory(libs)
add_subdirectory(src)

set(QGC_RESOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/qgcimages.qrc
        ${CMAKE_CURRENT_SOURCE_DIR}/qgcresources.qrc
        ${CMAKE_CURRENT_SOURCE_DIR}/qgroundcontrol.qrc
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/InstrumentValueIcons/InstrumentValueIcons.qrc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/APM/APMResources.qrc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/FirmwarePlugin/PX4/PX4Resources.qrc
)

if (WIN32)
    # append application icon resource for Windows
    set(QGC_RESOURCES
            ${QGC_RESOURCES}
            ${CMAKE_SOURCE_DIR}/deploy/windows/QGroundControl.rc)
endif()

if(CONFIG_UTM_ADAPTER)
    list(APPEND QGC_RESOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/UTMSP/utmsp.qrc
    )
else()
    list(APPEND QGC_RESOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/UTMSP/dummy/utmsp_dummy.qrc
    )
endif()

if (BUILD_TESTING)
    list(APPEND QGC_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/test/UnitTest.qrc)
endif()

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE macx.icns)
    set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/macx.icns")
    set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    list(APPEND QGC_RESOURCES
            ${app_icon_macos}
    )
endif()

#######################################################
#               QGroundControl Target
#######################################################

qt_add_executable(${PROJECT_NAME} ${QGC_RESOURCES})

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
elseif(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(ANDROID)
    add_android_openssl_libraries(${PROJECT_NAME})

    #set_target_properties(${PROJECT_NAME}
    #    PROPERTIES
    #        QT_ANDROID_ABIS ${ANDROID_ABI}
    #        QT_ANDROID_EXTRA_LIBS "${ANDROID_EXTRA_LIBS_APP}"
    #        QT_ANDROID_MIN_SDK_VERSION 26
    #        QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android/deploy
    #        QT_ANDROID_SDK_BUILD_TOOLS_REVISION 34.0.0
    #        QT_ANDROID_TARGET_SDK_VERSION 33
    #        QT_ANDROID_VERSION_NAME '0.1'
    #        QT_ANDROID_VERSION_CODE 0.1
    #        QT_QML_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/ui/qml # ${CMAKE_BINARY_DIR}/qml
    #)
    #QT_ANDROID_DEPLOY_RELEASE
    #QT_ANDROID_KEYSTORE_PATH
    #QT_ANDROID_KEYSTORE_ALIAS
    #QT_ANDROID_KEYSTORE_STORE_PASS
    #QT_ANDROID_KEYSTORE_KEY_PASS
    #QT_ANDROID_SIGN_APK
    #QT_ENABLE_VERBOSE_DEPLOYMENT
    #QT_HOST_PATH
    #QT_PATH_ANDROID_ABI_arm64-v8a
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE qgc)

#######################################################
#               Testing Configuration
#######################################################

if(BUILD_TESTING)
    add_subdirectory(test)
    target_link_libraries(qgc PRIVATE qgctest)
endif()

#######################################################
#		Install Configuration
#######################################################

include(GNUInstallDirs)

# Files/directories to install
#install(
#    TARGETS ${PROJECT_NAME}
#    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
#)
install(
        TARGETS ${PROJECT_NAME}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
        DIRECTORY ${CMAKE_SOURCE_DIR}/resources/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/qgroundcontrol
)
install(
        FILES ${CMAKE_SOURCE_DIR}/deploy/org.mavlink.qgroundcontrol.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(
        FILES ${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps/
        RENAME org.mavlink.qgroundcontrol.png
)
configure_file(
        ${CMAKE_SOURCE_DIR}/deploy/org.mavlink.qgroundcontrol.metainfo.xml.in
        ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
        @ONLY
)
install(
        FILES ${CMAKE_BINARY_DIR}/metainfo/org.mavlink.qgroundcontrol.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo/
)
if(NOT ANDROID)
    qt_generate_deploy_qml_app_script(TARGET ${PROJECT_NAME} OUTPUT_SCRIPT deploy_script)
    install(SCRIPT ${deploy_script})
endif()

#######################################################
#               Serial Port Configuration
#######################################################
if(NOT QT_MKSPEC MATCHES "winrt")
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
            Qt6::SerialPort
        )
endif()

#include(QGCDeploy)
