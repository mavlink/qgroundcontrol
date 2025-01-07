/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UnitTestList.h"
#include "UnitTest.h"
#include "QGCLoggingCategory.h"

// ADSB
#include "ADSBTest.h"

// AnalyzeView
#include "ExifParserTest.h"
#include "GeoTagControllerTest.h"
// #include "MavlinkLogTest.h"
#include "LogDownloadTest.h"
#include "PX4LogParserTest.h"
#include "ULogParserTest.h"

// Audio
#include "AudioOutputTest.h"

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

// Geo
#include "GeoTest.h"

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
// Compression
#include "DecompressionTest.h"
#include "QGCFileDownloadTest.h"

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

QGC_LOGGING_CATEGORY(UnitTestsLog, "qgc.test.unittestlist")

int runTests(bool stress, QStringView unitTestOptions)
{
    // ADSB
    UT_REGISTER_TEST(ADSBTest)

    // AnalyzeView
    UT_REGISTER_TEST(ExifParserTest)
    UT_REGISTER_TEST(GeoTagControllerTest)
    // UT_REGISTER_TEST(MavlinkLogTest)
    UT_REGISTER_TEST(LogDownloadTest)
    UT_REGISTER_TEST(PX4LogParserTest)
    UT_REGISTER_TEST(ULogParserTest)

    // Audio
    UT_REGISTER_TEST(AudioOutputTest)

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

    // Geo
    UT_REGISTER_TEST(GeoTest)

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
    // Compression
    UT_REGISTER_TEST(DecompressionTest)
    UT_REGISTER_TEST(QGCFileDownloadTest)

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
        // Run the test
        const int failures = UnitTest::run(unitTestOptions);
        if (failures == 0) {
            qDebug() << "ALL TESTS PASSED";
            result = 0;
        } else {
            qDebug() << failures << " TESTS FAILED!";
            result = -failures;
            break;
        }
    }

    return result;
}
