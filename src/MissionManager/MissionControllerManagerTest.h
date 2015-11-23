/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

#ifndef MissionControllerManagerTest_H
#define MissionControllerManagerTest_H

#include "UnitTest.h"
#include "MockLink.h"
#include "MissionManager.h"
#include "MultiSignalSpy.h"

#include <QGeoCoordinate>

/// This is the base class for the MissionManager and MissionController unit tests.
class MissionControllerManagerTest : public UnitTest
{
    Q_OBJECT
    
public:
    MissionControllerManagerTest(void);
    
protected slots:
    void cleanup(void);
    
protected:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _checkInProgressValues(bool inProgress);
    
    MissionManager* _missionManager;
    
    typedef struct {
        int             sequenceNumber;
        QGeoCoordinate  coordinate;
        MAV_CMD         command;
        double          param1;
        double          param2;
        double          param3;
        double          param4;
        bool            autocontinue;
        bool            isCurrentItem;
        MAV_FRAME       frame;
    } ItemInfo_t;
    
    typedef struct {
        const char*         itemStream;
        const ItemInfo_t    expectedItem;
    } TestCase_t;

    typedef enum {
        newMissionItemsAvailableSignalIndex = 0,
        inProgressChangedSignalIndex,
        errorSignalIndex,
        maxSignalIndex
    } MissionManagerSignalIndex_t;

    typedef enum {
        newMissionItemsAvailableSignalMask =    1 << newMissionItemsAvailableSignalIndex,
        inProgressChangedSignalMask =           1 << inProgressChangedSignalIndex,
        errorSignalMask =                       1 << errorSignalIndex,
    } MissionManagerSignalMask_t;

    MultiSignalSpy*     _multiSpyMissionManager;
    static const size_t _cMissionManagerSignals = maxSignalIndex;
    const char*         _rgMissionManagerSignals[_cMissionManagerSignals];

    static const int _missionManagerSignalWaitTime = MissionManager::_ackTimeoutMilliseconds * MissionManager::_maxRetryCount * 2;
};

#endif
