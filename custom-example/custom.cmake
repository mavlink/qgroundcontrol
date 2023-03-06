message(STATUS "Adding Custom Plugin")

# you can disable usb camera
set(QGC_DISABLE_UVC OFF)

# disable video streaming
set(DISABLE_VIDEOSTREAMING OFF)
# disable video streaming
set(QGC_DISABLE_MAVLINK_INSPECTOR OFF )

# disable airmap
set(DISABLE_AIRMAP OFF)

# disable Ardupilot mavlink dialect
set(QGC_DISABLE_APM_MAVLINK ON)
# disable Ardupilot firmware plugin
set(QGC_DISABLE_APM_PLUGIN ON)
# disable PX4 firmware plugin
# set(QGC_DISABLE_PX4_PLUGIN ON)
# disable Ardupilot firmware plugin factory
set(QGC_DISABLE_APM_PLUGIN_FACTORY ON)
# disable PX4 firmware plugin factory
set(QGC_DISABLE_PX4_PLUGIN_FACTORY ON)

set(CUSTOMCLASS "CustomPlugin")
set(CUSTOMHEADER \"CustomPlugin.h\")

set(CUSTOM_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/custom/src)

include_directories(${CUSTOM_SRC_PATH})
include_directories(${CUSTOM_SRC_PATH}/FirmwarePlugin)
include_directories(${CUSTOM_SRC_PATH}/AutoPilotPlugin)

set(CUSTOM_SRC
    ${CUSTOM_SRC_PATH}/CustomPlugin.cc
    ${CUSTOM_SRC_PATH}/AutoPilotPlugin/CustomAutoPilotPlugin.cc
    ${CUSTOM_SRC_PATH}/FirmwarePlugin/CustomFirmwarePlugin.cc
    ${CUSTOM_SRC_PATH}/FirmwarePlugin/CustomFirmwarePluginFactory.cc
        )


set(CUSTOM_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/custom/custom.qrc)