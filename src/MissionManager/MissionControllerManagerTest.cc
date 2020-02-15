/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MissionControllerManagerTest.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"

MissionControllerManagerTest::MissionControllerManagerTest(void)
{
    
}

void MissionControllerManagerTest::cleanup(void)
{
    delete _multiSpyMissionManager;
    _multiSpyMissionManager = nullptr;

    UnitTest::cleanup();
}

void MissionControllerManagerTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    _connectMockLink(firmwareType);
    
    // Wait for the Mission Manager to finish it's initial load
    
    _missionManager = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->missionManager();
    QVERIFY(_missionManager);
    
    _rgMissionManagerSignals[newMissionItemsAvailableSignalIndex] = SIGNAL(newMissionItemsAvailable(bool));
    _rgMissionManagerSignals[sendCompleteSignalIndex] =             SIGNAL(sendComplete(bool));
    _rgMissionManagerSignals[inProgressChangedSignalIndex] =        SIGNAL(inProgressChanged(bool));
    _rgMissionManagerSignals[errorSignalIndex] =                    SIGNAL(error(int, const QString&));

    _multiSpyMissionManager = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpyMissionManager);
    QCOMPARE(_multiSpyMissionManager->init(_missionManager, _rgMissionManagerSignals, _cMissionManagerSignals), true);
    
    if (_missionManager->inProgress()) {
        _multiSpyMissionManager->waitForSignalByIndex(newMissionItemsAvailableSignalIndex, _missionManagerSignalWaitTime);
        _multiSpyMissionManager->waitForSignalByIndex(inProgressChangedSignalIndex, _missionManagerSignalWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
    }
    
    QVERIFY(!_missionManager->inProgress());
    QCOMPARE(_missionManager->missionItems().count(), 0);
    _multiSpyMissionManager->clearAllSignals();
}

/// Checks the state of the inProgress value and signal to match the specified value
void MissionControllerManagerTest::_checkInProgressValues(bool inProgress)
{
    QCOMPARE(_missionManager->inProgress(), inProgress);
    QCOMPARE(_multiSpyMissionManager->pullBoolFromSignalIndex(inProgressChangedSignalIndex), inProgress);
}
