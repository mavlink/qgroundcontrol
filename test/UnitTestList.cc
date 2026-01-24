#include "UnitTestList.h"
#include "UnitTest.h"

#include <QtCore/QSet>

// ============================================================================
// ADSB
// ============================================================================
#include "ADSBVehicleTest.h"
#include "ADSBTCPLinkTest.h"
#include "ADSBVehicleManagerTest.h"

// ============================================================================
// AnalyzeView
// ============================================================================
#include "ExifParserTest.h"
#include "GeoTagControllerTest.h"
#include "LogDownloadTest.h"
#include "PX4LogParserTest.h"
#include "ULogParserTest.h"
#include "MavlinkLogTest.h"

// ============================================================================
// AutoPilotPlugins
// ============================================================================
// #include "RadioConfigTest.h"

// ============================================================================
// Camera
// ============================================================================
#include "QGCCameraManagerTest.h"

// ============================================================================
// Comms
// ============================================================================
#include "LinkConfigurationTest.h"
#include "MockConfigurationTest.h"
#include "MockLinkTest.h"
#include "QGCSerialPortInfoTest.h"
#include "TCPConfigurationTest.h"

// ============================================================================
// FactSystem
// ============================================================================
#include "FactSystemTestGeneric.h"
#include "FactSystemTestPX4.h"
#include "ParameterManagerTest.h"

// ============================================================================
// FollowMe
// ============================================================================
#include "FollowMeTest.h"

// ============================================================================
// GPS
// ============================================================================
#include "GpsTest.h"

// ============================================================================
// MAVLink
// ============================================================================
#include "MavlinkFTPTest.h"
#include "MAVLinkStreamConfigTest.h"
#include "SigningTest.h"
#include "StatusTextHandlerTest.h"
#include "SysStatusSensorInfoTest.h"

// ============================================================================
// QmlControls
// ============================================================================
#include "QGCGeoBoundingCubeTest.h"
#include "QmlObjectListModelTest.h"

// ============================================================================
// MissionManager
// ============================================================================
#include "CameraCalcTest.h"
#include "CameraSectionTest.h"
#include "CorridorScanComplexItemTest.h"
#include "MissionCommandTreeTest.h"
#include "MissionControllerManagerTest.h"
#include "MissionControllerTest.h"
#include "MissionErrorHandlingTest.h"
#include "MissionItemTest.h"
#include "MissionManagerTest.h"
#include "MissionSettingsTest.h"
#include "MissionWorkflowTest.h"
#include "PlanFileRoundtripTest.h"
#include "PlanMasterControllerTest.h"
#include "QGCMapPolygonTest.h"
#include "QGCMapPolylineTest.h"
#include "SimpleMissionItemTest.h"
#include "SpeedSectionTest.h"
#include "StructureScanComplexItemTest.h"
#include "SurveyComplexItemTest.h"
#include "TransectStyleComplexItemTest.h"
#include "FWLandingPatternTest.h"
#include "LandingComplexItemTest.h"
// #include "MissionCommandTreeEditorTest.h"
// #include "SectionTest.h"
// #include "VisualMissionItemTest.h"

// ============================================================================
// Terrain
// ============================================================================
#include "TerrainQueryTest.h"
#include "TerrainTileTest.h"

// ============================================================================
// UnitTestFramework
// ============================================================================
#include "MultiSignalSpyTest.h"
#include "TestFixturesTest.h"

// ============================================================================
// Utilities - Audio
// ============================================================================
#include "AudioOutputTest.h"

// ============================================================================
// Utilities - Compression
// ============================================================================
#include "QGCArchiveModelTest.h"
#include "QGCCompressionTest.h"
#include "QGCStreamingDecompressionTest.h"

// ============================================================================
// Utilities - FileSystem
// ============================================================================
#include "QGCArchiveWatcherTest.h"
#include "QGCFileDownloadTest.h"
#include "QGCFileHelperTest.h"
#include "QGCFileWatcherTest.h"

// ============================================================================
// Utilities - Geo
// ============================================================================
#include "GeoJsonHelperTest.h"
#include "GeoTest.h"

// ============================================================================
// Utilities - Network
// ============================================================================
#include "QGCNetworkHelperTest.h"

// ============================================================================
// Utilities - Shape
// ============================================================================
#include "ShapeTest.h"

// ============================================================================
// Utilities - Performance
// ============================================================================
#include "PerformanceBenchmarkTest.h"

// ============================================================================
// Utilities - Json
// ============================================================================
#include "Utilities/Json/JsonHelperTest.h"

