find_package(Qt6 REQUIRED COMPONENTS Core Network)

# set(APM_RESOURCES)
# qt_add_resources(APM_RESOURCES APMResources.qrc)
qt_add_library(APMFirmwarePlugin STATIC
    APMFirmwarePlugin.cc
    APMFirmwarePlugin.h
    APMFirmwarePluginFactory.cc
    APMFirmwarePluginFactory.h
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
    # ${APM_RESOURCES}
)

target_link_libraries(APMFirmwarePlugin
    PRIVATE
        APMAutoPilotPlugin
        AutoPilotPlugins
        MissionManager
        qgc
        Settings
        Utilities
        Vehicle
        VehicleSetup
    PUBLIC
        Qt6::Core
        Qt6::Network
        comm
        FactSystem
        FollowMe
        FirmwarePlugin
)

target_include_directories(APMFirmwarePlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
