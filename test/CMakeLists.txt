# ============================================================================
# QGroundControl Unit Tests
# ============================================================================

if(NOT QGC_BUILD_TESTING)
    return()
endif()

# ----------------------------------------------------------------------------
# Test Framework Sources
# ----------------------------------------------------------------------------
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        UnitTestList.cc
        UnitTestList.h
)

# ----------------------------------------------------------------------------
# Test Dependencies
# ----------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Test)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Test)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_UNITTEST_BUILD)

# ----------------------------------------------------------------------------
# CTest Integration (CTest included in root CMakeLists.txt)
# ----------------------------------------------------------------------------
include(QGCTest)
include(Coverage)
include(Sanitizers)

# Sanitizers add significant runtime overhead; expand CTest per-test limits to
# avoid false timeout failures in CI sanitizer jobs.
if(QGC_ENABLE_ASAN OR QGC_ENABLE_UBSAN OR QGC_ENABLE_TSAN OR QGC_ENABLE_MSAN)
    set(QGC_TEST_TIMEOUT_UNIT 180 CACHE STRING "Timeout for unit tests (seconds)" FORCE)
    set(QGC_TEST_TIMEOUT_INTEGRATION 360 CACHE STRING "Timeout for integration tests (seconds)" FORCE)
    set(QGC_TEST_TIMEOUT_SLOW 540 CACHE STRING "Timeout for slow tests (seconds)" FORCE)
    set(QGC_TEST_TIMEOUT_DEFAULT 270 CACHE STRING "Default test timeout (seconds)" FORCE)
    set(QGC_TEST_TIMEOUT_EXTENDED 360)
else()
    set(QGC_TEST_TIMEOUT_EXTENDED 180)
endif()

# ============================================================================
# Test Subdirectories & Registration
# ============================================================================

# ----------------------------------------------------------------------------
# Unit Test Framework (must be first - provides MultiSignalSpy, etc.)
# ----------------------------------------------------------------------------
add_subdirectory(UnitTestFramework)
add_qgc_test(MultiSignalSpyTest LABELS Unit)
add_qgc_test(QmlTestRunner LABELS Unit)
add_qgc_test(TestBaseClassesTest LABELS Unit)
add_qgc_test(TestFixturesTest LABELS Unit)

# ----------------------------------------------------------------------------
# ADSB
# ----------------------------------------------------------------------------
add_subdirectory(ADSB)
add_qgc_test(ADSBTest LABELS Unit)

