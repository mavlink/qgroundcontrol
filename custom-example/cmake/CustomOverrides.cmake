set(QGC_APP_NAME "Custom-QGroundControl" CACHE STRING "App Name" FORCE)

set(QGC_MACOS_ICON_PATH "${CMAKE_SOURCE_DIR}/custom/res" CACHE PATH "MacOS Icon Path" FORCE)
set(QGC_APPIMAGE_ICON_PATH "${CMAKE_SOURCE_DIR}/custom/res/icons/custom_qgroundcontrol.png" CACHE FILEPATH "AppImage Icon Path" FORCE)

if(EXISTS ${CMAKE_SOURCE_DIR}/custom/deploy/windows/installheader.bmp)
    set(QGC_WINDOWS_INSTALL_HEADER_PATH "${CMAKE_SOURCE_DIR}/custom/deploy/windows/installheader.bmp" CACHE FILEPATH "Windows Install Header Path" FORCE)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/custom/deploy/windows/WindowsQGC.ico)
    set(QGC_WINDOWS_ICON_PATH "${CMAKE_SOURCE_DIR}/custom/deploy/windows/WindowsQGC.ico" CACHE FILEPATH "Windows Icon Path" FORCE)
endif()

# Build a single flight stack by disabling APM support
set(QGC_DISABLE_APM_MAVLINK ON CACHE BOOL "Disable APM Dialect" FORCE)
set(QGC_DISABLE_APM_PLUGIN ON CACHE BOOL "Disable APM Plugin" FORCE)
set(QGC_DISABLE_APM_PLUGIN_FACTORY ON CACHE BOOL "Disable APM Plugin Factory" FORCE)

# We implement our own PX4 plugin factory
set(QGC_DISABLE_PX4_PLUGIN_FACTORY ON CACHE BOOL "Disable PX4 Plugin Factory" FORCE)
