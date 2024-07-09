message(STATUS "Adding Custom Plugin")
find_package(Qt6 REQUIRED COMPONENTS Core)

# Build a single flight stack by disabling APM support
set(QGC_DISABLE_APM_MAVLINK ON PARENT_SCOPE)
set(QGC_DISABLE_APM_PLUGIN ON PARENT_SCOPE)
set(QGC_DISABLE_APM_PLUGIN_FACTORY ON PARENT_SCOPE)

# We implement our own PX4 plugin factory
set(QGC_DISABLE_PX4_PLUGIN_FACTORY ON PARENT_SCOPE)

# Enable custom build
set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
    APPEND PROPERTY COMPILE_DEFINITIONS
    QGC_CUSTOM_BUILD
    CUSTOMHEADER="CustomPlugin.h"
    CUSTOMCLASS=CustomPlugin
)

# Our own, custom resources
list(APPEND CUSTOM_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/qgcimages.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/qgcresources.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/qgroundcontrol.qrc
    ${CMAKE_CURRENT_SOURCE_DIR}/custom.qrc
)
set(QGC_RESOURCES ${CUSTOM_RESOURCES} PARENT_SCOPE)

set(QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/res"
    CACHE STRING "Qt Creator 4.1 extra qml import paths" PARENT_SCOPE)

qt_add_library(custom STATIC
    src/CustomPlugin.cc
    src/CustomPlugin.h
    src/AutoPilotPlugin/CustomAutoPilotPlugin.cc
    src/AutoPilotPlugin/CustomAutoPilotPlugin.h
    src/FirmwarePlugin/CustomFirmwarePlugin.cc
    src/FirmwarePlugin/CustomFirmwarePlugin.h
    src/FirmwarePlugin/CustomFirmwarePluginFactory.cc
    src/FirmwarePlugin/CustomFirmwarePluginFactory.h
)

target_link_libraries(custom
    PRIVATE
        FirmwarePlugin
        PX4AutoPilotPlugin
        PX4FirmwarePlugin
        Settings
        Vehicle
    PUBLIC
        Qt6::Core
        QGC
)

target_include_directories(custom
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/AutoPilotPlugin
)