// ============================================================================
// Vehicle
// ============================================================================
#include "ComponentInformationCacheTest.h"
#include "ComponentInformationTranslationTest.h"
#include "FTPManagerTest.h"
#include "MAVLinkLogManagerTest.h"
#include "RequestMessageTest.h"
#include "SendMavCommandWithHandlerTest.h"
#include "SendMavCommandWithSignallingTest.h"
#include "VehicleLinkManagerTest.h"
#include "InitialConnectTest.h"

// ============================================================================
// Video
// ============================================================================
#include "VideoReceiverTest.h"
#include "VideoSettingsTest.h"
#include "VideoUrlConstructionTest.h"

// ============================================================================
// Not Yet Implemented
// ============================================================================
// #include "FlightGearUnitTest.h"
// #include "LinkManagerTest.h"
// #include "SendMavCommandTest.h"
// #include "TCPLinkTest.h"

// ============================================================================

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(UnitTestListLog, "Test.UnitTestList")

// ============================================================================
// Test Registration
// ============================================================================

namespace {

/// Registers all unit tests. Called once at startup.
void registerAllTests()
{
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;

    // ADSB
    UT_REGISTER_TEST(ADSBVehicleTest)
    UT_REGISTER_TEST(ADSBTCPLinkTest)
    UT_REGISTER_TEST(ADSBVehicleManagerTest)

    // AnalyzeView
    UT_REGISTER_TEST(ExifParserTest)
    UT_REGISTER_TEST(GeoTagControllerTest)
    UT_REGISTER_TEST(LogDownloadTest)
    UT_REGISTER_TEST(PX4LogParserTest)
    UT_REGISTER_TEST(ULogParserTest)
    UT_REGISTER_TEST(MavlinkLogTest)

    // Camera
    UT_REGISTER_TEST(QGCCameraManagerTest)

    // Comms
    UT_REGISTER_TEST(LinkConfigurationTest)
    UT_REGISTER_TEST(MockConfigurationTest)
    UT_REGISTER_TEST(MockLinkConnectedTest)
    UT_REGISTER_TEST(MockLinkTest)
    UT_REGISTER_TEST(QGCSerialPortInfoTest)
    UT_REGISTER_TEST(TCPConfigurationTest)

    // FactSystem
    UT_REGISTER_TEST(FactSystemTestGeneric)
    UT_REGISTER_TEST(FactSystemTestPX4)
    UT_REGISTER_TEST(ParameterManagerTest)

    // FollowMe
    UT_REGISTER_TEST(FollowMeTest)

    // GPS
    UT_REGISTER_TEST(GpsTest)

    // MAVLink
    UT_REGISTER_TEST(MavlinkFTPTest)
    UT_REGISTER_TEST(MAVLinkStreamConfigTest)
    UT_REGISTER_TEST(SigningTest)
    UT_REGISTER_TEST(StatusTextHandlerTest)
    UT_REGISTER_TEST(SysStatusSensorInfoTest)

    // QmlControls
    UT_REGISTER_TEST(QGCGeoBoundingCubeTest)
    UT_REGISTER_TEST(QmlObjectListModelTest)

    // MissionManager
    UT_REGISTER_TEST(CameraCalcTest)
    UT_REGISTER_TEST(CameraSectionTest)
    UT_REGISTER_TEST(CorridorScanComplexItemTest)
    UT_REGISTER_TEST(MissionCommandTreeTest)
    // MissionControllerManagerTest is a base class - don't register
    UT_REGISTER_TEST(MissionControllerTest)
    UT_REGISTER_TEST(MissionErrorHandlingTest)
    UT_REGISTER_TEST(MissionItemTest)
    UT_REGISTER_TEST(MissionManagerTest)
    UT_REGISTER_TEST(MissionSettingsTest)
    UT_REGISTER_TEST(MissionWorkflowTest)
    UT_REGISTER_TEST(PlanFileRoundtripTest)
    UT_REGISTER_TEST(PlanMasterControllerTest)
    UT_REGISTER_TEST(QGCMapPolygonTest)
    UT_REGISTER_TEST(QGCMapPolylineTest)
    UT_REGISTER_TEST(SimpleMissionItemTest)
    UT_REGISTER_TEST(SpeedSectionTest)
    UT_REGISTER_TEST(StructureScanComplexItemTest)
    UT_REGISTER_TEST(SurveyComplexItemTest)
    UT_REGISTER_TEST(TransectStyleComplexItemTest)
    UT_REGISTER_TEST(FWLandingPatternTest)
    UT_REGISTER_TEST(LandingComplexItemTest)

    // Terrain
    UT_REGISTER_TEST(TerrainQueryTest)
    UT_REGISTER_TEST(TerrainTileTest)

    // UnitTestFramework
    UT_REGISTER_TEST(MultiSignalSpyTest)
    UT_REGISTER_TEST(TestFixturesTest)
    UT_REGISTER_TEST(VehicleTestFixtureTest)
    UT_REGISTER_TEST(MissionTestFixtureTest)
    UT_REGISTER_TEST(ParameterTestFixtureTest)

    // Utilities - Audio
    UT_REGISTER_TEST(AudioOutputTest)

    // Utilities - Compression
    UT_REGISTER_TEST(QGCArchiveModelTest)
    UT_REGISTER_TEST(QGCCompressionTest)
    UT_REGISTER_TEST(QGCStreamingDecompressionTest)

    // Utilities - FileSystem
    UT_REGISTER_TEST(QGCArchiveWatcherTest)
    UT_REGISTER_TEST(QGCFileDownloadTest)
    UT_REGISTER_TEST(QGCFileHelperTest)
    UT_REGISTER_TEST(QGCFileWatcherTest)

    // Utilities - Geo
    UT_REGISTER_TEST(GeoJsonHelperTest)
    UT_REGISTER_TEST(GeoTest)

    // Utilities - Network
    UT_REGISTER_TEST(QGCNetworkHelperTest)

    // Utilities - Shape
    UT_REGISTER_TEST(ShapeTest)

    // Utilities - Performance
    UT_REGISTER_TEST(PerformanceBenchmarkTest)

    // Utilities - Json
    UT_REGISTER_TEST(JsonHelperTest)

    // Vehicle
    UT_REGISTER_TEST(ComponentInformationCacheTest)
    UT_REGISTER_TEST(ComponentInformationTranslationTest)
    UT_REGISTER_TEST(FTPManagerTest)
    UT_REGISTER_TEST(MAVLinkLogManagerTest)
    UT_REGISTER_TEST(RequestMessageTest)
    UT_REGISTER_TEST(SendMavCommandWithHandlerTest)
    UT_REGISTER_TEST(SendMavCommandWithSignallingTest)
    UT_REGISTER_TEST(VehicleLinkManagerTest)
    UT_REGISTER_TEST(InitialConnectTest)

    // Video
    UT_REGISTER_TEST(VideoReceiverTest)
    UT_REGISTER_TEST(VideoSettingsTest)
    UT_REGISTER_TEST(VideoUrlConstructionTest)
}

} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

