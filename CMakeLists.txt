cmake_minimum_required(VERSION 3.2)

project(QGroundControl LANGUAGES CXX)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel;Coverage")

# CMake build type
# Debug Release RelWithDebInfo MinSizeRel Coverage
if (NOT CMAKE_BUILD_TYPE)
	# default to release with debug symbols
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

if(DEFINED ENV{QT_VERSION})
	set(QT_VERSION $ENV{QT_VERSION})
endif()

if(NOT QT_VERSION)
	# try Qt 5.11.0 if none specified
	set(QT_VERSION "5.11.1")
endif()

if(DEFINED ENV{QT_MKSPEC})
	set(QT_MKSPEC $ENV{QT_MKSPEC})
endif()

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

if(NOT QT_MKSPEC)
	if(APPLE)
		set(QT_MKSPEC clang_64)
	elseif(LINUX)
		set(QT_MKSPEC gcc_64)
	elseif(WIN32)
		set(QT_MKSPEC msvc2017_64)
		#set(QT_MKSPEC winrt_x64_msvc2017)
	endif()
endif()

# Add folder where are supportive functions
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${QT_VERSION}")
message(STATUS "Qt spec: ${QT_MKSPEC}")

set(COMPANY "Mavlink")
set(COPYRIGHT "Copyright (c) 2018 QGroundControl. All rights reserved.")
set(IDENTIFIER "io.mavlink.qgroundcontrol")

execute_process(
	COMMAND git submodule update --init
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)

execute_process(
	COMMAND git describe --always --tags
	OUTPUT_VARIABLE git_tag
	OUTPUT_STRIP_TRAILING_WHITESPACE
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)

add_definitions(
	-DQGC_APPLICATION_NAME="QGroundControl"
	-DQGC_ORG_NAME="QGroundControl.org"
	-DQGC_ORG_DOMAIN="org.qgroundcontrol"
	)

message(STATUS "QGroundControl version: ${git_tag}")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#=============================================================================
# ccache
#
option(CCACHE "Use ccache if available" ON)
find_program(CCACHE_PROGRAM ccache)
if (CCACHE AND CCACHE_PROGRAM AND NOT DEFINED ENV{CCACHE_DISABLE})
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

#=============================================================================
# Qt5
#
find_package(Qt5 ${QT_VERSION}
	COMPONENTS
		Bluetooth
		Concurrent
		Core
		Location
		Multimedia
		Network
		Positioning
		Quick
		QuickWidgets
		Sql
		Svg
		Test
		TextToSpeech
		Widgets
		Xml
	REQUIRED
	HINTS
		$ENV{HOME}/Qt/${QT_VERSION}/${QT_MKSPEC}
		$ENV{QT_PATH}/${QT_VERSION}/${QT_MKSPEC}
		C:/Qt
)

if(NOT QT_MKSPEC MATCHES "winrt")
	find_package(Qt5 ${QT_VERSION}
		COMPONENTS
			SerialPort
		REQUIRED
		HINTS
			$ENV{HOME}/Qt/${QT_VERSION}/${QT_MKSPEC}
			$ENV{QT_PATH}/${QT_VERSION}/${QT_MKSPEC}
			C:/Qt
	)
endif()

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
	include(CTest)
	enable_testing()
	add_definitions(-DUNITTEST_BUILD)
endif()

if(LINUX)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS})
	string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES) # work around for cmake warning
elseif(APPLE)
	include_directories(libs/lib/Frameworks/SDL2.framework/Headers)
elseif(WIN32)
	include_directories(libs/lib/sdl2/msvc/include)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	# clang and AppleClang
	add_compile_options(
		-Wall
		-Wextra
		-Werror

		-frtti

		-Wno-address-of-packed-member # ignore for mavlink
	)

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# GCC
	if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9)
		add_compile_options(-fdiagnostics-color=always)
	endif()

	add_compile_options(
		-Wall
		-Wextra
		-Werror

		-frtti
	)
	
elseif (WIN32)

	add_definitions(-D_USE_MATH_DEFINES)
	add_compile_options(
		/wd4244 # warning C4244: '=': conversion from 'double' to 'float', possible loss of data
		)

endif()


