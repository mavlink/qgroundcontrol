/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


// We keep the list of all unit tests in a global location so it's easier to see which
// ones are enabled/disabled

#include "FactSystemTestGeneric.h"
#include "FactSystemTestPX4.h"
//#include "FileDialogTest.h"
//#include "FlightGearTest.h"
#include "GeoTest.h"
#include "LinkManagerTest.h"
//#include "MessageBoxTest.h"
#include "MissionItemTest.h"
#include "SimpleMissionItemTest.h"
#include "SurveyComplexItemTest.h"
#include "MissionControllerTest.h"
#include "MissionManagerTest.h"
//#include "RadioConfigTest.h"
#include "MavlinkLogTest.h"
//#include "MainWindowTest.h"
//#include "FileManagerTest.h"
#include "TCPLinkTest.h"
#include "ParameterManagerTest.h"
#include "MissionCommandTreeTest.h"
//#include "LogDownloadTest.h"
#include "SendMavCommandTest.h"
#include "VisualMissionItemTest.h"
#include "CameraSectionTest.h"
#include "SpeedSectionTest.h"
#include "PlanMasterControllerTest.h"
#include "MissionSettingsTest.h"
#include "QGCMapPolygonTest.h"
#include "AudioOutputTest.h"
#include "StructureScanComplexItemTest.h"
#include "QGCMapPolylineTest.h"
#include "CorridorScanComplexItemTest.h"
#include "TransectStyleComplexItemTest.h"
#include "CameraCalcTest.h"
#include "FWLandingPatternTest.h"

UT_REGISTER_TEST(FactSystemTestGeneric)
UT_REGISTER_TEST(FactSystemTestPX4)
//UT_REGISTER_TEST(FileDialogTest)
//UT_REGISTER_TEST(FlightGearUnitTest)
UT_REGISTER_TEST(GeoTest)
UT_REGISTER_TEST(LinkManagerTest)
//UT_REGISTER_TEST(MessageBoxTest)
UT_REGISTER_TEST(MissionItemTest)
UT_REGISTER_TEST(SimpleMissionItemTest)
UT_REGISTER_TEST(MissionControllerTest)
UT_REGISTER_TEST(MissionManagerTest)
//UT_REGISTER_TEST(RadioConfigTest)
UT_REGISTER_TEST(TCPLinkTest)
//UT_REGISTER_TEST(FileManagerTest)
UT_REGISTER_TEST(ParameterManagerTest)
UT_REGISTER_TEST(MissionCommandTreeTest)
//UT_REGISTER_TEST(LogDownloadTest)
UT_REGISTER_TEST(SendMavCommandTest)
UT_REGISTER_TEST(SurveyComplexItemTest)
UT_REGISTER_TEST(CameraSectionTest)
UT_REGISTER_TEST(SpeedSectionTest)
UT_REGISTER_TEST(PlanMasterControllerTest)
UT_REGISTER_TEST(MissionSettingsTest)
UT_REGISTER_TEST(QGCMapPolygonTest)
UT_REGISTER_TEST(AudioOutputTest)
UT_REGISTER_TEST(StructureScanComplexItemTest)
UT_REGISTER_TEST(CorridorScanComplexItemTest)
UT_REGISTER_TEST(TransectStyleComplexItemTest)
UT_REGISTER_TEST(QGCMapPolylineTest)
UT_REGISTER_TEST(CameraCalcTest)
UT_REGISTER_TEST(FWLandingPatternTest)

// List of unit test which are currently disabled.
// If disabling a new test, include reason in comment.

// FIXME: Temporarily disabled until this can be stabilized
//UT_REGISTER_TEST(MainWindowTest)

// Needs to be update for latest updates
//UT_REGISTER_TEST(MavlinkLogTest)
