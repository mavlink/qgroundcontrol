# The following options can be overriden by custom builds using the CustomOverrides.cmake file
set(QGC_APP_NAME "QGroundControl")
set(QGC_APP_COPYRIGHT "Copyright (c) 2024 QGroundControl. All rights reserved.")
set(QGC_APP_DESCRIPTION "Open Source Ground Control App")
set(QGC_ORG_NAME "QGroundControl.org")
set(QGC_ORG_DOMAIN "org.qgroundcontrol")

# MacOS
set(QGC_BUNDLE_ID "org.qgroundcontrol.QGroundControl")
set(QGC_MACOS_ICON_PATH "${CMAKE_SOURCE_DIR}/resources/icons")

# APM
option(QGC_DISABLE_APM_MAVLINK "Disable APM Dialect" OFF)
option(QGC_DISABLE_APM_PLUGIN "Disable APM Plugin" OFF)
option(QGC_DISABLE_APM_PLUGIN_FACTORY "Disable APM Plugin Factory" OFF)

# PX4
option(QGC_DISABLE_PX4_PLUGIN "Disable PX4 Plugin" OFF)
option(QGC_DISABLE_PX4_PLUGIN_FACTORY "Disable PX4 Plugin Factory" OFF)