set(QGC_SRC)
list(APPEND QGC_SRC

	libs/qwt/qwt_abstract_legend.cpp
	libs/qwt/qwt_abstract_scale.cpp
	libs/qwt/qwt_abstract_scale_draw.cpp
	libs/qwt/qwt_abstract_slider.cpp
	libs/qwt/qwt_analog_clock.cpp
	libs/qwt/qwt_arrow_button.cpp
	libs/qwt/qwt_clipper.cpp
	libs/qwt/qwt_color_map.cpp
	libs/qwt/qwt_compass.cpp
	libs/qwt/qwt_compass_rose.cpp
	libs/qwt/qwt_counter.cpp
	libs/qwt/qwt_curve_fitter.cpp
	libs/qwt/qwt_dial.cpp
	libs/qwt/qwt_dial_needle.cpp
	libs/qwt/qwt_dyngrid_layout.cpp
	libs/qwt/qwt_event_pattern.cpp
	libs/qwt/qwt_graphic.cpp
	libs/qwt/qwt_interval.cpp
	libs/qwt/qwt_knob.cpp
	libs/qwt/qwt_legend.cpp
	libs/qwt/qwt_legend_data.cpp
	libs/qwt/qwt_legend_label.cpp
	libs/qwt/qwt_magnifier.cpp
	libs/qwt/qwt_math.cpp
	libs/qwt/qwt_null_paintdevice.cpp
	libs/qwt/qwt_painter.cpp
	libs/qwt/qwt_painter_command.cpp
	libs/qwt/qwt_panner.cpp
	libs/qwt/qwt_picker.cpp
	libs/qwt/qwt_picker_machine.cpp
	libs/qwt/qwt_pixel_matrix.cpp
	libs/qwt/qwt_plot.cpp
	libs/qwt/qwt_plot_axis.cpp
	libs/qwt/qwt_plot_canvas.cpp
	libs/qwt/qwt_plot_curve.cpp
	libs/qwt/qwt_plot_dict.cpp
	libs/qwt/qwt_plot_grid.cpp
	libs/qwt/qwt_plot_item.cpp
	libs/qwt/qwt_plot_layout.cpp
	libs/qwt/qwt_plot_magnifier.cpp
	libs/qwt/qwt_plot_marker.cpp
	libs/qwt/qwt_plot_panner.cpp
	libs/qwt/qwt_plot_picker.cpp
	libs/qwt/qwt_plot_rasteritem.cpp
	libs/qwt/qwt_plot_scaleitem.cpp
	libs/qwt/qwt_plot_seriesitem.cpp
	libs/qwt/qwt_plot_spectrogram.cpp
	libs/qwt/qwt_plot_svgitem.cpp
	libs/qwt/qwt_plot_xml.cpp
	libs/qwt/qwt_plot_zoomer.cpp
	libs/qwt/qwt_point_data.cpp
	libs/qwt/qwt_point_mapper.cpp
	libs/qwt/qwt_raster_data.cpp
	libs/qwt/qwt_round_scale_draw.cpp
	libs/qwt/qwt_scale_div.cpp
	libs/qwt/qwt_scale_draw.cpp
	libs/qwt/qwt_scale_engine.cpp
	libs/qwt/qwt_scale_map.cpp
	libs/qwt/qwt_scale_widget.cpp
	libs/qwt/qwt_series_data.cpp
	libs/qwt/qwt_slider.cpp
	libs/qwt/qwt_spline.cpp
	libs/qwt/qwt_symbol.cpp
	libs/qwt/qwt_text.cpp
	libs/qwt/qwt_text_engine.cpp
	libs/qwt/qwt_text_label.cpp
	libs/qwt/qwt_thermo.cpp
	libs/qwt/qwt_transform.cpp
	libs/qwt/qwt_wheel.cpp
	libs/qwt/qwt_widget_overlay.cpp

	src/AnalyzeView/ExifParser.cc
	src/AnalyzeView/GeoTagController.cc
	src/AnalyzeView/LogDownloadController.cc
	src/AnalyzeView/MavlinkConsoleController.cc
	src/AnalyzeView/PX4LogParser.cc
	src/AnalyzeView/ULogParser.cc

	src/api/QGCCorePlugin.cc
	src/api/QGCOptions.cc
	src/api/QGCSettings.cc
	src/api/QmlComponentInfo.cc

	src/Audio/AudioOutput.cc

	src/AutoPilotPlugins/APM/APMAirframeComponent.cc
	src/AutoPilotPlugins/APM/APMAirframeComponentAirframes.cc
	src/AutoPilotPlugins/APM/APMAirframeComponentController.cc
	src/AutoPilotPlugins/APM/APMAirframeLoader.cc
	src/AutoPilotPlugins/APM/APMAutoPilotPlugin.cc
	src/AutoPilotPlugins/APM/APMCameraComponent.cc
	src/AutoPilotPlugins/APM/APMCompassCal.cc
	src/AutoPilotPlugins/APM/APMFlightModesComponent.cc
	src/AutoPilotPlugins/APM/APMFlightModesComponentController.cc
	src/AutoPilotPlugins/APM/APMLightsComponent.cc
	src/AutoPilotPlugins/APM/APMMotorComponent.cc
	src/AutoPilotPlugins/APM/APMPowerComponent.cc
	src/AutoPilotPlugins/APM/APMRadioComponent.cc
	src/AutoPilotPlugins/APM/APMSafetyComponent.cc
	src/AutoPilotPlugins/APM/APMSensorsComponent.cc
	src/AutoPilotPlugins/APM/APMSensorsComponentController.cc
	src/AutoPilotPlugins/APM/APMSubFrameComponent.cc
	src/AutoPilotPlugins/APM/APMTuningComponent.cc
	src/AutoPilotPlugins/APM/APMHeliComponent.cc

	src/AutoPilotPlugins/Common/ESP8266Component.cc
	src/AutoPilotPlugins/Common/ESP8266ComponentController.cc
	src/AutoPilotPlugins/Common/MotorComponent.cc
	src/AutoPilotPlugins/Common/RadioComponentController.cc
	src/AutoPilotPlugins/Common/SyslinkComponent.cc
	src/AutoPilotPlugins/Common/SyslinkComponentController.cc

	src/AutoPilotPlugins/PX4/AirframeComponent.cc
	src/AutoPilotPlugins/PX4/AirframeComponentAirframes.cc
	src/AutoPilotPlugins/PX4/AirframeComponentController.cc
	src/AutoPilotPlugins/PX4/CameraComponent.cc
	src/AutoPilotPlugins/PX4/FlightModesComponent.cc
	src/AutoPilotPlugins/PX4/PowerComponent.cc
	src/AutoPilotPlugins/PX4/PowerComponentController.cc
	src/AutoPilotPlugins/PX4/PX4AdvancedFlightModesController.cc
	src/AutoPilotPlugins/PX4/PX4AirframeLoader.cc
	src/AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc
	src/AutoPilotPlugins/PX4/PX4RadioComponent.cc
	src/AutoPilotPlugins/PX4/PX4SimpleFlightModesController.cc
	src/AutoPilotPlugins/PX4/PX4TuningComponent.cc
	src/AutoPilotPlugins/PX4/SafetyComponent.cc
	src/AutoPilotPlugins/PX4/SensorsComponent.cc
	src/AutoPilotPlugins/PX4/SensorsComponentController.cc

	src/AutoPilotPlugins/AutoPilotPlugin.cc
	src/AutoPilotPlugins/Generic/GenericAutoPilotPlugin.cc

	src/Camera/QGCCameraControl.cc
	src/Camera/QGCCameraIO.cc
	src/Camera/QGCCameraManager.cc

	#src/comm/BluetoothLink.cc
	src/comm/LinkConfiguration.cc
	src/comm/LinkInterface.cc
	src/comm/LinkManager.cc
	src/comm/LogReplayLink.cc
	src/comm/MavlinkMessagesTimer.cc
	src/comm/MAVLinkProtocol.cc
	src/comm/QGCFlightGearLink.cc
	src/comm/QGCJSBSimLink.cc
	src/comm/QGCMAVLink.cc
	src/comm/QGCSerialPortInfo.cc
	src/comm/QGCXPlaneLink.cc
	src/comm/SerialLink.cc
	src/comm/TCPLink.cc
	src/comm/UDPLink.cc

	src/FactSystem/Fact.cc
	src/FactSystem/FactControls/FactPanelController.cc
	src/FactSystem/FactGroup.cc
	src/FactSystem/FactMetaData.cc
	src/FactSystem/FactSystem.cc
	src/FactSystem/FactValueSliderListModel.cc
	src/FactSystem/ParameterManager.cc
	src/FactSystem/SettingsFact.cc

	src/FirmwarePlugin/APM/APMFirmwarePlugin.cc
	src/FirmwarePlugin/APM/APMFirmwarePluginFactory.cc
	src/FirmwarePlugin/APM/APMParameterMetaData.cc
	src/FirmwarePlugin/APM/ArduCopterFirmwarePlugin.cc
	src/FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.cc
	src/FirmwarePlugin/APM/ArduRoverFirmwarePlugin.cc
	src/FirmwarePlugin/APM/ArduSubFirmwarePlugin.cc

	src/FirmwarePlugin/CameraMetaData.cc
	src/FirmwarePlugin/FirmwarePlugin.cc
	src/FirmwarePlugin/FirmwarePluginManager.cc

	src/FirmwarePlugin/PX4/PX4FirmwarePlugin.cc
	src/FirmwarePlugin/PX4/PX4FirmwarePluginFactory.cc
	src/FirmwarePlugin/PX4/PX4ParameterMetaData.cc

	src/FlightDisplay/VideoManager.cc

	src/FlightMap/Widgets/ValuesWidgetController.cc

	src/FollowMe/FollowMe.cc

	src/GPS/Drivers/src/ashtech.cpp
	src/GPS/Drivers/src/gps_helper.cpp
	src/GPS/Drivers/src/mtk.cpp
	src/GPS/Drivers/src/rtcm.cpp
	src/GPS/Drivers/src/ubx.cpp
	src/GPS/GPSManager.cc
	src/GPS/GPSProvider.cc
	src/GPS/RTCM/RTCMMavlink.cc

	src/Joystick/Joystick.cc
	src/Joystick/JoystickManager.cc
	src/Joystick/JoystickSDL.cc

	src/MissionManager/CameraCalc.cc
	src/MissionManager/CameraSection.cc
	src/MissionManager/CameraSpec.cc
	src/MissionManager/ComplexMissionItem.cc
	src/MissionManager/CorridorScanComplexItem.cc
	src/MissionManager/FixedWingLandingComplexItem.cc
	src/MissionManager/GeoFenceController.cc
	src/MissionManager/GeoFenceManager.cc
	src/MissionManager/KML.cc
	src/MissionManager/MissionCommandList.cc
	src/MissionManager/MissionCommandTree.cc
	src/MissionManager/MissionCommandUIInfo.cc
	src/MissionManager/MissionController.cc
	src/MissionManager/MissionItem.cc
	src/MissionManager/MissionManager.cc
	src/MissionManager/MissionSettingsItem.cc
	src/MissionManager/PlanElementController.cc
	src/MissionManager/PlanManager.cc
	src/MissionManager/PlanMasterController.cc
	src/MissionManager/QGCFenceCircle.cc
	src/MissionManager/QGCFencePolygon.cc
	src/MissionManager/QGCMapCircle.cc
	src/MissionManager/QGCMapPolygon.cc
	src/MissionManager/QGCMapPolyline.cc
	src/MissionManager/RallyPoint.cc
	src/MissionManager/RallyPointController.cc
	src/MissionManager/RallyPointManager.cc
	src/MissionManager/SimpleMissionItem.cc
	src/MissionManager/SpeedSection.cc
	src/MissionManager/StructureScanComplexItem.cc
	src/MissionManager/SurveyComplexItem.cc
	src/MissionManager/TransectStyleComplexItem.cc
	src/MissionManager/VisualMissionItem.cc

	src/PositionManager/PositionManager.cpp
	src/PositionManager/SimulatedPosition.cc

	src/QmlControls/AppMessages.cc
	src/QmlControls/CoordinateVector.cc
	src/QmlControls/EditPositionDialogController.cc
	src/QmlControls/ParameterEditorController.cc
	src/QmlControls/QGCFileDialogController.cc
	src/QmlControls/QGCGeoBoundingCube.cc
	src/QmlControls/QGCImageProvider.cc
	src/QmlControls/QGroundControlQmlGlobal.cc
	src/QmlControls/QmlObjectListModel.cc
	src/QmlControls/QmlTestWidget.cc
	src/QmlControls/RCChannelMonitorController.cc
	src/QmlControls/ScreenToolsController.cc

	src/QtLocationPlugin/QGCMapEngine.cpp
	src/QtLocationPlugin/QGCMapTileSet.cpp
	src/QtLocationPlugin/QGCMapUrlEngine.cpp
	src/QtLocationPlugin/QGCTileCacheWorker.cpp
	src/QtLocationPlugin/QGeoCodeReplyQGC.cpp
	src/QtLocationPlugin/QGeoCodingManagerEngineQGC.cpp
	src/QtLocationPlugin/QGeoMapReplyQGC.cpp
	src/QtLocationPlugin/QGeoServiceProviderPluginQGC.cpp
	src/QtLocationPlugin/QGeoTiledMappingManagerEngineQGC.cpp
	src/QtLocationPlugin/QGeoTileFetcherQGC.cpp
	src/QtLocationPlugin/QMLControl/QGCMapEngineManager.cc

	src/Settings/AppSettings.cc
	src/Settings/AutoConnectSettings.cc
	src/Settings/BrandImageSettings.cc
	src/Settings/FlightMapSettings.cc
	src/Settings/GuidedSettings.cc
	src/Settings/RTKSettings.cc
	src/Settings/SettingsGroup.cc
	src/Settings/SettingsManager.cc
	src/Settings/UnitsSettings.cc
	src/Settings/VideoSettings.cc

	src/Terrain/TerrainQuery.cc

	src/uas/FileManager.cc
	src/uas/UAS.cc
	src/uas/UASMessageHandler.cc

	src/ui/HILDockWidget.cc
	src/ui/linechart/ChartPlot.cc
	src/ui/linechart/IncrementalPlot.cc
	src/ui/linechart/LinechartPlot.cc
	src/ui/linechart/Linecharts.cc
	src/ui/linechart/LinechartWidget.cc
	src/ui/linechart/Scrollbar.cc
	src/ui/linechart/ScrollZoomer.cc
	src/ui/MainWindow.cc
	src/ui/MAVLinkDecoder.cc
	src/ui/MultiVehicleDockWidget.cc
	src/ui/QGCHilConfiguration.cc
	src/ui/QGCHilFlightGearConfiguration.cc
	src/ui/QGCHilJSBSimConfiguration.cc
	src/ui/QGCHilXPlaneConfiguration.cc
	src/ui/QGCMapRCToParamDialog.cpp
	src/ui/QGCMAVLinkInspector.cc
	src/ui/QGCMAVLinkLogPlayer.cc
	src/ui/QGCPluginHost.cc
	src/ui/QGCUASFileView.cc
	src/ui/QGCUASFileViewMulti.cc
	src/ui/uas/QGCUnconnectedInfoWidget.cc

	src/Vehicle/ADSBVehicle.cc
	src/Vehicle/GPSRTKFactGroup.cc
	src/Vehicle/MAVLinkLogManager.cc
	src/Vehicle/MultiVehicleManager.cc
	src/Vehicle/Vehicle.cc

	src/VehicleSetup/Bootloader.cc
	src/VehicleSetup/FirmwareImage.cc
	src/VehicleSetup/FirmwareUpgradeController.cc
	src/VehicleSetup/JoystickConfigController.cc
	src/VehicleSetup/PX4FirmwareUpgradeThread.cc
	src/VehicleSetup/VehicleComponent.cc

	src/VideoStreaming/VideoItem.cc
	src/VideoStreaming/VideoReceiver.cc
	src/VideoStreaming/VideoStreaming.cc
	src/VideoStreaming/VideoSurface.cc

	src/ViewWidgets/CustomCommandWidget.cc
	src/ViewWidgets/CustomCommandWidgetController.cc
	src/ViewWidgets/ViewWidgetController.cc

	src/CmdLineOptParser.cc
	src/JsonHelper.cc
	src/KMLFileHelper.cc
	src/LogCompressor.cc
	src/main.cc
	src/QGC.cc
	src/QGCApplication.cc
	src/QGCComboBox.cc
	src/QGCDockWidget.cc
	src/QGCFileDownload.cc
	src/QGCGeo.cc
	src/QGCLoggingCategory.cc
	src/QGCMapPalette.cc
	src/QGCPalette.cc
	src/QGCQFileDialog.cc
	src/QGCQGeoCoordinate.cc
	src/QGCQmlWidgetHolder.cpp
	src/QGCQuickWidget.cc
	src/QGCTemporaryFile.cc
	src/QGCToolbox.cc
	src/RunGuard.cc
	src/TerrainTile.cc
	src/UTM.cpp

	# HEADERS
	# shouldn't be listed here, but aren't named properly for AUTOMOC
	src/comm/QGCFlightGearLink.h
	src/comm/QGCHilLink.h
	src/comm/QGCJSBSimLink.h
	src/MissionManager/Section.h
	src/QtLocationPlugin/QGCMapEngineData.h
	src/uas/UAS.h
	src/uas/UASInterface.h
	src/ui/QGCHilFlightGearConfiguration.h
	src/ui/QGCHilJSBSimConfiguration.h
)

