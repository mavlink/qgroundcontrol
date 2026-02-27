#include "PlanMasterControllerTest.h"

#include "MissionManager.h"
#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "PlanMasterController.h"
#include "Vehicle.h"

void PlanMasterControllerTest::init()
{
    UnitTest::init();
    MultiVehicleManager::instance()->init();
    _masterController = new PlanMasterController(this);
    _masterController->setFlyView(false);
    _masterController->start();
}

void PlanMasterControllerTest::cleanup()
{
    delete _masterController;
    _masterController = nullptr;
    _disconnectMockLink();
    UnitTest::cleanup();
}

void PlanMasterControllerTest::_testMissionPlannerFileLoad()
{
    _masterController->loadFromFile(":/unittest/MissionPlanner.waypoints");
    QCOMPARE(_masterController->missionController()->visualItems()->count(), 6);
}

void PlanMasterControllerTest::_testActiveVehicleChanged()
{
    // There was a defect where the PlanMasterController would, upon a new active vehicle,
    // overzelously disconnect all subscribers interested in the outgoing active vechicle.
    Vehicle* outgoingManagerVehicle = _masterController->managerVehicle();
    // spyMissionManager emulates a subscriber that should not be disconnected when
    // the active vehicle changes
    MultiSignalSpy spyMissionManager;
    spyMissionManager.init(outgoingManagerVehicle->missionManager());
    MultiSignalSpy spyMasterController;
    spyMasterController.init(_masterController);
    // Since MissionManager works with actual vehicles (which we don't have in the test cycle)
    // we have to be a bit creative emulating a signal emitted by a MissionManager.
    emit outgoingManagerVehicle->missionManager()->error(0, "");
    auto missionManagerErrorSignalMask = spyMissionManager.mask("error");
    QVERIFY(spyMissionManager.onlyEmittedOnceByMask(missionManagerErrorSignalMask));
    spyMissionManager.clearSignal("error");
    QVERIFY(spyMissionManager.noneEmitted());
    _connectMockLink(MAV_AUTOPILOT_PX4);
    auto masterControllerMgrVehicleChanged = spyMasterController.mask("managerVehicleChanged");
    QVERIFY(spyMasterController.emittedOnceByMask(masterControllerMgrVehicleChanged));
    emit outgoingManagerVehicle->missionManager()->error(0, "");
    // This signal was affected by the defect - it wouldn't reach the subscriber. Here
    // we make sure it does.
    QVERIFY(spyMissionManager.onlyEmittedOnceByMask(missionManagerErrorSignalMask));
}

#include "UnitTest.h"

UT_REGISTER_TEST(PlanMasterControllerTest, TestLabel::Integration, TestLabel::MissionManager)
