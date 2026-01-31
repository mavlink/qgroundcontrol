#include "UnitTestList.h"
#include "UnitTest.h"
#include "QGCLoggingCategory.h"

// ADSB
#include "ADSBTest.h"

// AnalyzeView
#include "DataFlashParserTest.h"
#include "ExifParserTest.h"
#include "GeoTagControllerTest.h"
#include "GeoTagDataTest.h"
#include "GeoTagImageModelTest.h"
// #include "MavlinkLogTest.h"
#include "LogDownloadStateMachineTest.h"
#include "LogDownloadTest.h"
#include "ULogParserTest.h"

// AutoPilotPlugins
// #include "RadioConfigTest.h"
// Common Sensors
#include "OrientationCalibrationStateTest.h"
#include "SensorCalibrationSideTest.h"
#include "SensorsComponentBaseTest.h"
// APM Sensors
#include "AccelCalibrationMachineTest.h"
#include "CompassCalibrationMachineTest.h"
// PX4 Sensors
#include "PX4OrientationCalibrationMachineTest.h"
#include "PX4SensorCalibrationStateMachineTest.h"
// PX4 Power
#include "PowerCalibrationStateMachineTest.h"

// Camera
#include "CameraDiscoveryStateMachineTest.h"
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
#include "PlanManagerStateMachineTest.h"
#include "PlanManagerTest.h"
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
#include "ComponentInformationManagerTest.h"
#include "ComponentInformationTranslationTest.h"
#include "RequestMetaDataTypeStateMachineTest.h"