if(QGC_AIRMAP)

	list(APPEND QGC_SRC
		src/Airmap/AirMapAdvisoryManager.cc
		src/Airmap/AirMapFlightManager.cc
		src/Airmap/AirMapFlightPlanManager.cc
		src/Airmap/AirMapManager.cc
		src/Airmap/AirMapRestrictionManager.cc
		src/Airmap/AirMapRulesetsManager.cc
		src/Airmap/AirMapSettings.cc
		src/Airmap/AirMapSharedState.cc
		src/Airmap/AirMapTelemetry.cc
		src/Airmap/AirMapTrafficMonitor.cc
		src/Airmap/AirMapVehicleManager.cc
		src/Airmap/AirMapWeatherInfoManager.cc

		src/AirspaceManagement/AirspaceAdvisoryProvider.cc
		src/AirspaceManagement/AirspaceFlightPlanProvider.cc
		src/AirspaceManagement/AirspaceManager.cc
		src/AirspaceManagement/AirspaceRestriction.cc
		src/AirspaceManagement/AirspaceRestrictionProvider.cc
		src/AirspaceManagement/AirspaceRulesetsProvider.cc
		src/AirspaceManagement/AirspaceVehicleManager.cc
		src/AirspaceManagement/AirspaceWeatherInfoProvider.cc
	)
