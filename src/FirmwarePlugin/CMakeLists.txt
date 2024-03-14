add_subdirectory(APM)
add_subdirectory(PX4)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_library(FirmwarePlugin STATIC
	CameraMetaData.cc
	CameraMetaData.h
	FirmwarePlugin.cc
	FirmwarePlugin.h
	FirmwarePluginManager.cc
	FirmwarePluginManager.h

	APM/APMFirmwarePlugin.cc
	APM/APMFirmwarePlugin.h
	APM/APMFirmwarePluginFactory.cc
	APM/APMFirmwarePluginFactory.h
	APM/APMParameterMetaData.cc
	APM/APMParameterMetaData.h
	APM/ArduCopterFirmwarePlugin.cc
	APM/ArduCopterFirmwarePlugin.h
	APM/ArduPlaneFirmwarePlugin.cc
	APM/ArduPlaneFirmwarePlugin.h
	APM/ArduRoverFirmwarePlugin.cc
	APM/ArduRoverFirmwarePlugin.h
	APM/ArduSubFirmwarePlugin.cc
	APM/ArduSubFirmwarePlugin.h
	APM/APMResources.qrc

	PX4/px4_custom_mode.h
	PX4/PX4FirmwarePlugin.cc
	PX4/PX4FirmwarePlugin.h
	PX4/PX4FirmwarePluginFactory.cc
	PX4/PX4FirmwarePluginFactory.h
	PX4/PX4ParameterMetaData.cc
	PX4/PX4ParameterMetaData.h
	PX4/PX4Resources.qrc
)

target_link_libraries(FirmwarePlugin
	PRIVATE
		qgc
)

target_include_directories(FirmwarePlugin
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
        APM
)
