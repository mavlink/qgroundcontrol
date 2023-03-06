add_subdirectory(APM)

if (NOT QGC_DISABLE_PX4_PLUGIN_FACTORY)
	set(PX4FirmwarePluginFactory
		PX4/PX4FirmwarePluginFactory.cc
		)
endif()

if (NOT QGC_DISABLE_PX4_PLUGIN)

	set(PX4FirmwarePlugin
		PX4/PX4FirmwarePlugin.cc
		PX4/PX4ParameterMetaData.cc
		PX4/PX4Resources.qrc
			)
endif()

if (NOT QGC_DISABLE_APM_PLUGIN_FACTORY)
	set(APMFirmwarePluginFactory
		APM/APMFirmwarePluginFactory.cc
		)
endif()

if (NOT QGC_DISABLE_APM_PLUGIN)
	set(APMFirmwarePlugin
		APM/APMFirmwarePlugin.cc
		APM/APMParameterMetaData.cc
		APM/ArduCopterFirmwarePlugin.cc
		APM/ArduPlaneFirmwarePlugin.cc
		APM/ArduRoverFirmwarePlugin.cc
		APM/ArduSubFirmwarePlugin.cc
		APM/APMResources.qrc
		)
endif()

add_library(FirmwarePlugin
	CameraMetaData.cc
	FirmwarePlugin.cc
	FirmwarePluginManager.cc

	${APMFirmwarePlugin}
	${APMFirmwarePluginFactory}

	${PX4FirmwarePluginFactory}
	${PX4FirmwarePlugin}
)

target_link_libraries(FirmwarePlugin
	PRIVATE
		qgc
)

target_include_directories(FirmwarePlugin
	INTERFACE
		${CMAKE_CURRENT_SOURCE_DIR}
		)

if (NOT QGC_DISABLE_APM_PLUGIN)
	target_include_directories(FirmwarePlugin
			INTERFACE
		APM)
endif()

if (NOT QGC_DISABLE_PX4_PLUGIN)
	target_include_directories(FirmwarePlugin
			INTERFACE
		PX4
	)
endif()
