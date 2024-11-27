# The following options can be overriden by custom builds using the CustomOverrides.cmake file

# General

set(QGC_APP_NAME "QGroundControl" CACHE STRING "App Name")
set(QGC_APP_COPYRIGHT "Copyright (c) 2024 QGroundControl. All rights reserved." CACHE STRING "Copyright")
set(QGC_APP_DESCRIPTION "Open Source Ground Control App" CACHE STRING "Description")
set(QGC_ORG_NAME "QGroundControl.org" CACHE STRING "Org Name")
set(QGC_ORG_DOMAIN "org.qgroundcontrol" CACHE STRING "Domain")

option(QGC_STABLE_BUILD "Stable Build" OFF)

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
set(QGC_MAVLINK_GIT_TAG "4db2f67156d996eae90ef437a73353468d850407" CACHE STRING "Tag of MAVLink Git Repo")

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

