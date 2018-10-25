

include_directories(
	.
	${CMAKE_CURRENT_BINARY_DIR}

	${Qt5Location_PRIVATE_INCLUDE_DIRS}
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

	add_subdirectory(qgcunittest)

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

if(BUILD_TESTING)
	list(APPEND QGC_RESOURCES
		UnitTest.qrc
	)
endif()

add_library(qgc

	${QGC_RESOURCES}
	${QGC_UI}

	${EXTRA_SRC}

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

	# UI
	QGCQmlWidgetHolder.ui

)

set_source_files_properties(QGCApplication.cc PROPERTIES COMPILE_DEFINITIONS GIT_VERSION="${git_tag}")

add_subdirectory(ui)

add_subdirectory(Airmap)
add_subdirectory(AnalyzeView)
add_subdirectory(api)
add_subdirectory(Audio)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(comm)
add_subdirectory(FactSystem)
add_subdirectory(FirmwarePlugin)
add_subdirectory(FlightDisplay)
add_subdirectory(FlightMap)
add_subdirectory(FollowMe)
add_subdirectory(GPS)
add_subdirectory(Joystick)
add_subdirectory(MissionManager)
add_subdirectory(PositionManager)
add_subdirectory(QmlControls)
add_subdirectory(QtLocationPlugin)
add_subdirectory(Settings)
add_subdirectory(Terrain)
add_subdirectory(uas)
add_subdirectory(Vehicle)
add_subdirectory(VehicleSetup)
add_subdirectory(VideoStreaming)
add_subdirectory(ViewWidgets)

target_link_libraries(qgc

	PUBLIC

	Qt5::Bluetooth
	Qt5::Concurrent
	Qt5::Core
	Qt5::Location
	Qt5::Multimedia
	Qt5::Positioning
	Qt5::Quick
	Qt5::QuickWidgets
	Qt5::Sql
	Qt5::Svg
	Qt5::Test
	Qt5::TextToSpeech
	Qt5::Widgets
	Qt5::Xml

	Airmap
	AnalyzeView
	api
	Audio
	AutoPilotPlugins
	Camera
	comm
	FactSystem
	FirmwarePlugin
	FlightDisplay
	FlightMap
	FollowMe
	gps
	Joystick
	MissionManager
	PositionManager
	QmlControls
	QtLocationPlugin
	qwt # LinechartWidget
	Settings
	Terrain
	uas
	ui
	Vehicle
	VehicleSetup
	VideoStreaming
	ViewWidgets
)

if(BUILD_TESTING)
	target_link_libraries(qgc PUBLIC qgcunittest)
endif()

target_include_directories(qgc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

