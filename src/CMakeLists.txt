

include_directories(
	.
	${CMAKE_CURRENT_BINARY_DIR}

	QtLocationPlugin/QMLControl
	${Qt5Location_PRIVATE_INCLUDE_DIRS}

	Camera
	FactSystem
	FactSystem/FactControls
	FlightDisplay
	FlightMap/Widgets
	FollowMe
	Joystick
	PositionManager
	qgcunittest
	QmlControls
	QtLocationPlugin
	Settings
	Terrain
	uas
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

		FactSystem/FactSystemTestBase.cc
		FactSystem/FactSystemTestGeneric.cc
		FactSystem/FactSystemTestPX4.cc
		FactSystem/ParameterManagerTest.cc

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

)

if(BUILD_TESTING)
	list(APPEND QGC_RESOURCES
		UnitTest.qrc
	)
endif()


set(QGC_UI)
list(APPEND QGC_UI
	QGCQmlWidgetHolder.ui
)

add_library(qgc

	${QGC_RESOURCES}
	${QGC_UI}

	${EXTRA_SRC}

	Camera/QGCCameraControl.cc
	Camera/QGCCameraIO.cc
	Camera/QGCCameraManager.cc

	FactSystem/Fact.cc
	FactSystem/FactControls/FactPanelController.cc
	FactSystem/FactGroup.cc
	FactSystem/FactMetaData.cc
	FactSystem/FactSystem.cc
	FactSystem/FactValueSliderListModel.cc
	FactSystem/ParameterManager.cc
	FactSystem/SettingsFact.cc

	FlightDisplay/VideoManager.cc

	FlightMap/Widgets/ValuesWidgetController.cc

	FollowMe/FollowMe.cc

	Joystick/Joystick.cc
	Joystick/JoystickManager.cc
	Joystick/JoystickSDL.cc

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
	QtLocationPlugin/QGCMapEngineData.h
	uas/UAS.h
	uas/UASInterface.h

)

set_source_files_properties(QGCApplication.cc PROPERTIES COMPILE_DEFINITIONS GIT_VERSION="${git_tag}")

add_subdirectory(ui)

add_subdirectory(AnalyzeView)
add_subdirectory(Airmap)
add_subdirectory(api)
add_subdirectory(Audio)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(comm)
add_subdirectory(FirmwarePlugin)
add_subdirectory(GPS)
add_subdirectory(MissionManager)


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
	AnalyzeView
	api
	Audio
	AutoPilotPlugins
	comm
	FirmwarePlugin
	gps
	qwt # LinechartWidget
	MissionManager
	ui
)

target_include_directories(qgc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})