else()
	list(APPEND QGC_SRC
		src/Airmap/dummy/AirspaceManager.cc
	)
	include_directories(src/Airmap/dummy)
endif()

set_source_files_properties(${CMAKE_SOURCE_DIR}/src/QGCApplication.cc PROPERTIES COMPILE_DEFINITIONS GIT_VERSION="${git_tag}")

if(MOBILE)
	list(APPEND QGC_SRC
		src/MobileScreenMgr.cc
	)

	if (ANDROID)
		list(APPEND QGC_SRC
			libs/qtandroidserialport/src/qserialport.cpp
			libs/qtandroidserialport/src/qserialport_android.cpp
			libs/qtandroidserialport/src/qserialportinfo.cpp
			libs/qtandroidserialport/src/qserialportinfo_android.cpp

			src/Joystick/JoystickAndroid.cc
		)
	endif()
endif()

if(BUILD_TESTING)

	add_custom_target(check
		COMMAND ctest --output-on-failure .
		USES_TERMINAL
	)

	function (add_qgc_test test_name)
		add_test(
			NAME ${test_name}
			COMMAND $<TARGET_FILE:QGroundControl> --unittest:${test_name}
		)
		add_dependencies(check QGroundControl)
	endfunction()

	list(APPEND QGC_SRC
		src/AnalyzeView/LogDownloadTest.cc

		src/Audio/AudioOutputTest.cc

		src/comm/MockLink.cc
		src/comm/MockLinkFileServer.cc
		src/comm/MockLinkMissionItemHandler.cc

		src/FactSystem/FactSystemTestBase.cc
		src/FactSystem/FactSystemTestGeneric.cc
		src/FactSystem/FactSystemTestPX4.cc
		src/FactSystem/ParameterManagerTest.cc

		src/MissionManager/CameraCalcTest.cc
		src/MissionManager/CameraSectionTest.cc
		src/MissionManager/CorridorScanComplexItemTest.cc
		src/MissionManager/MissionCommandTreeTest.cc
		src/MissionManager/MissionControllerManagerTest.cc
		src/MissionManager/MissionControllerTest.cc
		src/MissionManager/MissionItemTest.cc
		src/MissionManager/MissionManagerTest.cc
		src/MissionManager/MissionSettingsTest.cc
		src/MissionManager/PlanMasterControllerTest.cc
		src/MissionManager/QGCMapPolygonTest.cc
		src/MissionManager/QGCMapPolylineTest.cc
		src/MissionManager/SectionTest.cc
		src/MissionManager/SimpleMissionItemTest.cc
		src/MissionManager/SpeedSectionTest.cc
		src/MissionManager/StructureScanComplexItemTest.cc
		src/MissionManager/SurveyComplexItemTest.cc
		src/MissionManager/TransectStyleComplexItemTest.cc
		src/MissionManager/VisualMissionItemTest.cc

		src/qgcunittest/FileDialogTest.cc
		src/qgcunittest/FileManagerTest.cc
		src/qgcunittest/FlightGearTest.cc
		src/qgcunittest/GeoTest.cc
		src/qgcunittest/LinkManagerTest.cc
		src/qgcunittest/MainWindowTest.cc
		src/qgcunittest/MavlinkLogTest.cc
		src/qgcunittest/MessageBoxTest.cc
		src/qgcunittest/MultiSignalSpy.cc
		src/qgcunittest/RadioConfigTest.cc
		src/qgcunittest/TCPLinkTest.cc
		src/qgcunittest/TCPLoopBackServer.cc
		src/qgcunittest/UnitTest.cc
		src/qgcunittest/UnitTestList.cc

		src/Vehicle/SendMavCommandTest.cc
	)

	add_qgc_test(AudioOutputTest)
	add_qgc_test(CameraCalcTest)
	add_qgc_test(CameraSectionTest)
	add_qgc_test(CorridorScanComplexItemTest)
	add_qgc_test(FactSystemTestGeneric)
	add_qgc_test(FactSystemTestPX4)
	add_qgc_test(FileDialogTest)
	add_qgc_test(FileManagerTest)
	add_qgc_test(FlightGearUnitTest)
	add_qgc_test(GeoTest)
	add_qgc_test(LinkManagerTest)
	add_qgc_test(LogDownloadTest)
	add_qgc_test(MessageBoxTest)
	add_qgc_test(MissionCommandTreeTest)
	add_qgc_test(MissionControllerTest)
	add_qgc_test(MissionItemTest)
	add_qgc_test(MissionManagerTest)
	add_qgc_test(MissionSettingsTest)
	add_qgc_test(ParameterManagerTest)
	add_qgc_test(PlanMasterControllerTest)
	add_qgc_test(QGCMapPolygonTest)
	add_qgc_test(QGCMapPolylineTest)
	add_qgc_test(RadioConfigTest)
	add_qgc_test(SendMavCommandTest)
	add_qgc_test(SimpleMissionItemTest)
	add_qgc_test(SpeedSectionTest)
	add_qgc_test(StructureScanComplexItemTest)
	add_qgc_test(SurveyComplexItemTest)
	add_qgc_test(TCPLinkTest)
	add_qgc_test(TransectStyleComplexItemTest)