// StateMachine
#include "QGCStateMachineTest.h"
// StateMachine - States
#include "FunctionStateTest.h"
#include "DelayStateTest.h"
#include "AsyncFunctionStateTest.h"
#include "WaitForSignalStateTest.h"
#include "ConditionalStateTest.h"
#include "SkippableAsyncStateTest.h"
#include "ParallelStateTest.h"
#include "SubMachineStateTest.h"
#include "QGCStateTest.h"
#include "QGCFinalStateTest.h"
#include "QGCHistoryStateTest.h"
// StateMachine - Transitions
#include "GuardedTransitionTest.h"
#include "InternalTransitionTest.h"
#include "MachineEventTransitionTest.h"
#include "NamedEventTransitionTest.h"
#include "SignalDataTransitionTest.h"
#include "QGCSignalTransitionTest.h"

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
// Exif
#include "ExifUtilityTest.h"
// LogParsing
#include "DataFlashUtilityTest.h"
#include "ULogUtilityTest.h"
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
#include "AutotuneStateMachineTest.h"
// Actuators
#include "ActuatorTestingStateMachineTest.h"
#include "ActuatorTestingTest.h"
#include "MotorAssignmentStateMachineTest.h"
#include "MotorAssignmentTest.h"
// FTP
#include "FTPControllerTest.h"
#include "FTPDeleteStateMachineTest.h"
#include "FTPDownloadStateMachineTest.h"
#include "FTPListDirectoryStateMachineTest.h"
#include "FTPUploadStateMachineTest.h"
// VehicleSetup
#include "RCCalibrationStateMachineTest.h"
// VehicleSetup/Firmware
#include "FirmwareUpgradeStateMachineTest.h"
#include "RemoteControlCalibrationControllerTest.h"
// Components
#include "ComponentInformationCacheTest.h"
#include "ComponentInformationTranslationTest.h"
#include "FTPManagerTest.h"
#include "InitialConnectTest.h"
#include "MAVLinkLogManagerTest.h"
// #include "RequestMessageTest.h"  // FIXME: Disabled - pre-existing test failures
#include "SendMavCommandWithHandlerTest.h"
#include "SendMavCommandWithSignallingTest.h"
#include "VehicleLinkManagerTest.h"
#include "VehicleTest.h"

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
    UT_REGISTER_TEST(DataFlashParserTest)
    UT_REGISTER_TEST(ExifParserTest)
    UT_REGISTER_TEST(GeoTagControllerTest)
    UT_REGISTER_TEST(GeoTagDataTest)
    UT_REGISTER_TEST(GeoTagImageModelTest)
    // UT_REGISTER_TEST(MavlinkLogTest)
    UT_REGISTER_TEST(LogDownloadStateMachineTest)
    UT_REGISTER_TEST(LogDownloadTest)
    UT_REGISTER_TEST(ULogParserTest)

    // AutoPilotPlugins
    // UT_REGISTER_TEST(RadioConfigTest)
    // Common Sensors
    UT_REGISTER_TEST(OrientationCalibrationStateTest)
    UT_REGISTER_TEST(SensorCalibrationSideTest)
    UT_REGISTER_TEST(SensorsComponentBaseTest)
    // APM Sensors
    UT_REGISTER_TEST(AccelCalibrationMachineTest)
    UT_REGISTER_TEST(CompassCalibrationMachineTest)
    // PX4 Sensors
    UT_REGISTER_TEST(PX4OrientationCalibrationMachineTest)
    UT_REGISTER_TEST(PX4SensorCalibrationStateMachineTest)
    // PX4 Power
    UT_REGISTER_TEST(PowerCalibrationStateMachineTest)

    // Camera
    UT_REGISTER_TEST(CameraDiscoveryStateMachineTest)
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
    UT_REGISTER_TEST(PlanManagerStateMachineTest)
    UT_REGISTER_TEST(PlanManagerTest)
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
    // Exif
    UT_REGISTER_TEST(ExifUtilityTest)
    // LogParsing
    UT_REGISTER_TEST(DataFlashUtilityTest)
    UT_REGISTER_TEST(ULogUtilityTest)
    // FileSystem
    UT_REGISTER_TEST(QGCArchiveWatcherTest)
    UT_REGISTER_TEST(QGCFileDownloadTest)
    UT_REGISTER_TEST(QGCFileHelperTest)
    UT_REGISTER_TEST(QGCFileWatcherTest)
    // Geo
    UT_REGISTER_TEST(GeoTest)
    // Shape
    UT_REGISTER_TEST(ShapeTest)

    // StateMachine
    UT_REGISTER_TEST(QGCStateMachineTest)
    // StateMachine - States
    UT_REGISTER_TEST(FunctionStateTest)
    UT_REGISTER_TEST(DelayStateTest)
    UT_REGISTER_TEST(AsyncFunctionStateTest)
    UT_REGISTER_TEST(WaitForSignalStateTest)
    UT_REGISTER_TEST(ConditionalStateTest)
    UT_REGISTER_TEST(SkippableAsyncStateTest)
    UT_REGISTER_TEST(ParallelStateTest)
    UT_REGISTER_TEST(SubMachineStateTest)
    UT_REGISTER_TEST(QGCStateTest)
    UT_REGISTER_TEST(QGCFinalStateTest)
    UT_REGISTER_TEST(QGCHistoryStateTest)
    // StateMachine - Transitions
    UT_REGISTER_TEST(GuardedTransitionTest)
    UT_REGISTER_TEST(InternalTransitionTest)
    UT_REGISTER_TEST(MachineEventTransitionTest)
    UT_REGISTER_TEST(NamedEventTransitionTest)
    UT_REGISTER_TEST(SignalDataTransitionTest)
    UT_REGISTER_TEST(QGCSignalTransitionTest)

    // Vehicle
    UT_REGISTER_TEST(AutotuneStateMachineTest)
    // Actuators
    UT_REGISTER_TEST(ActuatorTestingStateMachineTest)
    UT_REGISTER_TEST(ActuatorTestingTest)
    UT_REGISTER_TEST(MotorAssignmentStateMachineTest)
    UT_REGISTER_TEST(MotorAssignmentTest)
    // FTP
    UT_REGISTER_TEST(FTPControllerTest)
    UT_REGISTER_TEST(FTPDeleteStateMachineTest)
    UT_REGISTER_TEST(FTPDownloadStateMachineTest)
    UT_REGISTER_TEST(FTPListDirectoryStateMachineTest)
    UT_REGISTER_TEST(FTPUploadStateMachineTest)
    // VehicleSetup
    UT_REGISTER_TEST(FirmwareUpgradeStateMachineTest)
    UT_REGISTER_TEST(RCCalibrationStateMachineTest)
    UT_REGISTER_TEST(RemoteControlCalibrationControllerTest)
    // Components
    UT_REGISTER_TEST(ComponentInformationCacheTest)
    UT_REGISTER_TEST(ComponentInformationManagerTest)
    UT_REGISTER_TEST(ComponentInformationTranslationTest)
    UT_REGISTER_TEST(RequestMetaDataTypeStateMachineTest)
    UT_REGISTER_TEST(FTPManagerTest)
    UT_REGISTER_TEST(InitialConnectTest)
    UT_REGISTER_TEST(MAVLinkLogManagerTest)
    // UT_REGISTER_TEST(RequestMessageTest)  // FIXME: Disabled - pre-existing test failures
    UT_REGISTER_TEST(SendMavCommandWithHandlerTest)
    UT_REGISTER_TEST(VehicleTest)
    UT_REGISTER_TEST(SendMavCommandWithSignallingTest)
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
