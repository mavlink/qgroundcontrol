# ============================================================================
# QGroundControl Unit Tests
# ============================================================================
# See TESTING.md for full documentation.

if(NOT QGC_BUILD_TESTING)
    return()
endif()

# TODO: Android test utilities
# if(ANDROID)
#     include(AndroidTestUtilities)
# endif()

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        UnitTestList.cc
        UnitTestList.h
)

find_package(Qt6 REQUIRED COMPONENTS Test)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Test)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_UNITTEST_BUILD)
target_precompile_headers(${CMAKE_PROJECT_NAME} PRIVATE <QtTest/QTest> <QtTest/QSignalSpy>)

include(QGCTest)
include(Coverage)
include(Sanitizers)
include(FuzzTesting)

# ============================================================================
# Test Registration
# ============================================================================

add_subdirectory(ADSB)
add_qgc_test(ADSBVehicleTest LABELS Unit ADSB TIMEOUT 120 RESOURCE_LOCK GlobalState)
add_qgc_test(ADSBTCPLinkTest LABELS Unit ADSB Network RESOURCE_LOCK GlobalState)
add_qgc_test(ADSBVehicleManagerTest LABELS Unit ADSB RESOURCE_LOCK GlobalState)

add_subdirectory(AnalyzeView)
add_qgc_test(ExifParserTest LABELS Unit AnalyzeView RESOURCE_LOCK GlobalState)
add_qgc_test(GeoTagControllerTest LABELS Unit AnalyzeView RESOURCE_LOCK GlobalState)
add_qgc_test(LogDownloadTest LABELS Integration AnalyzeView RESOURCE_LOCK GlobalState)
add_qgc_test(MavlinkLogTest LABELS Unit AnalyzeView RESOURCE_LOCK GlobalState)
add_qgc_test(PX4LogParserTest LABELS Unit AnalyzeView RESOURCE_LOCK GlobalState)
add_qgc_test(ULogParserTest LABELS Unit AnalyzeView RESOURCE_LOCK GlobalState)

add_subdirectory(Camera)
add_qgc_test(QGCCameraManagerTest LABELS Integration Camera RESOURCE_LOCK GlobalState MockLink)

add_subdirectory(Comms)
add_qgc_test(LinkConfigurationTest LABELS Unit Comms RESOURCE_LOCK GlobalState)
add_qgc_test(QGCSerialPortInfoTest LABELS Unit Comms RESOURCE_LOCK GlobalState)
add_qgc_test(TCPConfigurationTest LABELS Unit Comms RESOURCE_LOCK GlobalState)

