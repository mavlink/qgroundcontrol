if(NOT QGC_DISABLE_APM_PLUGIN)
    add_subdirectory(APM)
endif()
add_subdirectory(PX4)

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

if(NOT QGC_NO_ARDUPILOT_DIALECT)
    target_link_libraries(FirmwarePlugin PRIVATE APMFirmwarePlugin)
else()
    target_compile_definitions(FirmwarePlugin PUBLIC NO_ARDUPILOT_DIALECT)
endif()

if(NOT QGC_NO_PX4_DIALECT)
    target_link_libraries(FirmwarePlugin PRIVATE PX4FirmwarePlugin)
else()
    target_compile_definitions(FirmwarePlugin PUBLIC NO_PX4_DIALECT)
endif()
