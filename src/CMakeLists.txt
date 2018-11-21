
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

add_library(qgc

	${EXTRA_SRC}

	CmdLineOptParser.cc
	JsonHelper.cc
	KMLFileHelper.cc
	LogCompressor.cc
	main.cc
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
	ShapeFileHelper.cc
	SHPFileHelper.cc
	TerrainTile.cc
	UTM.cpp

	# UI
	QGCQmlWidgetHolder.ui
        QGCQmlWidgetHolder.h
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
        PRIVATE
		shp

	PUBLIC
                Qt5::QuickWidgets
                Qt5::Widgets

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

target_include_directories(qgc
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/ui/ui_autogen/include # HACK: AUTOUIC paths not inheriting?
        ${CMAKE_CURRENT_BINARY_DIR}/ui/ui_autogen/include_Debug
        ${CMAKE_CURRENT_BINARY_DIR}/qgc_autogen/include # HACK: AUTOUIC paths not inheriting?
        ${CMAKE_CURRENT_BINARY_DIR}/qgc_autogen/include_Debug
        )