namespace QGCUnitTest {

int runTests(bool stress, const QStringList& unitTests, const QString& outputFile)
{
    registerAllTests();

    if (unitTests.isEmpty()) {
        qCWarning(UnitTestListLog) << "No tests specified";
        return -1;
    }

    // Validate all test names before running
    const QStringList invalid = validateTestNames(unitTests);
    if (!invalid.isEmpty()) {
        qCWarning(UnitTestListLog) << "Unknown test(s):" << invalid.join(", ");
        qCWarning(UnitTestListLog) << "Available tests:" << UnitTest::registeredTests().join(", ");
        return -static_cast<int>(invalid.size());
    }

    const int iterations = stress ? kStressIterations : 1;
    int result = 0;

    for (int i = 0; i < iterations; ++i) {
        int failures = 0;

        for (const QString& test : unitTests) {
            failures += UnitTest::run(test, outputFile);
        }

        if (failures == 0) {
            if (stress) {
                qCDebug(UnitTestListLog).noquote() << QString("ALL TESTS PASSED (iteration %1/%2)").arg(i + 1).arg(iterations);
            } else {
                qCDebug(UnitTestListLog) << "ALL TESTS PASSED";
            }
        } else {
            qCWarning(UnitTestListLog) << failures << "TESTS FAILED!";
            result = -failures;
            break;
        }
    }

    return result;
}

QStringList registeredTestNames()
{
    registerAllTests();
    return UnitTest::registeredTests();
}

int registeredTestCount()
{
    registerAllTests();
    return UnitTest::testCount();
}

bool isTestRegistered(const QString& testName)
{
    registerAllTests();
    return UnitTest::registeredTests().contains(testName);
}

QStringList validateTestNames(const QStringList& testNames)
{
    registerAllTests();
    const QStringList registered = UnitTest::registeredTests();

    // Use QSet for O(1) lookup instead of O(n) QStringList::contains()
    const QSet<QString> registeredSet(registered.cbegin(), registered.cend());

    QStringList invalid;
    invalid.reserve(testNames.size()); // Worst case

    for (const QString& name : testNames) {
        if (!registeredSet.contains(name)) {
            invalid.append(name);
        }
    }

    return invalid;
}

} // namespace QGCUnitTest
