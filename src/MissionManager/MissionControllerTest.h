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

#ifndef MissionControllerTest_H
#define MissionControllerTest_H

#include "UnitTest.h"
#include "MockLink.h"
#include "MissionManager.h"
#include "MultiSignalSpy.h"
#include "MissionControllerManagerTest.h"
#include "MissionController.h"

#include <QGeoCoordinate>

class MissionControllerTest : public MissionControllerManagerTest
{
    Q_OBJECT
    
public:
    MissionControllerTest(void);

private slots:
    void cleanup(void);

    void _testOfflineToOnlineAPM(void);
    void _testOfflineToOnlinePX4(void);

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _testEmptyVehicleWorker(MAV_AUTOPILOT firmwareType);
    void _testAddWaypointWorker(MAV_AUTOPILOT firmwareType);
    void _testOfflineToOnlineWorker(MAV_AUTOPILOT firmwareType);

    void _testEmptyVehicleAPM(void);
    void _testEmptyVehiclePX4(void);
    void _testAddWayppointAPM(void);
    void _testAddWayppointPX4(void);

    enum {
        missionItemsChangedSignalIndex = 0,
        waypointLinesChangedSignalIndex,
        liveHomePositionAvailableChangedSignalIndex,
        liveHomePositionChangedSignalIndex,
        maxSignalIndex
    };

    enum {
        missionItemsChangedSignalMask =                 1 << missionItemsChangedSignalIndex,
        waypointLinesChangedSignalMask =                1 <<  waypointLinesChangedSignalIndex,
        liveHomePositionAvailableChangedSignalMask =    1 << liveHomePositionAvailableChangedSignalIndex,
        liveHomePositionChangedSignalMask =             1 << liveHomePositionChangedSignalIndex,
    };

    MultiSignalSpy*     _multiSpy;
    static const size_t _cSignals = maxSignalIndex;
    const char*         _rgSignals[_cSignals];

    MissionController*  _missionController;
};

#endif
