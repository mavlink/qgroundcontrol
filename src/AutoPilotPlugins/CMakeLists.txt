add_subdirectory(APM)
add_subdirectory(Common)
add_subdirectory(PX4)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(AutoPilotPlugins STATIC
	AutoPilotPlugin.cc
	AutoPilotPlugin.h
	Generic/GenericAutoPilotPlugin.cc
	Generic/GenericAutoPilotPlugin.h
)

target_link_libraries(AutoPilotPlugins
    PRIVATE
        FirmwarePlugin
        qgc
        Vehicle
	PUBLIC
		Qt6::Core
        VehicleSetup
)

target_include_directories(AutoPilotPlugins
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		Generic
)
