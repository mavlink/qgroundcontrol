#include "PlanManagerOpaqueIdTest.h"

#include <algorithm>

#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

#include "GeoFenceController.h"
#include "GeoFenceManager.h"
#include "MissionController.h"
#include "MissionItem.h"
#include "MissionManager.h"
#include "MockLink.h"
#include "PlanMasterController.h"
#include "RallyPointController.h"
#include "RallyPointManager.h"
#include "SimpleMissionItem.h"
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

/// Number of items in the plan editor of the kind the vehicle appends
int editorAppendedItemCount(PlanMasterController& planController, MAV_MISSION_TYPE planType)
{
    switch (planType) {
        case MAV_MISSION_TYPE_FENCE:
            return planController.geoFenceController()->circles()->count();
        case MAV_MISSION_TYPE_RALLY:
            return planController.rallyPointController()->points()->count();
        default: {
            QmlObjectListModel* const visualItems = planController.missionController()->visualItems();
            int count = 0;
            for (int i = 0; i < visualItems->count(); i++) {
                const SimpleMissionItem* const item = visualItems->value<SimpleMissionItem*>(i);
                if (item && (item->command() == static_cast<int>(appendedCommand(planType)))) {
                    count++;
                }
            }
            return count;
        }
    }
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

void PlanManagerOpaqueIdTest::_connect(MAV_AUTOPILOT autopilot, MockConfiguration::Options options)
{
    if ((autopilot == MAV_AUTOPILOT_ARDUPILOTMEGA) && !apmFirmwareSupported()) {
        QSKIP("ArduPilot support not registered in this build");
    }

    _connectMockLink(autopilot, MockConfiguration::FailNone, options | MockConfiguration::OptionPlanOpaqueIds);
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

void PlanManagerOpaqueIdTest::_flyViewFollowsVehicleChange_data()
{
    QTest::addColumn<MAV_AUTOPILOT>("autopilot");

    QTest::newRow("PX4") << MAV_AUTOPILOT_PX4;
    QTest::newRow("ArduPilot") << MAV_AUTOPILOT_ARDUPILOTMEGA;
}

void PlanManagerOpaqueIdTest::_flyViewFollowsVehicleChange()
{
    QFETCH(MAV_AUTOPILOT, autopilot);

    _connect(autopilot, MockConfiguration::OptionPreloadMission);
    if (QTest::currentTestFailed()) {
        return;
    }

    PlanMasterController flyController;
    flyController.setFlyView(true);
    flyController.start();
    MissionController* const flyMission = flyController.missionController();
    QVERIFY_TRUE_WAIT(flyMission->visualItems()->count() > 1, TestTimeout::mediumMs());

    // Plan View holding a plan the user is editing must not be replaced
    PlanMasterController planController;
    planController.setFlyView(false);
    planController.start();
    MissionController* const planMission = planController.missionController();
    QVERIFY_TRUE_WAIT(planMission->visualItems()->count() > 1, TestTimeout::mediumMs());
    QVERIFY(planMission->insertSimpleMissionItem(mockLink()->homeCoordinate().atDistanceAndAzimuth(100.0, 0.0),
                                                 planMission->visualItems()->count()));
    const int planCount = planMission->visualItems()->count();

    const int flyCount = flyMission->visualItems()->count();
    QSignalSpy spyFly(flyMission, &MissionController::newItemsFromVehicle);
    QVERIFY(spyFly.isValid());

    mockLink()->appendPlanItemOnVehicle(MAV_MISSION_TYPE_MISSION);

    QVERIFY(spyFly.wait(TestTimeout::longMs()));
    QCOMPARE(flyMission->visualItems()->count(), flyCount + 1);
    const SimpleMissionItem* const lastItem = flyMission->visualItems()->value<SimpleMissionItem*>(flyCount);
    QVERIFY(lastItem);
    QCOMPARE(lastItem->command(), static_cast<int>(MAV_CMD_NAV_LOITER_UNLIM));
    QCOMPARE(planMission->visualItems()->count(), planCount);
}

void PlanManagerOpaqueIdTest::_planViewOffersVehicleChange_data()
{
    _addPlanTypeRows();
}

void PlanManagerOpaqueIdTest::_planViewOffersVehicleChange()
{
    QFETCH(MAV_AUTOPILOT, autopilot);
    QFETCH(MAV_MISSION_TYPE, planType);

    _connect(autopilot, MockConfiguration::OptionPreloadMission);
    if (QTest::currentTestFailed()) {
        return;
    }

    PlanMasterController planController;
    planController.setFlyView(false);
    planController.start();

    // Give the editor a plan of its own
    const QGeoCoordinate home = mockLink()->homeCoordinate();
    switch (planType) {
        case MAV_MISSION_TYPE_FENCE:
            planController.geoFenceController()->addInclusionCircle(home.atDistanceAndAzimuth(300.0, 315.0),
                                                                    home.atDistanceAndAzimuth(300.0, 135.0));
            break;
        case MAV_MISSION_TYPE_RALLY:
            planController.rallyPointController()->addPoint(home.atDistanceAndAzimuth(100.0, 90.0));
            break;
        default:
            QVERIFY_TRUE_WAIT(planController.missionController()->visualItems()->count() > 1, TestTimeout::mediumMs());
            QVERIFY(planController.missionController()->insertSimpleMissionItem(
                home.atDistanceAndAzimuth(100.0, 0.0), planController.missionController()->visualItems()->count()));
            break;
    }
    const int editorCount = editorAppendedItemCount(planController, planType);

    QSignalSpy spyChanges(&planController, &PlanMasterController::vehiclePlanChangesChanged);
    QVERIFY(spyChanges.isValid());

    // A change on the vehicle is offered without replacing the plan being edited
    mockLink()->appendPlanItemOnVehicle(planType);
    QVERIFY(spyChanges.wait(TestTimeout::longMs()));
    QCOMPARE(planController.vehiclePlanChanges().count(), 1);
    QVERIFY(planController.dirtyForUpload());
    QCOMPARE(editorAppendedItemCount(planController, planType), editorCount);

    planController.ignoreVehiclePlanChanges();
    QVERIFY(planController.vehiclePlanChanges().isEmpty());

    // Downloading after another change replaces the editor's plan with the vehicle's
    spyChanges.clear();
    mockLink()->appendPlanItemOnVehicle(planType);
    QVERIFY(spyChanges.wait(TestTimeout::longMs()));
    QCOMPARE(planController.vehiclePlanChanges().count(), 1);
    planController.loadFromVehicle();
    QVERIFY_TRUE_WAIT(!planController.syncInProgress() && planController.vehiclePlanChanges().isEmpty(),
                      TestTimeout::longMs());
    QCOMPARE(editorAppendedItemCount(planController, planType), 2);
}
