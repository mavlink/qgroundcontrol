add_subdirectory(APM)
add_subdirectory(PX4)

find_package(Qt6 REQUIRED COMPONENTS Core Positioning)

qt_add_library(FirmwarePlugin STATIC
	CameraMetaData.cc
	CameraMetaData.h
	FirmwarePlugin.cc
	FirmwarePlugin.h
	FirmwarePluginManager.cc
	FirmwarePluginManager.h
)

target_link_libraries(FirmwarePlugin
    PRIVATE
        AutoPilotPlugins
        CommonAutoPilotPlugin
        Camera
        MissionManager
        qgc
        Utilities
        VehicleSetup
    PUBLIC
    	Qt6::Core
        Qt6::Positioning
        comm
        FactSystem
        FollowMe
)

target_include_directories(FirmwarePlugin PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
