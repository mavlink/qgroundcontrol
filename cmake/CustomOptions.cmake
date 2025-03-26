include(CMakeDependentOption)
# The following options can be overriden by custom builds using the CustomOverrides.cmake file

# App
set(QGC_APP_NAME "QGroundControl" CACHE STRING "App Name")
set(QGC_APP_COPYRIGHT "Copyright (c) 2024 QGroundControl. All rights reserved." CACHE STRING "Copyright")
set(QGC_APP_DESCRIPTION "Open Source Ground Control App" CACHE STRING "Description")
set(QGC_ORG_NAME "QGroundControl.org" CACHE STRING "Org Name")
set(QGC_ORG_DOMAIN "org.qgroundcontrol" CACHE STRING "Domain")
set(QGC_SETTINGS_VERSION "9" CACHE STRING "Settings Version") # If you need to make an incompatible changes to stored settings, bump this version number up by 1. This will caused store settings to be cleared on next boot.

# Build
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(QGC_STABLE_BUILD "Stable Build" OFF)
option(QGC_USE_CACHE "Use Build Caching" ON)
cmake_dependent_option(QGC_BUILD_TESTING "Enable testing" ON "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
cmake_dependent_option(QGC_DEBUG_QML "Build QGroundControl with QML debugging/profiling support." OFF "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)

# Features
option(QGC_UTM_ADAPTER "Enable UTM Adapter" OFF)
option(QGC_VIEWER3D "Enable Viewer3D" ON) # Qt6Quick3D_FOUND
# option(QGC_DISABLE_MAVLINK_INSPECTOR "Disable Mavlink Inspector" OFF) # This removes QtCharts which is GPL licensed

# Comms
option(QGC_ENABLE_BLUETOOTH "Enable Bluetooth Links" ON) # Qt6Bluetooth_FOUND
option(QGC_ZEROCONF_ENABLED "Enable ZeroConf Compatibility" OFF)
option(QGC_AIRLINK_DISABLED "Disable AIRLink" ON)
option(QGC_NO_SERIAL_LINK "Disable Serial Links" OFF) # NOT IOS AND Qt6SerialPort_FOUND

# Video
option(QGC_ENABLE_UVC "Enable UVC Devices" ON) # Qt6Multimedia_FOUND
option(QGC_ENABLE_GST_VIDEOSTREAMING "Enable GStreamer Video Backend" ON)
option(QGC_ENABLE_QT_VIDEOSTREAMING "Enable QtMultimedia Video Backend" OFF) # Qt6Multimedia_FOUND

# MAVLink
set(QGC_MAVLINK_GIT_REPO "https://github.com/mavlink/c_library_v2.git" CACHE STRING "URL to MAVLink Git Repo")
set(QGC_MAVLINK_GIT_TAG "7ea034366ee7f09f3991a5b82f51f0c259023b38" CACHE STRING "Tag of MAVLink Git Repo")

# APM
option(QGC_DISABLE_APM_MAVLINK "Disable APM Dialect" OFF)
option(QGC_DISABLE_APM_PLUGIN "Disable APM Plugin" OFF)
option(QGC_DISABLE_APM_PLUGIN_FACTORY "Disable APM Plugin Factory" OFF)

# PX4
option(QGC_DISABLE_PX4_PLUGIN "Disable PX4 Plugin" OFF)
option(QGC_DISABLE_PX4_PLUGIN_FACTORY "Disable PX4 Plugin Factory" OFF)

# Android
if(QT_VERSION VERSION_GREATER_EQUAL 6.7.0)
    set(QGC_QT_ANDROID_MIN_SDK_VERSION "28" CACHE STRING "Android Min SDK Version")
else() # Allow building for Android 7.1 if supported
    set(QGC_QT_ANDROID_MIN_SDK_VERSION "25" CACHE STRING "Android Min SDK Version")
endif()
set(QGC_QT_ANDROID_TARGET_SDK_VERSION "35" CACHE STRING "Android Target SDK Version")
set(QGC_ANDROID_PACKAGE_NAME "org.mavlink.qgroundcontrol" CACHE STRING "Android Package Name")
set(QGC_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android" CACHE PATH "Android Package Path")
set(QT_ANDROID_DEPLOYMENT_TYPE "" CACHE STRING "Forces Signing if Set to Release")
option(QT_ANDROID_SIGN_APK "Enable Signing APK" OFF)
option(QT_ANDROID_SIGN_AAB "Enable Signing AAB" OFF)
option(QT_USE_TARGET_ANDROID_BUILD_DIR "Use Target Android Build Dir" OFF)

# MacOS
set(QGC_BUNDLE_ID "org.qgroundcontrol.QGroundControl" CACHE STRING "MacOS Bundle ID") # MACOS
set(QGC_MACOS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/macos" CACHE PATH "MacOS Icon Path") # MACOS

# Linux
set(QGC_APPIMAGE_ICON_PATH "${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png" CACHE FILEPATH "AppImage Icon Path")

# Windows
set(QGC_WINDOWS_INSTALL_HEADER_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/installheader.bmp" CACHE FILEPATH "Windows Install Header Path")
set(QGC_WINDOWS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/WindowsQGC.ico" CACHE FILEPATH "Windows Icon Path")

# CPM
set(CPM_SOURCE_CACHE ${CMAKE_BINARY_DIR}/cpm_modules CACHE PATH "Directory to download CPM dependencies")

# Qt
set(QGC_QT_MINIMUM_VERSION "6.6.3" CACHE STRING "Minimum Supported Qt Version")
set(QGC_QT_MAXIMUM_VERSION "6.8.3" CACHE STRING "Maximum Supported Qt Version")
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH "Install path for QML")
set(QML_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}" CACHE STRING "Extra QML Import Paths")
option(QML_IMPORT_TRACE "Debug QML Imports" OFF)
option(QT_SILENCE_MISSING_DEPENDENCY_TARGET_WARNING "Silence Missing Dependency Warnings" OFF)
option(QT_ENABLE_VERBOSE_DEPLOYMENT "Verbose Deployment" OFF)
