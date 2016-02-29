/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

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
#include "ComplexMissionItemTest.h"
#include "MissionControllerTest.h"
#include "MissionManagerTest.h"
#include "RadioConfigTest.h"
#include "SetupViewTest.h"
#include "MavlinkLogTest.h"

UT_REGISTER_TEST(FactSystemTestGeneric)
UT_REGISTER_TEST(FactSystemTestPX4)
UT_REGISTER_TEST(FileDialogTest)
UT_REGISTER_TEST(FlightGearUnitTest)
UT_REGISTER_TEST(GeoTest)
UT_REGISTER_TEST(LinkManagerTest)
UT_REGISTER_TEST(MavlinkLogTest)
UT_REGISTER_TEST(MessageBoxTest)
UT_REGISTER_TEST(MissionItemTest)
UT_REGISTER_TEST(SimpleMissionItemTest)
UT_REGISTER_TEST(ComplexMissionItemTest)
UT_REGISTER_TEST(MissionControllerTest)
UT_REGISTER_TEST(MissionManagerTest)
UT_REGISTER_TEST(RadioConfigTest)

// List of unit test which are currently disabled.
// If disabling a new test, include reason in comment.

// Why is this one off?
//UT_REGISTER_TEST(FileManagerTest)

// FIXME: Temporarily disabled until this can be stabilized
//UT_REGISTER_TEST(MainWindowTest)

// This unit test has gotten too flaky to run reliably under TeamCity. Removing for now till there is
// time to debug.
//UT_REGISTER_TEST(TCPLinkUnitTest)

// Windows based unit tests are not working correctly. Needs major reword to support.
//UT_REGISTER_TEST(SetupViewTest)
