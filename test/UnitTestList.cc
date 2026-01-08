#include "UnitTestList.h"
#include "UnitTest.h"
#include "QGCLoggingCategory.h"

// ADSB
#include "ADSBTest.h"

// AnalyzeView
#include "ExifParserTest.h"
// #include "GeoTagControllerTest.h"
// #include "MavlinkLogTest.h"
#include "LogDownloadTest.h"
#include "PX4LogParserTest.h"
// #include "ULogParserTest.h"

// AutoPilotPlugins
// #include "RadioConfigTest.h"

// Camera
#include "QGCCameraManagerTest.h"

// Comms
#include "QGCSerialPortInfoTest.h"

// FactSystem
#include "FactSystemTestGeneric.h"
#include "FactSystemTestPX4.h"
#include "ParameterManagerTest.h"

// FollowMe
#include "FollowMeTest.h"

// GPS
#include "GpsTest.h"

// MAVLink
#include "StatusTextHandlerTest.h"
#include "SigningTest.h"

// MissionManager
#include "CameraCalcTest.h"
#include "CameraSectionTest.h"
#include "CorridorScanComplexItemTest.h"
// #include "FWLandingPatternTest.h"
// #include "LandingComplexItemTest.h"
// #include "MissionCommandTreeEditorTest.h"
#include "MissionCommandTreeTest.h"
#include "MissionControllerManagerTest.h"
#include "MissionControllerTest.h"
#include "MissionItemTest.h"
#include "MissionManagerTest.h"
#include "MissionSettingsTest.h"
#include "PlanMasterControllerTest.h"
#include "QGCMapPolygonTest.h"
#include "QGCMapPolylineTest.h"
// #include "SectionTest.h"
#include "SimpleMissionItemTest.h"
#include "SpeedSectionTest.h"
#include "StructureScanComplexItemTest.h"
#include "SurveyComplexItemTest.h"
#include "TransectStyleComplexItemTest.h"
// #include "VisualMissionItemTest.h"

// qgcunittest
#include "ComponentInformationCacheTest.h"
#include "ComponentInformationTranslationTest.h"

// QmlControls

// Terrain
#include "TerrainQueryTest.h"
#include "TerrainTileTest.h"

// UI

// Utilities
// Audio
#include "AudioOutputTest.h"
// Compression
#include "QGCArchiveModelTest.h"
#include "QGCCompressionTest.h"
#include "QGCStreamingDecompressionTest.h"
// FileSystem
#include "QGCArchiveWatcherTest.h"
#include "QGCFileDownloadTest.h"
#include "QGCFileHelperTest.h"
#include "QGCFileWatcherTest.h"
// Geo
#include "GeoTest.h"
// Shape
#include "ShapeTest.h"

// Vehicle
// Components
#include "ComponentInformationCacheTest.h"
#include "ComponentInformationTranslationTest.h"
#include "FTPManagerTest.h"
// #include "InitialConnectTest.h"
#include "MAVLinkLogManagerTest.h"
// #include "RequestMessageTest.h"
// #include "SendMavCommandWithHandlerTest.h"
// #include "SendMavCommandWithSignalingTest.h"
#include "VehicleLinkManagerTest.h"

// Missing
// #include "FlightGearUnitTest.h"
// #include "LinkManagerTest.h"
// #include "SendMavCommandTest.h"
// #include "TCPLinkTest.h"

