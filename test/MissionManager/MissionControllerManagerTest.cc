#include "MissionControllerManagerTest.h"
#include "MultiVehicleManager.h"
#include "Vehicle.h"
#include "MultiSignalSpy.h"

#include <QtTest/QTest>

void MissionControllerManagerTest::cleanup()
{
    delete _multiSpyMissionManager;
    _multiSpyMissionManager = nullptr;

    _disconnectMockLink();
    UnitTest::cleanup();
}

void MissionControllerManagerTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    _connectMockLink(firmwareType);

    // Wait for the Mission Manager to finish it's initial load

    _missionManager = MultiVehicleManager::instance()->activeVehicle()->missionManager();
    VERIFY_NOT_NULL(_missionManager);

    _multiSpyMissionManager = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpyMissionManager);
    QCOMPARE(_multiSpyMissionManager->init(_missionManager), true);

    if (_missionManager->inProgress()) {
        // MockLink responds quickly, use shorter timeout for initialization
        _multiSpyMissionManager->waitForSignal("newMissionItemsAvailable", _missionManagerSuccessWaitTime);
        _multiSpyMissionManager->waitForSignal("inProgressChanged", _missionManagerSuccessWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkSignalByMask(_multiSpyMissionManager->mask("newMissionItemsAvailable", "inProgressChanged")), true);
    }

    QVERIFY(!_missionManager->inProgress());
    QCOMPARE(_missionManager->missionItems().count(), 0);
    _multiSpyMissionManager->clearAllSignals();
}

/// Checks the state of the inProgress value and signal to match the specified value
void MissionControllerManagerTest::_checkInProgressValues(bool inProgress)
{
    QCOMPARE(_missionManager->inProgress(), inProgress);
    QCOMPARE(_multiSpyMissionManager->pullBoolFromSignal("inProgressChanged"), inProgress);
}
