/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


// We keep the list of all unit tests in a global location so it's easier to see which
// ones are enabled/disabled

#include "FactSystemTestGeneric.h"
#include "FactSystemTestPX4.h"
#include "FileDialogTest.h"
#include "FlightGearTest.h"
#include "GeoTest.h"
#include "LinkManagerTest.h"
#include "MessageBoxTest.h"
#include "MissionItemTest.h"
#include "SimpleMissionItemTest.h"
#include "SurveyMissionItemTest.h"
#include "MissionControllerTest.h"
#include "MissionManagerTest.h"
#include "RadioConfigTest.h"
#include "MavlinkLogTest.h"
#include "MainWindowTest.h"
#include "FileManagerTest.h"
#include "TCPLinkTest.h"
#include "ParameterManagerTest.h"
#include "MissionCommandTreeTest.h"
#include "LogDownloadTest.h"
#include "SendMavCommandTest.h"
#include "VisualMissionItemTest.h"
#include "CameraSectionTest.h"
#include "SpeedSectionTest.h"

UT_REGISTER_TEST(FactSystemTestGeneric)
UT_REGISTER_TEST(FactSystemTestPX4)
UT_REGISTER_TEST(FileDialogTest)
UT_REGISTER_TEST(FlightGearUnitTest)
UT_REGISTER_TEST(GeoTest)
UT_REGISTER_TEST(LinkManagerTest)
UT_REGISTER_TEST(MessageBoxTest)
UT_REGISTER_TEST(MissionItemTest)
UT_REGISTER_TEST(SimpleMissionItemTest)
UT_REGISTER_TEST(MissionControllerTest)
UT_REGISTER_TEST(MissionManagerTest)
UT_REGISTER_TEST(RadioConfigTest)
UT_REGISTER_TEST(TCPLinkTest)
UT_REGISTER_TEST(ParameterManagerTest)
UT_REGISTER_TEST(MissionCommandTreeTest)
UT_REGISTER_TEST(LogDownloadTest)
UT_REGISTER_TEST(SendMavCommandTest)
UT_REGISTER_TEST(SurveyMissionItemTest)
UT_REGISTER_TEST(CameraSectionTest)
UT_REGISTER_TEST(SpeedSectionTest)

// List of unit test which are currently disabled.
// If disabling a new test, include reason in comment.

// FIXME: Temporarily disabled until this can be stabilized
//UT_REGISTER_TEST(MainWindowTest)

// Onboard file support has been removed until it can be make to work correctly
//UT_REGISTER_TEST(FileManagerTest)

// Needs to be update for latest changes
//UT_REGISTER_TEST(MavlinkLogTest)
