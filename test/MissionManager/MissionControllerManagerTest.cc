#include "MissionControllerManagerTest.h"

#include "MultiSignalSpy.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"

MissionControllerManagerTest::~MissionControllerManagerTest() = default;

void MissionControllerManagerTest::cleanup()
{
    _multiSpyMissionManager.reset();
    VehicleTestManualConnect::cleanup();
}

void MissionControllerManagerTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    _connectMockLink(firmwareType);
    // Wait for the Mission Manager to finish it's initial load
    _missionManager = MultiVehicleManager::instance()->activeVehicle()->missionManager();
    QVERIFY(_missionManager);
    _multiSpyMissionManager = std::make_unique<MultiSignalSpy>();
    QVERIFY(_multiSpyMissionManager);
    QVERIFY(_multiSpyMissionManager->init(_missionManager));
    if (_missionManager->inProgress()) {
        QVERIFY_WAIT_SIGNAL((*_multiSpyMissionManager), "newMissionItemsAvailable", _missionManagerSignalWaitTime);
        QVERIFY_WAIT_SIGNAL((*_multiSpyMissionManager), "inProgressChanged", _missionManagerSignalWaitTime);
        QVERIFY(_multiSpyMissionManager->emittedByMask(
            _multiSpyMissionManager->mask("newMissionItemsAvailable", "inProgressChanged")));
    }
    QVERIFY(!_missionManager->inProgress());
    QCOMPARE(_missionManager->missionItems().count(), 0);
    _multiSpyMissionManager->clearAllSignals();
}

/// Checks the state of the inProgress value and signal to match the specified value
void MissionControllerManagerTest::_checkInProgressValues(bool inProgress)
{
    QCOMPARE(_missionManager->inProgress(), inProgress);
    QCOMPARE(_multiSpyMissionManager->argument<bool>("inProgressChanged"), inProgress);
}

#include "UnitTest.h"

UT_REGISTER_TEST(MissionControllerManagerTest, TestLabel::Integration, TestLabel::MissionManager)
