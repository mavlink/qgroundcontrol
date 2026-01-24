#include "PlanMasterControllerTest.h"
#include "MultiSignalSpy.h"
#include "MissionManager.h"
#include "PlanMasterController.h"
#include "Vehicle.h"

#include <QtTest/QTest>

void PlanMasterControllerTest::cleanup()
{
    // _testActiveVehicleChanged may have connected a MockLink
    _disconnectMockLink();
    OfflinePlanTest::cleanup();
}

void PlanMasterControllerTest::_testMissionPlannerFileLoad()
{
    planController()->loadFromFile(":/unittest/MissionPlanner.waypoints");
    QCOMPARE(missionController()->visualItems()->count(), 6);
}

void PlanMasterControllerTest::_testActiveVehicleChanged() {
    // There was a defect where the PlanMasterController would, upon a new active vehicle,
    // overzelously disconnect all subscribers interested in the outgoing active vechicle.
    Vehicle* outgoingManagerVehicle = planController()->managerVehicle();
    VERIFY_NOT_NULL(outgoingManagerVehicle);

    // spyMissionManager emulates a subscriber that should not be disconnected when
    // the active vehicle changes
    MultiSignalSpy spyMissionManager;
    QVERIFY(spyMissionManager.init(outgoingManagerVehicle->missionManager()));
    MultiSignalSpy spyMasterController;
    QVERIFY(spyMasterController.init(planController()));

    // Since MissionManager works with actual vehicles (which we don't have in the test cycle)
    // we have to be a bit creative emulating a signal emitted by a MissionManager.
    emit outgoingManagerVehicle->missionManager()->error(0,"");
    QVERIFY_ONLY_SIGNAL(spyMissionManager, "error");
    spyMissionManager.clearSignal("error");
    QVERIFY_NO_SIGNALS(spyMissionManager);

    _connectMockLink(MAV_AUTOPILOT_PX4);
    QVERIFY_SIGNAL_EMITTED(spyMasterController, "managerVehicleChanged");

    emit outgoingManagerVehicle->missionManager()->error(0,"");
    // This signal was affected by the defect - it wouldn't reach the subscriber. Here
    // we make sure it does.
    QVERIFY_ONLY_SIGNAL(spyMissionManager, "error");
}
