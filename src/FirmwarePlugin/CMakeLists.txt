
add_library(FirmwarePlugin
	APM/APMFirmwarePlugin.cc
	APM/APMFirmwarePluginFactory.cc
	APM/APMParameterMetaData.cc
	APM/ArduCopterFirmwarePlugin.cc
	APM/ArduPlaneFirmwarePlugin.cc
	APM/ArduRoverFirmwarePlugin.cc
	APM/ArduSubFirmwarePlugin.cc

	CameraMetaData.cc
	FirmwarePlugin.cc
	FirmwarePluginManager.cc

	PX4/PX4FirmwarePlugin.cc
	PX4/PX4FirmwarePluginFactory.cc
	PX4/PX4ParameterMetaData.cc

	# Resources
	APM/APMResources.qrc
	PX4/PX4Resources.qrc
)

target_link_libraries(FirmwarePlugin
	PUBLIC
		qgc
)

target_include_directories(FirmwarePlugin
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}
		APM
	)

