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

#include "MissionControllerManagerTest.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"

UT_REGISTER_TEST(MissionControllerManagerTest)

MissionControllerManagerTest::MissionControllerManagerTest(void)
    : _mockLink(NULL)
{
    
}

void MissionControllerManagerTest::cleanup(void)
{
    delete _multiSpyMissionManager;
    _multiSpyMissionManager = NULL;

    LinkManager::instance()->disconnectLink(_mockLink);
    _mockLink = NULL;
    QTest::qWait(1000); // Need to allow signals to move between threads

    UnitTest::cleanup();
}

void MissionControllerManagerTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    LinkManager* linkMgr = LinkManager::instance();
    Q_CHECK_PTR(linkMgr);
    
    _mockLink = new MockLink();
    Q_CHECK_PTR(_mockLink);
    _mockLink->setFirmwareType(firmwareType);
    LinkManager::instance()->_addLink(_mockLink);
    
    linkMgr->connectLink(_mockLink);
    
    // Wait for the Vehicle to work it's way through the various threads
    
    QSignalSpy spyVehicle(MultiVehicleManager::instance(), SIGNAL(activeVehicleChanged(Vehicle*)));
    QCOMPARE(spyVehicle.wait(5000), true);
    
    // Wait for the Mission Manager to finish it's initial load
    
    _missionManager = MultiVehicleManager::instance()->activeVehicle()->missionManager();
    QVERIFY(_missionManager);
    
    _rgMissionManagerSignals[canEditChangedSignalIndex] =             SIGNAL(canEditChanged(bool));
    _rgMissionManagerSignals[newMissionItemsAvailableSignalIndex] =   SIGNAL(newMissionItemsAvailable(void));
    _rgMissionManagerSignals[inProgressChangedSignalIndex] =          SIGNAL(inProgressChanged(bool));
    _rgMissionManagerSignals[errorSignalIndex] =                      SIGNAL(error(int, const QString&));

    _multiSpyMissionManager = new MultiSignalSpy();
    Q_CHECK_PTR(_multiSpyMissionManager);
    QCOMPARE(_multiSpyMissionManager->init(_missionManager, _rgMissionManagerSignals, _cMissionManagerSignals), true);
    
    if (_missionManager->inProgress()) {
        _multiSpyMissionManager->waitForSignalByIndex(newMissionItemsAvailableSignalIndex, _missionManagerSignalWaitTime);
        _multiSpyMissionManager->waitForSignalByIndex(inProgressChangedSignalIndex, _missionManagerSignalWaitTime);
        QCOMPARE(_multiSpyMissionManager->checkSignalByMask(newMissionItemsAvailableSignalMask | inProgressChangedSignalMask), true);
        QCOMPARE(_multiSpyMissionManager->checkNoSignalByMask(canEditChangedSignalIndex), true);
    }
    
    QVERIFY(!_missionManager->inProgress());
    QCOMPARE(_missionManager->missionItems()->count(), 0);
    _multiSpyMissionManager->clearAllSignals();
}

/// Checks the state of the inProgress value and signal to match the specified value
void MissionControllerManagerTest::_checkInProgressValues(bool inProgress)
{
    QCOMPARE(_missionManager->inProgress(), inProgress);
    QSignalSpy* spy = _multiSpyMissionManager->getSpyByIndex(inProgressChangedSignalIndex);
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toBool(), inProgress);
}