# ----------------------------------------------------------------------------
# AnalyzeView
# ----------------------------------------------------------------------------
add_subdirectory(AnalyzeView)
add_qgc_test(DataFlashParserTest LABELS Unit AnalyzeView RESOURCE_LOCK TempFiles)
add_qgc_test(ExifParserTest LABELS Unit AnalyzeView RESOURCE_LOCK TempFiles)
add_qgc_test(GeoTagControllerTest LABELS Unit AnalyzeView RESOURCE_LOCK TempFiles)
add_qgc_test(GeoTagDataTest LABELS Unit AnalyzeView)
add_qgc_test(GeoTagImageModelTest LABELS Unit AnalyzeView)
add_qgc_test(ULogParserTest LABELS Unit AnalyzeView RESOURCE_LOCK TempFiles)
add_qgc_test(LogDownloadTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(MavlinkLogTest LABELS Integration AnalyzeView Vehicle RESOURCE_LOCK MockLink)

# ----------------------------------------------------------------------------
# Camera
# ----------------------------------------------------------------------------
add_subdirectory(Camera)
add_qgc_test(QGCCameraManagerTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(QGCVideoStreamInfoTest LABELS Unit Camera)
add_qgc_test(VideoManagerTest LABELS Unit)

# ----------------------------------------------------------------------------
# Comms
# ----------------------------------------------------------------------------
add_subdirectory(Comms)
add_qgc_test(BluetoothConfigurationTest LABELS Unit Comms)
add_qgc_test(BluetoothEmulatedAdapterTest LABELS Integration Comms)
add_qgc_test(BluetoothLiveAdapterTest LABELS Integration Comms)
add_qgc_test(BluetoothWorkerTest LABELS Unit Comms)
add_qgc_test(QGCSerialPortInfoTest LABELS Unit Comms)

# ----------------------------------------------------------------------------
# FactSystem
# ----------------------------------------------------------------------------
add_subdirectory(FactSystem)
add_qgc_test(FactGroupTest LABELS Unit)
add_qgc_test(FactMetaDataTest LABELS Unit)
add_qgc_test(FactSystemTestGeneric LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(FactSystemTestPX4 LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(FactTest LABELS Unit)
add_qgc_test(FactValueSliderListModelTest LABELS Unit)
add_qgc_test(ParameterManagerTest LABELS Integration Vehicle SERIAL TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})

# ----------------------------------------------------------------------------
# FollowMe
# ----------------------------------------------------------------------------
add_subdirectory(FollowMe)
add_qgc_test(FollowMeTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)

# ----------------------------------------------------------------------------
# GPS
# ----------------------------------------------------------------------------
add_subdirectory(GPS)
add_qgc_test(GpsTest LABELS Unit GPS)

# ----------------------------------------------------------------------------
# Joystick
# ----------------------------------------------------------------------------
add_subdirectory(Joystick)
add_qgc_test(JoystickTest LABELS Unit RESOURCE_LOCK Joystick)
add_qgc_test(JoystickManagerTest LABELS Unit RESOURCE_LOCK Joystick TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(MockJoystickTest LABELS Unit Joystick RESOURCE_LOCK Joystick)
add_qgc_test(SDLTest LABELS Unit Joystick RESOURCE_LOCK Joystick)

# ----------------------------------------------------------------------------
# MAVLink
# ----------------------------------------------------------------------------
add_subdirectory(MAVLink)
add_qgc_test(SigningTest LABELS Unit MAVLink)
add_qgc_test(StatusTextHandlerTest LABELS Unit MAVLink)
add_qgc_test(SysStatusSensorInfoTest LABELS Unit MAVLink)

# ----------------------------------------------------------------------------
# MissionManager
# ----------------------------------------------------------------------------
add_subdirectory(MissionManager)
add_qgc_test(CameraCalcTest LABELS Unit MissionManager)
add_qgc_test(CameraSectionTest LABELS Unit MissionManager)
add_qgc_test(CorridorScanComplexItemTest LABELS Unit MissionManager RESOURCE_LOCK PlanViewSettings)
add_qgc_test(FWLandingPatternTest LABELS Unit MissionManager)
add_qgc_test(LandingComplexItemTest LABELS Unit MissionManager)
add_qgc_test(MissionCommandTreeEditorTest LABELS Unit MissionManager TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(MissionCommandTreeTest LABELS Unit MissionManager)
add_qgc_test(MissionControllerManagerTest LABELS Integration MissionManager RESOURCE_LOCK MockLink)
add_qgc_test(MissionControllerTest LABELS Integration MissionManager RESOURCE_LOCK MockLink)
add_qgc_test(MissionItemTest LABELS Unit MissionManager)
add_qgc_test(MissionManagerTest LABELS Integration MissionManager SERIAL)
add_qgc_test(MissionSettingsTest LABELS Unit MissionManager)
add_qgc_test(PlanMasterControllerTest LABELS Integration MissionManager RESOURCE_LOCK MockLink)
add_qgc_test(QGCMapPolygonTest LABELS Unit MissionManager)
add_qgc_test(QGCMapPolylineTest LABELS Unit MissionManager)
# SectionTest: base class only (used by CameraSectionTest, SpeedSectionTest)
# VisualMissionItemTest: base class only (used by FWLandingPatternTest, LandingComplexItemTest, etc.)
add_qgc_test(SimpleMissionItemTest LABELS Unit MissionManager)
add_qgc_test(SpeedSectionTest LABELS Unit MissionManager)
add_qgc_test(StructureScanComplexItemTest LABELS Unit MissionManager RESOURCE_LOCK PlanViewSettings)
add_qgc_test(SurveyComplexItemTest LABELS Unit MissionManager RESOURCE_LOCK PlanViewSettings)
add_qgc_test(TransectStyleComplexItemTest LABELS Unit MissionManager RESOURCE_LOCK PlanViewSettings)

# ----------------------------------------------------------------------------
# QtLocationPlugin
# ----------------------------------------------------------------------------
add_subdirectory(QtLocationPlugin)
add_qgc_test(MapProviderTest LABELS Unit)
add_qgc_test(QGCCacheWorkerTest LABELS Unit)
add_qgc_test(QGCCachedTileSetTest LABELS Unit)
add_qgc_test(QGCMapEngineManagerArchiveTest LABELS Unit RESOURCE_LOCK TempFiles)
add_qgc_test(QGCTileCacheDatabaseTest LABELS Unit)
add_qgc_test(QGCTileSetTest LABELS Unit)
add_qgc_test(UrlFactoryTest LABELS Unit)

# ----------------------------------------------------------------------------
# QmlControls
# ----------------------------------------------------------------------------
add_subdirectory(QmlControls)
add_qgc_test(QmlObjectListModelTest LABELS Unit)

# ----------------------------------------------------------------------------
# Terrain
# ----------------------------------------------------------------------------
add_subdirectory(Terrain)
add_qgc_test(TerrainQueryTest LABELS Integration Network)
add_qgc_test(TerrainTileTest LABELS Unit)

# ----------------------------------------------------------------------------
# Utilities
# ----------------------------------------------------------------------------
add_subdirectory(Utilities)
# Audio
add_qgc_test(AudioOutputTest LABELS Unit Utilities)
# Compression
add_qgc_test(QGCArchiveModelTest LABELS Unit Utilities)
add_qgc_test(QGCCompressionTest LABELS Unit Utilities RESOURCE_LOCK TempFiles)
add_qgc_test(QGCStreamingDecompressionTest LABELS Unit Utilities)
# Network
add_qgc_test(QGCNetworkHelperTest LABELS Unit Utilities Network)
# Parsing
add_qgc_test(DataFlashUtilityTest LABELS Unit Utilities)
add_qgc_test(ExifUtilityTest LABELS Unit Utilities)
add_qgc_test(JsonHelperTest LABELS Unit Utilities)
add_qgc_test(JsonParsingTest LABELS Unit Utilities)
add_qgc_test(ULogUtilityTest LABELS Unit Utilities)
# FileSystem
add_qgc_test(QGCArchiveWatcherTest LABELS Unit Utilities RESOURCE_LOCK TempFiles)
add_qgc_test(QGCCachedFileDownloadTest LABELS Integration Network Utilities)
add_qgc_test(QGCFileDownloadTest LABELS Integration Network Utilities)
add_qgc_test(QGCFileHelperTest LABELS Unit Utilities RESOURCE_LOCK TempFiles)
add_qgc_test(QGCFileWatcherTest LABELS Unit Utilities)
# Geo
add_qgc_test(GeoTest LABELS Unit Utilities)
add_qgc_test(GeoJsonHelperTest LABELS Unit Utilities)
add_qgc_test(KMLDomDocumentTest LABELS Unit Utilities)
add_qgc_test(QGCMathTest LABELS Unit Utilities)
# StateMachine
add_qgc_test(QGCStateMachineTest LABELS Unit Utilities)
add_qgc_test(ErrorRecoveryBuilderTest LABELS Unit Utilities)
add_qgc_test(StateContextTest LABELS Unit Utilities)
add_qgc_test(StateHistoryRecorderTest LABELS Unit Utilities)
add_qgc_test(AsyncFunctionStateTest LABELS Unit Utilities)
add_qgc_test(ConditionalStateTest LABELS Unit Utilities)
add_qgc_test(DelayStateTest LABELS Unit Utilities)
add_qgc_test(FunctionStateTest LABELS Unit Utilities)
add_qgc_test(ParallelStateTest LABELS Unit Utilities)
add_qgc_test(ProgressStateTest LABELS Unit Utilities)
add_qgc_test(QGCAbstractStateTest LABELS Unit Utilities)
add_qgc_test(QGCFinalStateTest LABELS Unit Utilities)
add_qgc_test(QGCHistoryStateTest LABELS Unit Utilities)
add_qgc_test(QGCStateTest LABELS Unit Utilities)
add_qgc_test(RequestMessageStateTest LABELS Unit Utilities)
add_qgc_test(RetryableRequestMessageStateTest LABELS Integration Vehicle Utilities RESOURCE_LOCK MockLink)
add_qgc_test(SendMavlinkCommandStateTest LABELS Unit Utilities)
add_qgc_test(SendMavlinkMessageStateTest LABELS Unit Utilities)
add_qgc_test(ShowAppMessageStateTest LABELS Unit Utilities)
add_qgc_test(SkippableAsyncStateTest LABELS Unit Utilities)
add_qgc_test(SubMachineStateTest LABELS Unit Utilities)
add_qgc_test(WaitForMavlinkMessageStateTest LABELS Unit Utilities)
add_qgc_test(WaitForSignalStateTest LABELS Unit Utilities)
add_qgc_test(WaitStateBaseTest LABELS Unit Utilities)
add_qgc_test(GuardedTransitionTest LABELS Unit Utilities)
add_qgc_test(InternalTransitionTest LABELS Unit Utilities)
add_qgc_test(MachineEventTransitionTest LABELS Unit Utilities)
add_qgc_test(NamedEventTransitionTest LABELS Unit Utilities)
add_qgc_test(QGCSignalTransitionTest LABELS Unit Utilities)
add_qgc_test(RetryTransitionTest LABELS Unit Utilities)
add_qgc_test(SignalDataTransitionTest LABELS Unit Utilities)
add_qgc_test(TimeoutTransitionTest LABELS Unit Utilities)
add_qgc_test(AdditionalStatesCoverageTest LABELS Unit Utilities)
add_qgc_test(AdditionalTransitionsCoverageTest LABELS Unit Utilities)
# Platform (desktop only)
if(NOT ANDROID AND NOT IOS)
    add_qgc_test(PlatformTest LABELS Unit Utilities)
    add_qgc_test(RunGuardTest LABELS Unit Utilities)
    add_qgc_test(SignalHandlerTest LABELS Unit Utilities)
endif()
# Shape
add_qgc_test(ShapeTest LABELS Unit Utilities)

# ----------------------------------------------------------------------------
# Viewer3D
# ----------------------------------------------------------------------------
add_subdirectory(Viewer3D)
add_qgc_test(CityMapGeometryTest LABELS Unit Viewer3D)
add_qgc_test(OsmParserTest LABELS Unit Viewer3D)
add_qgc_test(OsmParserThreadTest LABELS Unit Viewer3D RESOURCE_LOCK TempFiles)
add_qgc_test(Viewer3DTerrainGeometryTest LABELS Unit Viewer3D)
add_qgc_test(Viewer3DTileQueryTest LABELS Unit Viewer3D)
add_qgc_test(GeoCoordinateTypeTest LABELS Unit Viewer3D)
add_qgc_test(Viewer3DInstancingTest LABELS Unit Viewer3D)
add_qgc_test(Viewer3DManagerTest LABELS Unit Viewer3D)
add_qgc_test(Viewer3DMapProviderTest LABELS Unit Viewer3D)

# ----------------------------------------------------------------------------
# Vehicle
# ----------------------------------------------------------------------------
add_subdirectory(Vehicle)
add_qgc_test(APMAirframeComponentControllerTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(ComponentInformationCacheTest LABELS Unit Vehicle)
add_qgc_test(ComponentInformationTranslationTest LABELS Unit Vehicle)
add_qgc_test(ComponentInformationManagerTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(FTPControllerTest LABELS Integration Vehicle RESOURCE_LOCK MockLink TempFiles)
add_qgc_test(FTPManagerTest LABELS Integration Vehicle SERIAL)
add_qgc_test(FirmwareUpgradeControllerTest LABELS Unit Vehicle)
add_qgc_test(InitialConnectTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(InitialConnectPeripheralStartupTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(MAVLinkLogManagerTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(RequestMessageTest LABELS Integration Vehicle RESOURCE_LOCK MockLink TIMEOUT ${QGC_TEST_TIMEOUT_EXTENDED})
add_qgc_test(RequestMetaDataTypeStateMachineTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(SendMavCommandWithHandlerTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(SendMavCommandWithSignallingTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
add_qgc_test(VehicleLinkManagerTest LABELS Integration Vehicle RESOURCE_LOCK MockLink)
