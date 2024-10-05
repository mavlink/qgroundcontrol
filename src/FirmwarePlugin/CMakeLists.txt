if(NOT QGC_DISABLE_APM_PLUGIN)
    add_subdirectory(APM)
endif()
if(NOT QGC_DISABLE_PX4_PLUGIN)
    add_subdirectory(PX4)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

qt_add_library(FirmwarePlugin STATIC
    CameraMetaData.cc
    CameraMetaData.h
    FirmwarePlugin.cc
    FirmwarePlugin.h
    FirmwarePluginFactory.cc
    FirmwarePluginFactory.h
    FirmwarePluginManager.cc
    FirmwarePluginManager.h
    $<$<NOT:$<BOOL:${QGC_DISABLE_APM_PLUGIN_FACTORY}>>:APM/APMFirmwarePluginFactory.cc>
    $<$<NOT:$<BOOL:${QGC_DISABLE_APM_PLUGIN_FACTORY}>>:APM/APMFirmwarePluginFactory.h>
    $<$<NOT:$<BOOL:${QGC_DISABLE_PX4_PLUGIN_FACTORY}>>:PX4/PX4FirmwarePluginFactory.cc>
    $<$<NOT:$<BOOL:${QGC_DISABLE_PX4_PLUGIN_FACTORY}>>:PX4/PX4FirmwarePluginFactory.h>
)

target_link_libraries(FirmwarePlugin
    PRIVATE
        AutoPilotPlugins
        Camera
        CommonAutoPilotPlugin
        MissionManager
        QGC
        Utilities
        VehicleSetup
        $<$<NOT:$<BOOL:${QGC_DISABLE_APM_PLUGIN}>>:APMFirmwarePlugin>
        $<$<NOT:$<BOOL:${QGC_DISABLE_PX4_PLUGIN}>>:PX4FirmwarePlugin>
    PUBLIC
        Qt6::Core
        Qt6::Positioning
        FactSystem
        FollowMe
        MAVLink
)

target_include_directories(FirmwarePlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# if(QGC_DISABLE_APM_MAVLINK)
#     target_compile_definitions(FirmwarePlugin PUBLIC NO_ARDUPILOT_DIALECT)
# endif()
