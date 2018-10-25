

include_directories(
	.
	${CMAKE_CURRENT_BINARY_DIR}

	QtLocationPlugin/QMLControl
	${Qt5Location_PRIVATE_INCLUDE_DIRS}

	AnalyzeView
	api
	Audio
	AutoPilotPlugins
	AutoPilotPlugins/APM
	AutoPilotPlugins/Common
	AutoPilotPlugins/PX4
	Camera
	comm
	FactSystem
	FactSystem/FactControls
	FirmwarePlugin
	FirmwarePlugin/APM
	FlightDisplay
	FlightMap/Widgets
	FollowMe
	Joystick
	MissionManager
	PositionManager
	qgcunittest
	QmlControls
	QtLocationPlugin
	Settings
	Terrain
	uas
	ui
	ui/linechart
	Vehicle
	VehicleSetup
	VideoStreaming
	ViewWidgets
)

set(EXTRA_SRC)

if(MOBILE)
	list(APPEND EXTRA_SRC
		MobileScreenMgr.cc
	)

	if (ANDROID)
		list(APPEND EXTRA_SRC
			libs/qtandroidserialport/src/qserialport.cpp
			libs/qtandroidserialport/src/qserialport_android.cpp
			libs/qtandroidserialport/src/qserialportinfo.cpp
			libs/qtandroidserialport/src/qserialportinfo_android.cpp

			Joystick/JoystickAndroid.cc
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
		Audio/AudioOutputTest.cc

		comm/MockLink.cc
		comm/MockLinkFileServer.cc
		comm/MockLinkMissionItemHandler.cc

		FactSystem/FactSystemTestBase.cc
		FactSystem/FactSystemTestGeneric.cc
		FactSystem/FactSystemTestPX4.cc
		FactSystem/ParameterManagerTest.cc

		MissionManager/CameraCalcTest.cc
		MissionManager/CameraSectionTest.cc
		MissionManager/CorridorScanComplexItemTest.cc
		MissionManager/MissionCommandTreeTest.cc
		MissionManager/MissionControllerManagerTest.cc
		MissionManager/MissionControllerTest.cc
		MissionManager/MissionItemTest.cc
		MissionManager/MissionManagerTest.cc
		MissionManager/MissionSettingsTest.cc
		MissionManager/PlanMasterControllerTest.cc
		MissionManager/QGCMapPolygonTest.cc
		MissionManager/QGCMapPolylineTest.cc
		MissionManager/SectionTest.cc
		MissionManager/SimpleMissionItemTest.cc
		MissionManager/SpeedSectionTest.cc
		MissionManager/StructureScanComplexItemTest.cc
		MissionManager/SurveyComplexItemTest.cc
		MissionManager/TransectStyleComplexItemTest.cc
		MissionManager/VisualMissionItemTest.cc

		qgcunittest/FileDialogTest.cc
		qgcunittest/FileManagerTest.cc
		qgcunittest/FlightGearTest.cc
		qgcunittest/GeoTest.cc
		qgcunittest/LinkManagerTest.cc
		qgcunittest/MainWindowTest.cc
		qgcunittest/MavlinkLogTest.cc
		qgcunittest/MessageBoxTest.cc
		qgcunittest/MultiSignalSpy.cc
		qgcunittest/RadioConfigTest.cc
		qgcunittest/TCPLinkTest.cc
		qgcunittest/TCPLoopBackServer.cc
		qgcunittest/UnitTest.cc
		qgcunittest/UnitTestList.cc

		Vehicle/SendMavCommandTest.cc
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
		VideoStreaming/gstqtvideosink/delegates/basedelegate.cpp
		VideoStreaming/gstqtvideosink/delegates/qtquick2videosinkdelegate.cpp
		VideoStreaming/gstqtvideosink/delegates/qtvideosinkdelegate.cpp
		VideoStreaming/gstqtvideosink/delegates/qwidgetvideosinkdelegate.cpp
		VideoStreaming/gstqtvideosink/gstqtglvideosink.cpp
		VideoStreaming/gstqtvideosink/gstqtglvideosinkbase.cpp
		VideoStreaming/gstqtvideosink/gstqtquick2videosink.cpp
		VideoStreaming/gstqtvideosink/gstqtvideosink.cpp
		VideoStreaming/gstqtvideosink/gstqtvideosinkbase.cpp
		VideoStreaming/gstqtvideosink/gstqtvideosinkplugin.cpp
		VideoStreaming/gstqtvideosink/gstqwidgetvideosink.cpp
		VideoStreaming/gstqtvideosink/painters/genericsurfacepainter.cpp
		VideoStreaming/gstqtvideosink/painters/openglsurfacepainter.cpp
		VideoStreaming/gstqtvideosink/painters/videomaterial.cpp
		VideoStreaming/gstqtvideosink/painters/videonode.cpp
		VideoStreaming/gstqtvideosink/utils/bufferformat.cpp
		VideoStreaming/gstqtvideosink/utils/utils.cpp
	)
