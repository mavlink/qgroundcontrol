/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


// AnalyzeView
#include "LogDownloadTest.h"

// Audio
#include "AudioOutputTest.h"

// comm

// Compression
#include "DecompressionTest.h"

// FactSystem
#include "FactSystemTestGeneric.h"
#include "FactSystemTestPX4.h"
#include "ParameterManagerTest.h"

// Geo
#include "GeoTest.h"

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
// #include "FileDialogTest.h"
// #include "MainWindowTest.h"
// #include "MavlinkLogTest.h"
// #include "MessageBoxTest.h"
// #include "RadioConfigTest.h"

// QmlControls

// ui

// Vehicle
#include "FTPManagerTest.h"
// #include "InitialConnectTest.h"
// #include "RequestMessageTest.h"
// #include "SendMavCommandWithHandlerTest.h"
// #include "SendMavCommandWithSignalingTest.h"

// Missing
// #include "FlightGearUnitTest.h"
// #include "LinkManagerTest.h"
// #include "SendMavCommandTest.h"
// #include "TCPLinkTest.h"


// AnalyzeView
UT_REGISTER_TEST(LogDownloadTest)

// Audio
UT_REGISTER_TEST(AudioOutputTest)

// comm

// Compression
UT_REGISTER_TEST(DecompressionTest)

// FactSystem
UT_REGISTER_TEST(FactSystemTestGeneric)
UT_REGISTER_TEST(FactSystemTestPX4)
UT_REGISTER_TEST(ParameterManagerTest)

// Geo
UT_REGISTER_TEST(GeoTest)

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
UT_REGISTER_TEST(ComponentInformationCacheTest)
UT_REGISTER_TEST(ComponentInformationTranslationTest)
// UT_REGISTER_TEST(FileDialogTest)
// UT_REGISTER_TEST(MainWindowTest)
// UT_REGISTER_TEST(MavlinkLogTest)
// UT_REGISTER_TEST(MessageBoxTest)
// UT_REGISTER_TEST(RadioConfigTest)

// QmlControls

// ui

// Vehicle
UT_REGISTER_TEST(FTPManagerTest)
// UT_REGISTER_TEST(InitialConnectTest)
// UT_REGISTER_TEST(RequestMessageTest)
// UT_REGISTER_TEST(SendMavCommandWithHandlerTest)
// UT_REGISTER_TEST(SendMavCommandWithSignalingTest)

// Missing
// UT_REGISTER_TEST(FlightGearUnitTest)
// UT_REGISTER_TEST(LinkManagerTest)
// UT_REGISTER_TEST(SendMavCommandTest)
// UT_REGISTER_TEST(TCPLinkTest)