endif()

if (VIDEO)
	list(APPEND QGC_SRC
		src/VideoStreaming/gstqtvideosink/delegates/basedelegate.cpp
		src/VideoStreaming/gstqtvideosink/delegates/qtquick2videosinkdelegate.cpp
		src/VideoStreaming/gstqtvideosink/delegates/qtvideosinkdelegate.cpp
		src/VideoStreaming/gstqtvideosink/delegates/qwidgetvideosinkdelegate.cpp
		src/VideoStreaming/gstqtvideosink/gstqtglvideosink.cpp
		src/VideoStreaming/gstqtvideosink/gstqtglvideosinkbase.cpp
		src/VideoStreaming/gstqtvideosink/gstqtquick2videosink.cpp
		src/VideoStreaming/gstqtvideosink/gstqtvideosink.cpp
		src/VideoStreaming/gstqtvideosink/gstqtvideosinkbase.cpp
		src/VideoStreaming/gstqtvideosink/gstqtvideosinkplugin.cpp
		src/VideoStreaming/gstqtvideosink/gstqwidgetvideosink.cpp
		src/VideoStreaming/gstqtvideosink/painters/genericsurfacepainter.cpp
		src/VideoStreaming/gstqtvideosink/painters/openglsurfacepainter.cpp
		src/VideoStreaming/gstqtvideosink/painters/videomaterial.cpp
		src/VideoStreaming/gstqtvideosink/painters/videonode.cpp
		src/VideoStreaming/gstqtvideosink/utils/bufferformat.cpp
		src/VideoStreaming/gstqtvideosink/utils/utils.cpp
	)
