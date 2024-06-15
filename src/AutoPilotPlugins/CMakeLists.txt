add_subdirectory(Common)
if(NOT QGC_DISABLE_APM_PLUGIN)
    add_subdirectory(APM)
endif()
if(NOT QGC_DISABLE_PX4_PLUGIN)
	add_subdirectory(PX4)
endif()

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
        QGC
        Vehicle
        VehicleSetup
	PUBLIC
		Qt6::Core
)

target_include_directories(AutoPilotPlugins
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		Generic
)
