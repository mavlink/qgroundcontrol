#include "PlanManagerOpaqueIdTest.h"

#include <algorithm>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GeoFenceManager.h"
#include "MissionItem.h"
#include "MissionManager.h"
#include "MockLink.h"
#include "RallyPointManager.h"
#include "Vehicle.h"

UT_REGISTER_TEST(PlanManagerOpaqueIdTest, TestLabel::Integration, TestLabel::MissionManager)

namespace {

MAV_CMD appendedCommand(MAV_MISSION_TYPE planType)
{
    // Matches the items MockLinkMissionItemHandler::appendPlanItem adds
    switch (planType) {
        case MAV_MISSION_TYPE_FENCE:
            return MAV_CMD_NAV_FENCE_CIRCLE_INCLUSION;
        case MAV_MISSION_TYPE_RALLY:
            return MAV_CMD_NAV_RALLY_POINT;
        default:
            return MAV_CMD_NAV_LOITER_UNLIM;
    }
}

bool holdsAppendedItem(PlanManager* planManager, MAV_MISSION_TYPE planType)
{
    const QList<MissionItem*>& items = planManager->missionItems();
    return std::any_of(items.cbegin(), items.cend(),
                       [planType](const MissionItem* item) { return item->command() == appendedCommand(planType); });
}

}  // namespace

void PlanManagerOpaqueIdTest::_addPlanTypeRows()
{
    QTest::addColumn<MAV_AUTOPILOT>("autopilot");
    QTest::addColumn<MAV_MISSION_TYPE>("planType");

    for (const auto& [autopilot, autopilotName] :
         {std::pair{MAV_AUTOPILOT_PX4, "PX4"}, std::pair{MAV_AUTOPILOT_ARDUPILOTMEGA, "ArduPilot"}}) {
        QTest::addRow("%s mission", autopilotName) << autopilot << MAV_MISSION_TYPE_MISSION;
        QTest::addRow("%s fence", autopilotName) << autopilot << MAV_MISSION_TYPE_FENCE;
        QTest::addRow("%s rally", autopilotName) << autopilot << MAV_MISSION_TYPE_RALLY;
    }
}

void PlanManagerOpaqueIdTest::_connect(MAV_AUTOPILOT autopilot)
{
    if ((autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    _connectMockLink(autopilot, MockConfiguration::FailNone, MockConfiguration::OptionPlanOpaqueIds);
}

PlanManager* PlanManagerOpaqueIdTest::_planManager(MAV_MISSION_TYPE planType) const
{
    switch (planType) {
        case MAV_MISSION_TYPE_FENCE:
            return vehicle()->geoFenceManager();
        case MAV_MISSION_TYPE_RALLY:
            return vehicle()->rallyPointManager();
        default:
            return vehicle()->missionManager();
    }
}

void PlanManagerOpaqueIdTest::_vehicleChangeReloads_data()
{
    _addPlanTypeRows();
}

void PlanManagerOpaqueIdTest::_vehicleChangeReloads()
{
    QFETCH(MAV_AUTOPILOT, autopilot);
    QFETCH(MAV_MISSION_TYPE, planType);

    _connect(autopilot);
    if (QTest::currentTestFailed()) {
        return;
    }

    PlanManager* const planManager = _planManager(planType);
    QSignalSpy spyLoad(planManager, &PlanManager::newMissionItemsAvailable);
    QVERIFY(spyLoad.isValid());
    mockLink()->clearReceivedMavlinkMessageCounts();

    // A changed plan streams a new non-zero id
    mockLink()->appendPlanItemOnVehicle(planType);
    QVERIFY(spyLoad.wait(TestTimeout::longMs()));
    QVERIFY(holdsAppendedItem(planManager, planType));

    // An emptied plan streams id 0, which still means a change once the vehicle has shown it supports ids
    spyLoad.clear();
    mockLink()->clearPlanOnVehicle(planType);
    QVERIFY(spyLoad.wait(TestTimeout::longMs()));
    QVERIFY(!holdsAppendedItem(planManager, planType));

    QCOMPARE(mockLink()->receivedMissionRequestListCount(planType), 2);
}

void PlanManagerOpaqueIdTest::_ownUploadDoesNotReload_data()
{
    _addPlanTypeRows();
}

void PlanManagerOpaqueIdTest::_ownUploadDoesNotReload()
{
    QFETCH(MAV_AUTOPILOT, autopilot);
    QFETCH(MAV_MISSION_TYPE, planType);

    _connect(autopilot);
    if (QTest::currentTestFailed()) {
        return;
    }

    PlanManager* const planManager = _planManager(planType);

    QList<MissionItem*> items;
    const auto makeItem = [planManager](int seq, MAV_CMD command) {
        return new MissionItem(seq, command, MAV_FRAME_GLOBAL_RELATIVE_ALT, 100, 0, 0, 0, 47.39 + (0.001 * seq), 8.54,
                               50, true /* autoContinue */, seq == 0 /* isCurrentItem */, planManager);
    };
    if (planType == MAV_MISSION_TYPE_MISSION) {
        // Home position, which PlanManager drops for firmware that does not take it in the mission
        items.append(makeItem(0, MAV_CMD_NAV_WAYPOINT));
    }
    items.append(makeItem(items.count(), appendedCommand(planType)));

    QSignalSpy spySend(planManager, &PlanManager::sendComplete);
    QVERIFY(spySend.isValid());
    planManager->writeMissionItems(items);
    QVERIFY(spySend.wait(TestTimeout::longMs()));
    QCOMPARE(spySend.first().first().toBool(), false /* error */);

    // The plan id from MISSION_ACK must match the one MISSION_CURRENT streams afterwards
    mockLink()->clearReceivedMavlinkMessageCounts();
    int missionCurrentCount = 0;
    const auto countMissionCurrent = [&missionCurrentCount](const mavlink_message_t& message) {
        if (message.msgid == MAVLINK_MSG_ID_MISSION_CURRENT) {
            missionCurrentCount++;
        }
    };
    const QMetaObject::Connection connection =
        connect(vehicle(), &Vehicle::mavlinkMessageReceived, this, countMissionCurrent);
    QVERIFY_TRUE_WAIT(missionCurrentCount >= 2, TestTimeout::longMs());
    (void) disconnect(connection);

    QVERIFY(!planManager->inProgress());
    QCOMPARE(mockLink()->receivedMissionRequestListCount(planType), 0);
}