endif()

set(QGC_RESOURCES)
list(APPEND QGC_RESOURCES
	#HackAndroidFileDialog.qrc
	HackFileDialog.qrc
	qgcresources.qrc
	qgroundcontrol.qrc
	src/FirmwarePlugin/APM/APMResources.qrc
	src/FirmwarePlugin/PX4/PX4Resources.qrc

	src/Airmap/airmap.qrc
)

if(BUILD_TESTING)
	list(APPEND QGC_RESOURCES
		UnitTest.qrc
	)
endif()

# TODO: get qtquick compiler working
#qtquick_compiler_add_resources(QGC_RESOURCES ${QGC_RESOURCES}
#find_package(Qt5QuickCompiler)

set(QGC_UI)
list(APPEND QGC_UI
	#src/ui/Linechart.ui
	src/QGCQmlWidgetHolder.ui
	src/ui/MainWindow.ui
	src/ui/MultiVehicleDockWidget.ui
	src/ui/QGCHilConfiguration.ui
	src/ui/QGCHilFlightGearConfiguration.ui
	src/ui/QGCHilJSBSimConfiguration.ui
	src/ui/QGCHilXPlaneConfiguration.ui
	src/ui/QGCMapRCToParamDialog.ui
	src/ui/QGCMAVLinkInspector.ui
	src/ui/QGCMAVLinkLogPlayer.ui
	src/ui/QGCPluginHost.ui
	src/ui/QGCUASFileView.ui
	src/ui/QGCUASFileViewMulti.ui
	src/ui/QMap3D.ui
	src/ui/uas/QGCUnconnectedInfoWidget.ui
)

