find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(APM_RESOURCES)
qt_add_resources(APM_RESOURCES APMResources.qrc)
qt_add_library(APMFirmwarePlugin STATIC
    APMFirmwarePlugin.cc
    APMFirmwarePlugin.h
    # APMFirmwarePluginFactory.cc
    # APMFirmwarePluginFactory.h
    APMParameterMetaData.cc
    APMParameterMetaData.h
    ArduCopterFirmwarePlugin.cc
    ArduCopterFirmwarePlugin.h
    ArduPlaneFirmwarePlugin.cc
    ArduPlaneFirmwarePlugin.h
    ArduRoverFirmwarePlugin.cc
    ArduRoverFirmwarePlugin.h
    ArduSubFirmwarePlugin.cc
    ArduSubFirmwarePlugin.h
    ${APM_RESOURCES}
)

target_link_libraries(APMFirmwarePlugin
    PRIVATE
        APMAutoPilotPlugin
        AutoPilotPlugins
        MissionManager
        QGC
        Settings
        Utilities
        Vehicle
        VehicleSetup
    PUBLIC
        Qt6::Core
        Qt6::Network
        Comms
        FactSystem
        FollowMe
        FirmwarePlugin
        MAVLink
)

target_include_directories(APMFirmwarePlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# file(GLOB QML_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.qml)
# # file(GLOB QML_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/res/*/*.*)
# qt_add_qml_module(APMFirmwarePlugin
#     URI QGroundControl.APMFirmwarePlugin
#     VERSION 1.0
#     # RESOURCES ${QML_RESOURCES}
#     QML_FILES ${QML_SOURCES}
#     OUTPUT_TARGETS APMFirmwarePlugin_targets
#     IMPORT_PATH ${QT_QML_OUTPUT_DIRECTORY}
#     IMPORTS
#         QtQuick
#         QtQuick.Layouts

#         QGroundControl
#         QGroundControl.Controls
#         QGroundControl.MultiVehicleManager
#         QGroundControl.ScreenTools
#         QGroundControl.Palette
#         QGroundControl.FactSystem
#         QGroundControl.FactControls
#         MAVLink
# )
