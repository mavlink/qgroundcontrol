
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${Qt5Location_PRIVATE_INCLUDE_DIRS}
	VideoManager
)

set(EXTRA_SRC)

if(MOBILE)
	list(APPEND EXTRA_SRC
		MobileScreenMgr.cc
		MobileScreenMgr.h
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

	add_qgc_test(ComponentInformationCacheTest)
	add_qgc_test(CameraCalcTest)
	add_qgc_test(CameraSectionTest)
	add_qgc_test(CorridorScanComplexItemTest)
	add_qgc_test(FactSystemTestGeneric)
	add_qgc_test(FactSystemTestPX4)
	#add_qgc_test(FileDialogTest)
	#add_qgc_test(FileManagerTest)
	add_qgc_test(FlightGearUnitTest)
	add_qgc_test(GeoTest)
	add_qgc_test(LinkManagerTest)
	add_qgc_test(LogDownloadTest)
	#add_qgc_test(MessageBoxTest)
	add_qgc_test(MissionCommandTreeTest)
	add_qgc_test(MissionControllerTest)
	add_qgc_test(MissionItemTest)
	add_qgc_test(MissionManagerTest)
	add_qgc_test(MissionSettingsTest)
	add_qgc_test(ParameterManagerTest)
	add_qgc_test(PlanMasterControllerTest)
	add_qgc_test(QGCMapPolygonTest)
	add_qgc_test(QGCMapPolylineTest)
	#add_qgc_test(RadioConfigTest)
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
	CmdLineOptParser.h
	JsonHelper.cc
	JsonHelper.h
	KMLDomDocument.cc
	KMLDomDocument.h
	KMLHelper.cc
	KMLHelper.h
	LogCompressor.cc
	LogCompressor.h
	main.cc
	QGC.cc
	QGC.h
	QGCApplication.cc
	QGCApplication.h
	QGCComboBox.cc
	QGCComboBox.h
	QGCConfig.h
	QGCFileDownload.cc
	QGCFileDownload.h
	QGCLoggingCategory.cc
	QGCLoggingCategory.h
	QGCMapPalette.cc
	QGCMapPalette.h
	QGCPalette.cc
	QGCPalette.h
	QGCQGeoCoordinate.cc
	QGCQGeoCoordinate.h
	QGCTemporaryFile.cc
	QGCTemporaryFile.h
	QGCToolbox.cc
	QGCToolbox.h
	RunGuard.cc
	RunGuard.h
	ShapeFileHelper.cc
	ShapeFileHelper.h
	SHPFileHelper.cc
	SHPFileHelper.h
	stable_headers.h
	TerrainTile.cc
	TerrainTile.h
)

set_source_files_properties(QGCApplication.cc PROPERTIES COMPILE_DEFINITIONS APP_VERSION_STR="${APP_VERSION_STR}")

add_subdirectory(ui)

add_subdirectory(ADSB)
add_subdirectory(Airmap)
add_subdirectory(AnalyzeView)
add_subdirectory(api)
add_subdirectory(Audio)
add_subdirectory(AutoPilotPlugins)
add_subdirectory(Camera)
add_subdirectory(comm)
add_subdirectory(Compression)
add_subdirectory(FactSystem)
add_subdirectory(FirmwarePlugin)
add_subdirectory(FlightDisplay)
add_subdirectory(FlightMap)
add_subdirectory(FollowMe)
add_subdirectory(Geo)
add_subdirectory(GPS)
add_subdirectory(Joystick)
if (${QGC_GST_MICROHARD_ENABLED})
  add_subdirectory(Microhard)
endif ()
add_subdirectory(MissionManager)
add_subdirectory(PlanView)
add_subdirectory(PositionManager)
add_subdirectory(QmlControls)
add_subdirectory(QtLocationPlugin)
add_subdirectory(Settings)
if (${QGC_GST_TAISYNC_ENABLED})
  add_subdirectory(Taisync)
endif ()
add_subdirectory(Terrain)
add_subdirectory(uas)
add_subdirectory(Vehicle)
add_subdirectory(VehicleSetup)
add_subdirectory(VideoManager)
add_subdirectory(VideoReceiver)

target_link_libraries(qgc
	PRIVATE
		shp
		Qt5::QuickControls2

	PUBLIC
		Qt5::QuickWidgets
		Qt5::Widgets

		ADSB
		Airmap
		AnalyzeView
		api
		Audio
		AutoPilotPlugins
		Camera
		comm
		FactSystem
		FirmwarePlugin
		FlightMap
		FollowMe
		Geo
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
		VideoManager
		z # zlib
)

if(GST_FOUND)
	if (${QGC_GST_MICROHARD_ENABLED})
		target_compile_definitions(qgc PUBLIC QGC_GST_MICROHARD_ENABLED)
		target_link_libraries(qgc PUBLIC Microhard)
	endif()
	if (${QGC_GST_TAISYNC_ENABLED})
		target_compile_definitions(qgc PUBLIC QGC_GST_TAISYNC_ENABLED)
		target_link_libraries(qgc PUBLIC Taisync)
	endif()
endif()

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