include_directories(

	src/QtLocationPlugin/QMLControl
	#src/QtLocationPlugin/qtlocation/include/QtLocation
	#src/QtLocationPlugin/qtlocation/include/QtLocation/5.5.1
	#src/QtLocationPlugin/qtlocation/include/QtPositioning/5.5.1
	${Qt5Location_PRIVATE_INCLUDE_DIRS}

	libs/eigen

	libs/mavlink/include/mavlink/v2.0
	libs/mavlink/include/mavlink/v2.0/ardupilotmega
	libs/mavlink/include/mavlink/v2.0/common

	libs/qwt

	src

	src/AnalyzeView
	src/api
	src/Audio
	src/AutoPilotPlugins
	src/AutoPilotPlugins/APM
	src/AutoPilotPlugins/Common
	src/AutoPilotPlugins/PX4
	src/Camera
	src/comm
	src/FactSystem
	src/FactSystem/FactControls
	src/FirmwarePlugin
	src/FirmwarePlugin/APM
	src/FlightDisplay
	src/FlightMap/Widgets
	src/FollowMe
	src/GPS
	src/Joystick
	src/MissionManager
	src/PositionManager
	src/qgcunittest
	src/QmlControls
	src/QtLocationPlugin
	src/Settings
	src/Terrain
	src/uas
	src/ui
	src/ui/linechart
	src/Vehicle
	src/VehicleSetup
	src/VideoStreaming
	src/ViewWidgets
)

