# The following options can be overriden by custom builds using the CustomOverrides.cmake file
set(QGC_APP_NAME "QGroundControl" CACHE STRING "App Name")
set(QGC_APP_COPYRIGHT "Copyright (c) 2024 QGroundControl. All rights reserved." CACHE STRING "Copyright")
set(QGC_APP_DESCRIPTION "Open Source Ground Control App" CACHE STRING "Description")
set(QGC_ORG_NAME "QGroundControl.org" CACHE STRING "Org Name")
set(QGC_ORG_DOMAIN "org.qgroundcontrol" CACHE STRING "Domain")

# MacOS
set(QGC_BUNDLE_ID "org.qgroundcontrol.QGroundControl" CACHE STRING "MacOS Bundle ID")
set(QGC_MACOS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/mac" CACHE PATH "MacOS Icon Path")

# APM
option(QGC_DISABLE_APM_MAVLINK "Disable APM Dialect" OFF)
option(QGC_DISABLE_APM_PLUGIN "Disable APM Plugin" OFF)
option(QGC_DISABLE_APM_PLUGIN_FACTORY "Disable APM Plugin Factory" OFF)

# PX4
option(QGC_DISABLE_PX4_PLUGIN "Disable PX4 Plugin" OFF)
option(QGC_DISABLE_PX4_PLUGIN_FACTORY "Disable PX4 Plugin Factory" OFF)
