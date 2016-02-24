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
#include "QGCApplication.h"

MissionControllerManagerTest::MissionControllerManagerTest(void)
{
    
}

void MissionControllerManagerTest::cleanup(void)
{
    delete _multiSpyMissionManager;
    _multiSpyMissionManager = NULL;

    UnitTest::cleanup();
}

void MissionControllerManagerTest::_initForFirmwareType(MAV_AUTOPILOT firmwareType)
{
    _connectMockLink(firmwareType);
    
    // Wait for the Mission Manager to finish it's initial load
    
    _missionManager = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->missionManager();
    QVERIFY(_missionManager);
    
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