endif()

set(QGC_RESOURCES)
list(APPEND QGC_RESOURCES
	FirmwarePlugin/APM/APMResources.qrc
	FirmwarePlugin/PX4/PX4Resources.qrc
)

if(BUILD_TESTING)
	list(APPEND QGC_RESOURCES
		UnitTest.qrc
	)
endif()


set(QGC_UI)
list(APPEND QGC_UI
	#src/ui/Linechart.ui
	QGCQmlWidgetHolder.ui
	ui/MainWindow.ui
	ui/MultiVehicleDockWidget.ui
	ui/QGCHilConfiguration.ui
	ui/QGCHilFlightGearConfiguration.ui
	ui/QGCHilJSBSimConfiguration.ui
	ui/QGCHilXPlaneConfiguration.ui
	ui/QGCMapRCToParamDialog.ui
	ui/QGCMAVLinkInspector.ui
	ui/QGCMAVLinkLogPlayer.ui
	ui/QGCPluginHost.ui
	ui/QGCUASFileView.ui
	ui/QGCUASFileViewMulti.ui
	ui/QMap3D.ui
	ui/uas/QGCUnconnectedInfoWidget.ui
)

add_library(qgc

	${QGC_RESOURCES}
	${QGC_UI}

	${EXTRA_SRC}

	api/QGCCorePlugin.cc
	api/QGCOptions.cc
	api/QGCSettings.cc
	api/QmlComponentInfo.cc

	Audio/AudioOutput.cc

	AutoPilotPlugins/APM/APMAirframeComponent.cc
	AutoPilotPlugins/APM/APMAirframeComponentAirframes.cc
	AutoPilotPlugins/APM/APMAirframeComponentController.cc
	AutoPilotPlugins/APM/APMAirframeLoader.cc
	AutoPilotPlugins/APM/APMAutoPilotPlugin.cc
	AutoPilotPlugins/APM/APMCameraComponent.cc
	AutoPilotPlugins/APM/APMCompassCal.cc
	AutoPilotPlugins/APM/APMFlightModesComponent.cc
	AutoPilotPlugins/APM/APMFlightModesComponentController.cc
	AutoPilotPlugins/APM/APMLightsComponent.cc
	AutoPilotPlugins/APM/APMPowerComponent.cc
	AutoPilotPlugins/APM/APMRadioComponent.cc
	AutoPilotPlugins/APM/APMSafetyComponent.cc
	AutoPilotPlugins/APM/APMSensorsComponent.cc
	AutoPilotPlugins/APM/APMSensorsComponentController.cc
	AutoPilotPlugins/APM/APMSubFrameComponent.cc
	AutoPilotPlugins/APM/APMTuningComponent.cc
	AutoPilotPlugins/APM/APMHeliComponent.cc

	AutoPilotPlugins/Common/ESP8266Component.cc
	AutoPilotPlugins/Common/ESP8266ComponentController.cc
	AutoPilotPlugins/Common/MotorComponent.cc
	AutoPilotPlugins/Common/RadioComponentController.cc
	AutoPilotPlugins/Common/SyslinkComponent.cc
	AutoPilotPlugins/Common/SyslinkComponentController.cc

	AutoPilotPlugins/PX4/AirframeComponent.cc
	AutoPilotPlugins/PX4/AirframeComponentAirframes.cc
	AutoPilotPlugins/PX4/AirframeComponentController.cc
	AutoPilotPlugins/PX4/CameraComponent.cc
	AutoPilotPlugins/PX4/FlightModesComponent.cc
	AutoPilotPlugins/PX4/PowerComponent.cc
	AutoPilotPlugins/PX4/PowerComponentController.cc
	AutoPilotPlugins/PX4/PX4AdvancedFlightModesController.cc
	AutoPilotPlugins/PX4/PX4AirframeLoader.cc
	AutoPilotPlugins/PX4/PX4AutoPilotPlugin.cc
	AutoPilotPlugins/PX4/PX4RadioComponent.cc
	AutoPilotPlugins/PX4/PX4SimpleFlightModesController.cc
	AutoPilotPlugins/PX4/PX4TuningComponent.cc
	AutoPilotPlugins/PX4/SafetyComponent.cc
	AutoPilotPlugins/PX4/SensorsComponent.cc
	AutoPilotPlugins/PX4/SensorsComponentController.cc

	AutoPilotPlugins/AutoPilotPlugin.cc
	AutoPilotPlugins/Generic/GenericAutoPilotPlugin.cc

	Camera/QGCCameraControl.cc
	Camera/QGCCameraIO.cc
	Camera/QGCCameraManager.cc

	#src/comm/BluetoothLink.cc
	comm/LinkConfiguration.cc
	comm/LinkInterface.cc
	comm/LinkManager.cc
	comm/LogReplayLink.cc
	comm/MavlinkMessagesTimer.cc
	comm/MAVLinkProtocol.cc
	comm/QGCFlightGearLink.cc
	comm/QGCJSBSimLink.cc
	comm/QGCMAVLink.cc
	comm/QGCSerialPortInfo.cc
	comm/QGCXPlaneLink.cc
	comm/SerialLink.cc
	comm/TCPLink.cc
	comm/UDPLink.cc

	FactSystem/Fact.cc
	FactSystem/FactControls/FactPanelController.cc
	FactSystem/FactGroup.cc
	FactSystem/FactMetaData.cc
	FactSystem/FactSystem.cc
	FactSystem/FactValueSliderListModel.cc
	FactSystem/ParameterManager.cc
	FactSystem/SettingsFact.cc

	FirmwarePlugin/APM/APMFirmwarePlugin.cc
	FirmwarePlugin/APM/APMFirmwarePluginFactory.cc
	FirmwarePlugin/APM/APMParameterMetaData.cc
	FirmwarePlugin/APM/ArduCopterFirmwarePlugin.cc
	FirmwarePlugin/APM/ArduPlaneFirmwarePlugin.cc
	FirmwarePlugin/APM/ArduRoverFirmwarePlugin.cc
	FirmwarePlugin/APM/ArduSubFirmwarePlugin.cc

	FirmwarePlugin/CameraMetaData.cc
	FirmwarePlugin/FirmwarePlugin.cc
	FirmwarePlugin/FirmwarePluginManager.cc

	FirmwarePlugin/PX4/PX4FirmwarePlugin.cc
	FirmwarePlugin/PX4/PX4FirmwarePluginFactory.cc
	FirmwarePlugin/PX4/PX4ParameterMetaData.cc

	FlightDisplay/VideoManager.cc

	FlightMap/Widgets/ValuesWidgetController.cc

	FollowMe/FollowMe.cc

	Joystick/Joystick.cc
	Joystick/JoystickManager.cc
	Joystick/JoystickSDL.cc

	MissionManager/CameraCalc.cc
	MissionManager/CameraSection.cc
	MissionManager/CameraSpec.cc
	MissionManager/ComplexMissionItem.cc
	MissionManager/CorridorScanComplexItem.cc
	MissionManager/FixedWingLandingComplexItem.cc
	MissionManager/GeoFenceController.cc
	MissionManager/GeoFenceManager.cc
	MissionManager/KML.cc
	MissionManager/MissionCommandList.cc
	MissionManager/MissionCommandTree.cc
	MissionManager/MissionCommandUIInfo.cc
	MissionManager/MissionController.cc
	MissionManager/MissionItem.cc
	MissionManager/MissionManager.cc
	MissionManager/MissionSettingsItem.cc
	MissionManager/PlanElementController.cc
	MissionManager/PlanManager.cc
	MissionManager/PlanMasterController.cc
	MissionManager/QGCFenceCircle.cc
	MissionManager/QGCFencePolygon.cc
	MissionManager/QGCMapCircle.cc
	MissionManager/QGCMapPolygon.cc
	MissionManager/QGCMapPolyline.cc
	MissionManager/RallyPoint.cc
	MissionManager/RallyPointController.cc
	MissionManager/RallyPointManager.cc
	MissionManager/SimpleMissionItem.cc
	MissionManager/SpeedSection.cc
	MissionManager/StructureScanComplexItem.cc
	MissionManager/SurveyComplexItem.cc
	MissionManager/TransectStyleComplexItem.cc
	MissionManager/VisualMissionItem.cc

	PositionManager/PositionManager.cpp
	PositionManager/SimulatedPosition.cc

	QmlControls/AppMessages.cc
	QmlControls/CoordinateVector.cc
	QmlControls/EditPositionDialogController.cc
	QmlControls/ParameterEditorController.cc
	QmlControls/QGCFileDialogController.cc
	QmlControls/QGCGeoBoundingCube.cc
	QmlControls/QGCImageProvider.cc
	QmlControls/QGroundControlQmlGlobal.cc
	QmlControls/QmlObjectListModel.cc
	QmlControls/QmlTestWidget.cc
	QmlControls/RCChannelMonitorController.cc
	QmlControls/ScreenToolsController.cc

	QtLocationPlugin/QGCMapEngine.cpp
	QtLocationPlugin/QGCMapTileSet.cpp
	QtLocationPlugin/QGCMapUrlEngine.cpp
	QtLocationPlugin/QGCTileCacheWorker.cpp
	QtLocationPlugin/QGeoCodeReplyQGC.cpp
	QtLocationPlugin/QGeoCodingManagerEngineQGC.cpp
	QtLocationPlugin/QGeoMapReplyQGC.cpp
	QtLocationPlugin/QGeoServiceProviderPluginQGC.cpp
	QtLocationPlugin/QGeoTiledMappingManagerEngineQGC.cpp
	QtLocationPlugin/QGeoTileFetcherQGC.cpp
	QtLocationPlugin/QMLControl/QGCMapEngineManager.cc

	Settings/AppSettings.cc
	Settings/AutoConnectSettings.cc
	Settings/BrandImageSettings.cc
	Settings/FlightMapSettings.cc
	Settings/GuidedSettings.cc
	Settings/RTKSettings.cc
	Settings/SettingsGroup.cc
	Settings/SettingsManager.cc
	Settings/UnitsSettings.cc
	Settings/VideoSettings.cc

	Terrain/TerrainQuery.cc

	uas/FileManager.cc
	uas/UAS.cc
	uas/UASMessageHandler.cc

	ui/HILDockWidget.cc
	ui/linechart/ChartPlot.cc
	ui/linechart/IncrementalPlot.cc
	ui/linechart/LinechartPlot.cc
	ui/linechart/Linecharts.cc
	ui/linechart/LinechartWidget.cc
	ui/linechart/Scrollbar.cc
	ui/linechart/ScrollZoomer.cc
	ui/MainWindow.cc
	ui/MAVLinkDecoder.cc
	ui/MultiVehicleDockWidget.cc
	ui/QGCHilConfiguration.cc
	ui/QGCHilFlightGearConfiguration.cc
	ui/QGCHilJSBSimConfiguration.cc
	ui/QGCHilXPlaneConfiguration.cc
	ui/QGCMapRCToParamDialog.cpp
	ui/QGCMAVLinkInspector.cc
	ui/QGCMAVLinkLogPlayer.cc
	ui/QGCPluginHost.cc
	ui/QGCUASFileView.cc
	ui/QGCUASFileViewMulti.cc
	ui/uas/QGCUnconnectedInfoWidget.cc

	Vehicle/ADSBVehicle.cc
	Vehicle/GPSRTKFactGroup.cc
	Vehicle/MAVLinkLogManager.cc
	Vehicle/MultiVehicleManager.cc
	Vehicle/Vehicle.cc

	VehicleSetup/Bootloader.cc
	VehicleSetup/FirmwareImage.cc
	VehicleSetup/FirmwareUpgradeController.cc
	VehicleSetup/JoystickConfigController.cc
	VehicleSetup/PX4FirmwareUpgradeThread.cc
	VehicleSetup/VehicleComponent.cc

	VideoStreaming/VideoItem.cc
	VideoStreaming/VideoReceiver.cc
	VideoStreaming/VideoStreaming.cc
	VideoStreaming/VideoSurface.cc

	ViewWidgets/CustomCommandWidget.cc
	ViewWidgets/CustomCommandWidgetController.cc
	ViewWidgets/ViewWidgetController.cc

	CmdLineOptParser.cc
	JsonHelper.cc
	KMLFileHelper.cc
	LogCompressor.cc
	QGC.cc
	QGCApplication.cc
	QGCComboBox.cc
	QGCDockWidget.cc
	QGCFileDownload.cc
	QGCGeo.cc
	QGCLoggingCategory.cc
	QGCMapPalette.cc
	QGCPalette.cc
	QGCQFileDialog.cc
	QGCQGeoCoordinate.cc
	QGCQmlWidgetHolder.cpp
	QGCQuickWidget.cc
	QGCTemporaryFile.cc
	QGCToolbox.cc
	RunGuard.cc
	TerrainTile.cc
	UTM.cpp
	main.cc

	# HEADERS
	# shouldn't be listed here, but aren't named properly for AUTOMOC
	comm/QGCFlightGearLink.h
	comm/QGCHilLink.h
	comm/QGCJSBSimLink.h
	MissionManager/Section.h
	QtLocationPlugin/QGCMapEngineData.h
	uas/UAS.h
	uas/UASInterface.h
	ui/QGCHilFlightGearConfiguration.h
	ui/QGCHilJSBSimConfiguration.h
)

set_source_files_properties(QGCApplication.cc PROPERTIES COMPILE_DEFINITIONS GIT_VERSION="${git_tag}")

target_link_libraries(qgc

	PUBLIC

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

	Airmap
	gps
	qwt # LinechartWidget
)

target_include_directories(qgc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(AnalyzeView)
add_subdirectory(Airmap)
add_subdirectory(GPS)

