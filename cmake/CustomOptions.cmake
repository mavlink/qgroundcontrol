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
if(APPLE AND NOT IOS)
    # Still haven't figured out how to package GStreamer with the app.
    option(QGC_ENABLE_GST_VIDEOSTREAMING "Enable GStreamer Video Backend" OFF)
else()
    option(QGC_ENABLE_GST_VIDEOSTREAMING "Enable GStreamer Video Backend" ON)
endif()
option(QGC_ENABLE_QT_VIDEOSTREAMING "Enable QtMultimedia Video Backend" OFF) # Qt6Multimedia_FOUND

set(QGC_MAVLINK_GIT_REPO "https://github.com/mavlink/c_library_v2.git" CACHE STRING "URL to MAVLink Git Repo")
set(QGC_MAVLINK_GIT_TAG "b71f061a53941637cbcfc5bcf860f96bc82e0892" CACHE STRING "Tag of MAVLink Git Repo")

# Android
set(QGC_QT_ANDROID_MIN_SDK_VERSION "28" CACHE STRING "Android Min SDK Version")
set(QGC_QT_ANDROID_TARGET_SDK_VERSION "35" CACHE STRING "Android Target SDK Version")
set(QGC_ANDROID_PACKAGE_NAME "org.mavlink.qgroundcontrol" CACHE STRING "Android Package Name")

# MacOS
set(QGC_BUNDLE_ID "org.qgroundcontrol.QGroundControl" CACHE STRING "MacOS Bundle ID") # MACOS
set(QGC_MACOS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/mac" CACHE PATH "MacOS Icon Path") # MACOS

# APM
option(QGC_DISABLE_APM_MAVLINK "Disable APM Dialect" OFF)
option(QGC_DISABLE_APM_PLUGIN "Disable APM Plugin" OFF)
option(QGC_DISABLE_APM_PLUGIN_FACTORY "Disable APM Plugin Factory" OFF)

# PX4
option(QGC_DISABLE_PX4_PLUGIN "Disable PX4 Plugin" OFF)
option(QGC_DISABLE_PX4_PLUGIN_FACTORY "Disable PX4 Plugin Factory" OFF)