add_subdirectory(FactSystem)
add_qgc_test(FactSystemTestGeneric LABELS Integration FactSystem RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(FactSystemTestPX4 LABELS Integration FactSystem RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(ParameterManagerTest LABELS Integration FactSystem Slow TIMEOUT 300
    RESOURCE_LOCK GlobalState MockLink ParameterManager)

add_subdirectory(FollowMe)
add_qgc_test(FollowMeTest LABELS Integration FollowMe RESOURCE_LOCK GlobalState)

add_subdirectory(GPS)
add_qgc_test(GpsTest LABELS Unit GPS RESOURCE_LOCK GlobalState)

add_subdirectory(Joystick)
add_qgc_test(JoystickTest LABELS Unit RESOURCE_LOCK GlobalState TIMEOUT 120)
add_qgc_test(JoystickManagerTest LABELS Unit RESOURCE_LOCK GlobalState TIMEOUT 120)

add_subdirectory(MAVLink)
add_qgc_test(MavlinkFTPTest LABELS Unit MAVLink RESOURCE_LOCK GlobalState)
add_qgc_test(MAVLinkStreamConfigTest LABELS Unit MAVLink RESOURCE_LOCK GlobalState)
add_qgc_test(SigningTest LABELS Unit MAVLink RESOURCE_LOCK GlobalState)
add_qgc_test(StatusTextHandlerTest LABELS Unit MAVLink RESOURCE_LOCK GlobalState)
add_qgc_test(SysStatusSensorInfoTest LABELS Unit MAVLink RESOURCE_LOCK GlobalState)

add_subdirectory(QmlControls)
add_qgc_test(QGCGeoBoundingCubeTest LABELS Unit QmlControls RESOURCE_LOCK GlobalState)
add_qgc_test(QmlObjectListModelTest LABELS Unit QmlControls RESOURCE_LOCK GlobalState)

add_subdirectory(MissionManager)
add_qgc_test(CameraCalcTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(CameraSectionTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(CorridorScanComplexItemTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(FWLandingPatternTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(LandingComplexItemTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(MissionCommandTreeTest LABELS Unit MissionManager RESOURCE_LOCK GlobalState)
# MissionControllerManagerTest is a base class - not registered as a test
add_qgc_test(MissionControllerTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(MissionErrorHandlingTest LABELS Unit MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(MissionItemTest LABELS Unit MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(MissionManagerTest LABELS Integration MissionManager Slow
    RESOURCE_LOCK GlobalState MockLink MissionController)
add_qgc_test(MissionWorkflowTest LABELS Integration MissionManager Slow
    RESOURCE_LOCK GlobalState MockLink MissionController)
add_qgc_test(PlanFileRoundtripTest LABELS Unit MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(MissionSettingsTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(PlanMasterControllerTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(QGCMapPolygonTest LABELS Unit MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(QGCMapPolylineTest LABELS Unit MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(SimpleMissionItemTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(SpeedSectionTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(StructureScanComplexItemTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(SurveyComplexItemTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)
add_qgc_test(TransectStyleComplexItemTest LABELS Integration MissionManager RESOURCE_LOCK GlobalState)

add_subdirectory(UnitTestFramework)

add_subdirectory(Terrain)
add_qgc_test(TerrainQueryTest LABELS Unit Terrain RESOURCE_LOCK GlobalState)
add_qgc_test(TerrainTileTest LABELS Unit Terrain RESOURCE_LOCK GlobalState)

add_subdirectory(Utilities)
add_qgc_test(AudioOutputTest LABELS Unit Utilities Audio RESOURCE_LOCK GlobalState)
add_qgc_test(QGCArchiveModelTest LABELS Unit Utilities Compression RESOURCE_LOCK GlobalState)
add_qgc_test(QGCCompressionTest LABELS Unit Utilities Compression RESOURCE_LOCK GlobalState)
add_qgc_test(QGCStreamingDecompressionTest LABELS Unit Utilities Compression RESOURCE_LOCK GlobalState)
add_qgc_test(QGCArchiveWatcherTest LABELS Unit Utilities FileSystem RESOURCE_LOCK GlobalState)
add_qgc_test(QGCFileDownloadTest LABELS Unit Utilities FileSystem Network RESOURCE_LOCK GlobalState)
add_qgc_test(QGCFileHelperTest LABELS Unit Utilities FileSystem RESOURCE_LOCK GlobalState)
add_qgc_test(QGCFileWatcherTest LABELS Unit Utilities FileSystem RESOURCE_LOCK GlobalState)
add_qgc_test(GeoJsonHelperTest LABELS Unit Utilities Geo RESOURCE_LOCK GlobalState)
add_qgc_test(GeoTest LABELS Unit Utilities Geo RESOURCE_LOCK GlobalState)
add_qgc_test(QGCNetworkHelperTest LABELS Unit Utilities Network RESOURCE_LOCK GlobalState)
add_qgc_test(ShapeTest LABELS Unit Utilities Shape RESOURCE_LOCK GlobalState)
add_qgc_test(PerformanceBenchmarkTest LABELS Unit Utilities Performance RESOURCE_LOCK GlobalState)
add_qgc_test(JsonHelperTest LABELS Unit Utilities Json RESOURCE_LOCK GlobalState)

add_subdirectory(Vehicle)
add_qgc_test(ComponentInformationCacheTest LABELS Unit Vehicle RESOURCE_LOCK GlobalState)
add_qgc_test(ComponentInformationTranslationTest LABELS Unit Vehicle RESOURCE_LOCK GlobalState)

add_subdirectory(Video)
add_qgc_test(VideoReceiverTest LABELS Unit Video RESOURCE_LOCK GlobalState)
add_qgc_test(VideoSettingsTest LABELS Unit Video RESOURCE_LOCK GlobalState)
add_qgc_test(VideoUrlConstructionTest LABELS Unit Video RESOURCE_LOCK GlobalState)
add_qgc_test(FTPManagerTest LABELS Integration Vehicle Slow RESOURCE_LOCK GlobalState MockLink FTPManager)
add_qgc_test(InitialConnectTest LABELS Integration Vehicle RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(MAVLinkLogManagerTest LABELS Integration Vehicle MAVLink RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(RequestMessageTest LABELS Integration Vehicle MAVLink RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(SendMavCommandWithHandlerTest LABELS Integration Vehicle MAVLink RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(SendMavCommandWithSignallingTest LABELS Integration Vehicle MAVLink RESOURCE_LOCK GlobalState MockLink)
add_qgc_test(VehicleLinkManagerTest LABELS Integration Vehicle Slow RESOURCE_LOCK GlobalState MockLink)
