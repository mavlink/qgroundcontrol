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
)

target_link_libraries(FirmwarePlugin
    PRIVATE
        AutoPilotPlugins
        CommonAutoPilotPlugin
        Camera
        MissionManager
        QGC
        Utilities
        VehicleSetup
    PUBLIC
    	Qt6::Core
        Qt6::Positioning
        FactSystem
        FollowMe
        MAVLink
)

target_include_directories(FirmwarePlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Load These Firmware Plugins

if(NOT QGC_DISABLE_APM_PLUGIN)
    add_subdirectory(APM)
    target_link_libraries(FirmwarePlugin PRIVATE APMFirmwarePlugin)
endif()

if(NOT QGC_DISABLE_APM_PLUGIN_FACTORY)
    target_sources(FirmwarePlugin
        PRIVATE
            APM/APMFirmwarePluginFactory.cc
            APM/APMFirmwarePluginFactory.h
    )
endif()

if(NOT QGC_DISABLE_PX4_PLUGIN)
    add_subdirectory(PX4)
    target_link_libraries(FirmwarePlugin PRIVATE PX4FirmwarePlugin)
endif()

if(NOT QGC_DISABLE_PX4_PLUGIN_FACTORY)
    target_sources(FirmwarePlugin
        PRIVATE
            PX4/PX4FirmwarePluginFactory.cc
            PX4/PX4FirmwarePluginFactory.h
    )
endif()