int QGCUnitTest::runTests(bool stress, const QStringList& unitTests)
{
    // ADSB
    UT_REGISTER_TEST(ADSBTest)

    // AnalyzeView
    UT_REGISTER_TEST(ExifParserTest)
    // UT_REGISTER_TEST(GeoTagControllerTest)
    // UT_REGISTER_TEST(MavlinkLogTest)
    UT_REGISTER_TEST(LogDownloadTest)
    UT_REGISTER_TEST(PX4LogParserTest)
    // UT_REGISTER_TEST(ULogParserTest)

    // AutoPilotPlugins
    // UT_REGISTER_TEST(RadioConfigTest)

    // Camera
    UT_REGISTER_TEST(QGCCameraManagerTest)

    // Comms
    UT_REGISTER_TEST(QGCSerialPortInfoTest)

    // FactSystem
    UT_REGISTER_TEST(FactSystemTestGeneric)
    UT_REGISTER_TEST(FactSystemTestPX4)
    UT_REGISTER_TEST(ParameterManagerTest)

    // FollowMe
    UT_REGISTER_TEST(FollowMeTest)

    // GPS
    // UT_REGISTER_TEST(GpsTest)

    // MAVLink
    UT_REGISTER_TEST(StatusTextHandlerTest)
    UT_REGISTER_TEST(SigningTest)

    // MissionManager
    UT_REGISTER_TEST(CameraCalcTest)
    UT_REGISTER_TEST(CameraSectionTest)
    UT_REGISTER_TEST(CorridorScanComplexItemTest)
    // UT_REGISTER_TEST(FWLandingPatternTest)
    // UT_REGISTER_TEST(LandingComplexItemTest)
    // UT_REGISTER_TEST_STANDALONE(MissionCommandTreeEditorTest)
    UT_REGISTER_TEST(MissionCommandTreeTest)
    UT_REGISTER_TEST(MissionControllerManagerTest)
    UT_REGISTER_TEST(MissionControllerTest)
    UT_REGISTER_TEST(MissionItemTest)
    UT_REGISTER_TEST(MissionManagerTest)
    UT_REGISTER_TEST(MissionSettingsTest)
    UT_REGISTER_TEST(PlanMasterControllerTest)
    UT_REGISTER_TEST(QGCMapPolygonTest)
    UT_REGISTER_TEST(QGCMapPolylineTest)
    // UT_REGISTER_TEST(SectionTest)
    UT_REGISTER_TEST(SimpleMissionItemTest)
    UT_REGISTER_TEST(SpeedSectionTest)
    UT_REGISTER_TEST(StructureScanComplexItemTest)
    UT_REGISTER_TEST(SurveyComplexItemTest)
    UT_REGISTER_TEST(TransectStyleComplexItemTest)
    // UT_REGISTER_TEST(VisualMissionItemTest)

    // qgcunittest

    // QmlControls

    // Terrain
    UT_REGISTER_TEST(TerrainQueryTest)
    UT_REGISTER_TEST(TerrainTileTest)

    // UI

    // Utilities
    // Audio
    UT_REGISTER_TEST(AudioOutputTest)
    // Compression
    UT_REGISTER_TEST(QGCArchiveModelTest)
    UT_REGISTER_TEST(QGCCompressionTest)
    UT_REGISTER_TEST(QGCStreamingDecompressionTest)
    // FileSystem
    UT_REGISTER_TEST(QGCArchiveWatcherTest)
    UT_REGISTER_TEST(QGCFileDownloadTest)
    UT_REGISTER_TEST(QGCFileHelperTest)
    UT_REGISTER_TEST(QGCFileWatcherTest)
    // Geo
    UT_REGISTER_TEST(GeoTest)
    // Shape
    UT_REGISTER_TEST(ShapeTest)

    // Vehicle
    // Components
    UT_REGISTER_TEST(ComponentInformationCacheTest)
    UT_REGISTER_TEST(ComponentInformationTranslationTest)
    UT_REGISTER_TEST(FTPManagerTest)
    // UT_REGISTER_TEST(InitialConnectTest)
    UT_REGISTER_TEST(MAVLinkLogManagerTest)
    // UT_REGISTER_TEST(RequestMessageTest)
    // UT_REGISTER_TEST(SendMavCommandWithHandlerTest)
    // UT_REGISTER_TEST(SendMavCommandWithSignalingTest)
    UT_REGISTER_TEST(VehicleLinkManagerTest)

    // Missing
    // UT_REGISTER_TEST(FlightGearUnitTest)
    // UT_REGISTER_TEST(LinkManagerTest)
    // UT_REGISTER_TEST(SendMavCommandTest)
    // UT_REGISTER_TEST(TCPLinkTest)

    int result = 0;
    for (int i=0; i < (stress ? 20 : 1); i++) {
        int failures = 0;
        for (const QString& test: unitTests) {
            // Run the test
            failures += UnitTest::run(test);
        }

        if (failures == 0) {
            qDebug() << "ALL TESTS PASSED";
            result = 0;
        } else {
            qWarning() << failures << "TESTS FAILED!";
            result = -failures;
            break;
        }
    }

    return result;
}