if(ANDROID)
	add_library(QGroundControl SHARED ${QGC_SRC} ${QGC_RESOURCES} ${QGC_UI})
else()
	add_executable(QGroundControl ${QGC_SRC} ${QGC_RESOURCES} ${QGC_UI})
endif()

target_link_libraries(QGroundControl
	Qt5::Bluetooth
	Qt5::Concurrent
	Qt5::Core
	Qt5::Quick
	Qt5::QuickWidgets
	Qt5::Location
	Qt5::Multimedia
	Qt5::Positioning
	Qt5::Sql
	Qt5::Svg
	Qt5::Test
	Qt5::TextToSpeech
	Qt5::Xml
	Qt5::Widgets
	)
	
if(NOT QT_MKSPEC MATCHES "winrt")
	target_link_libraries(QGroundControl
		Qt5::SerialPort
		)
endif()

if(LINUX)
	target_link_libraries(QGroundControl ${SDL2_LIBRARIES})
elseif(APPLE)
	target_link_libraries(QGroundControl -F${CMAKE_SOURCE_DIR}/libs/lib/Frameworks "-framework SDL2")
	
	set_target_properties(${PROJECT_NAME} PROPERTIES
			MACOSX_BUNDLE YES
	)

	# deploy
	include(Macdeployqt)
	macdeployqt(QGroundControl)

elseif(WIN32)

	if(MSVC) # Check if we are using the Visual Studio compiler
		set_target_properties(${PROJECT_NAME} PROPERTIES
			WIN32_EXECUTABLE YES
			LINK_FLAGS "/ENTRY:mainCRTStartup"
		)
	endif()

	# Support both 32 and 64 bit builds
	if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
		set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/libs/lib/sdl2/msvc/lib/x64/SDL2.lib;${CMAKE_SOURCE_DIR}/libs/lib/sdl2/msvc/lib/x64/SDL2main.lib")
	else ()
		set(SDL2_LIBRARIES "${CMAKE_SOURCE_DIR}/libs/lib/sdl2/msvc/lib/x86/SDL2.lib;${CMAKE_SOURCE_DIR}/libs/lib/sdl2/msvc/lib/x86/SDL2main.lib")
	endif ()

	string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)
	
	target_link_libraries(QGroundControl ${SDL2_LIBRARIES})

	# deploy
	include(Windeployqt)
	windeployqt(QGroundControl "QGroundControl-installer.exe")

elseif(ANDROID)
	include(AddQtAndroidApk)
	add_qt_android_apk(QGroundControl.apk QGroundControl
		PACKAGE_NAME "io.mavlink.qgroundcontrol"
		#KEYSTORE ${CMAKE_CURRENT_LIST_DIR}/mykey.keystore myalias
		#KEYSTORE_PASSWORD xxxxx
	)

elseif(LINUX)
	# TODO: investigate https://github.com/probonopd/linuxdeployqt

	add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/release/package/QGroundControl.AppImage
		COMMAND ${CMAKE_SOURCE_DIR}/deploy/create_linux_appimage.sh ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/release/package;
		DEPENDS QGroundControl
		USES_TERMINAL
	)
	add_custom_target(appimage DEPENDS ${CMAKE_BINARY_DIR}/release/package/QGroundControl.AppImage)

endif()

