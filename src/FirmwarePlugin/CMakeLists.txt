add_subdirectory(APM)

add_library(FirmwarePlugin
	CameraMetaData.cc
	FirmwarePlugin.cc
	FirmwarePluginManager.cc

	APM/APMFirmwarePlugin.cc
	APM/APMFirmwarePluginFactory.cc
	APM/APMParameterMetaData.cc
	APM/ArduCopterFirmwarePlugin.cc
	APM/ArduPlaneFirmwarePlugin.cc
	APM/ArduRoverFirmwarePlugin.cc
	APM/ArduSubFirmwarePlugin.cc
	APM/APMResources.qrc

	PX4/PX4FirmwarePlugin.cc
	PX4/PX4FirmwarePluginFactory.cc
	PX4/PX4ParameterMetaData.cc
	PX4/PX4Resources.qrc
)

target_link_libraries(FirmwarePlugin
	PRIVATE
		qgc
)

target_include_directories(FirmwarePlugin
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
                APM
	)

