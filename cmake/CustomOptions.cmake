# The following options can be overriden by custom builds using the CustomOverrides.cmake file

# General
set(QGC_APP_NAME "QGroundControl" CACHE STRING "App Name")
set(QGC_APP_COPYRIGHT "Copyright (c) 2024 QGroundControl. All rights reserved." CACHE STRING "Copyright")
set(QGC_APP_DESCRIPTION "Open Source Ground Control App" CACHE STRING "Description")
set(QGC_ORG_NAME "QGroundControl.org" CACHE STRING "Org Name")
set(QGC_ORG_DOMAIN "org.qgroundcontrol" CACHE STRING "Domain")

option(QGC_STABLE_BUILD "Stable Build" OFF)
option(QGC_DOWNLOAD_DEPENDENCIES "Download Dependencies if Possible" ON)

option(QGC_ENABLE_BLUETOOTH "Enable Bluetooth Links" ON) # Qt6Bluetooth_FOUND
option(QGC_ZEROCONF_ENABLED "Enable ZeroConf Compatibility" OFF)
option(QGC_AIRLINK_DISABLED "Disable AIRLink" ON)
option(QGC_NO_SERIAL_LINK "Disable Serial Links" OFF) # NOT IOS AND Qt6SerialPort_FOUND

option(QGC_UTM_ADAPTER "Enable UTM Adapter" OFF)
option(QGC_VIEWER3D "Enable Viewer3D" ON) # Qt6Quick3D_FOUND

option(QGC_ENABLE_UVC "Enable UVC Devices" ON) # Qt6Multimedia_FOUND
option(QGC_ENABLE_GST_VIDEOSTREAMING "Enable GStreamer Video Backend" ON)
option(QGC_ENABLE_QT_VIDEOSTREAMING "Enable QtMultimedia Video Backend" OFF) # Qt6Multimedia_FOUND

set(QGC_MAVLINK_GIT_REPO "https://github.com/mavlink/c_library_v2.git" CACHE STRING "URL to MAVLink Git Repo")
set(QGC_MAVLINK_GIT_TAG "b71f061a53941637cbcfc5bcf860f96bc82e0892" CACHE STRING "Tag of MAVLink Git Repo")

# Android
set(QGC_QT_ANDROID_MIN_SDK_VERSION "28" CACHE STRING "Android Min SDK Version")
set(QGC_QT_ANDROID_TARGET_SDK_VERSION "35" CACHE STRING "Android Target SDK Version")
set(QGC_ANDROID_PACKAGE_NAME "org.mavlink.qgroundcontrol" CACHE STRING "Android Package Name")
set(QGC_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android" CACHE PATH "Android Package Path")

# MacOS
set(QGC_BUNDLE_ID "org.qgroundcontrol.QGroundControl" CACHE STRING "MacOS Bundle ID") # MACOS
set(QGC_MACOS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/macos" CACHE PATH "MacOS Icon Path") # MACOS

# Linux
set(QGC_APPIMAGE_ICON_PATH "${CMAKE_SOURCE_DIR}/resources/icons/qgroundcontrol.png" CACHE FILEPATH "AppImage Icon Path")

# Windows
set(QGC_WINDOWS_INSTALL_HEADER_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/installheader.bmp" CACHE FILEPATH "Windows Install Header Path")
set(QGC_WINDOWS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/WindowsQGC.ico" CACHE FILEPATH "Windows Icon Path")

# APM
option(QGC_DISABLE_APM_MAVLINK "Disable APM Dialect" OFF)
option(QGC_DISABLE_APM_PLUGIN "Disable APM Plugin" OFF)
option(QGC_DISABLE_APM_PLUGIN_FACTORY "Disable APM Plugin Factory" OFF)

# PX4
option(QGC_DISABLE_PX4_PLUGIN "Disable PX4 Plugin" OFF)
option(QGC_DISABLE_PX4_PLUGIN_FACTORY "Disable PX4 Plugin Factory" OFF)

# If you need to make an incompatible changes to stored settings, bump this version number
# up by 1. This will caused store settings to be cleared on next boot.
set(QGC_SETTINGS_VERSION "9" CACHE STRING "Settings Version")

set(CPM_SOURCE_CACHE ${CMAKE_BINARY_DIR}/cpm_modules CACHE PATH "Directory to download CPM dependencies")
