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
#include "SimpleMissionItem.h"

#include <QGeoCoordinate>

class MissionControllerTest : public MissionControllerManagerTest
{
    Q_OBJECT
    
public:
    MissionControllerTest(void);

private slots:
    void cleanup(void);

    void _testEmptyVehicleAPM(void);
    void _testEmptyVehiclePX4(void);
    void _testAddWayppointAPM(void);
    void _testAddWayppointPX4(void);
    void _testOfflineToOnlineAPM(void);
    void _testOfflineToOnlinePX4(void);

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _testEmptyVehicleWorker(MAV_AUTOPILOT firmwareType);
    void _testAddWaypointWorker(MAV_AUTOPILOT firmwareType);
    void _testOfflineToOnlineWorker(MAV_AUTOPILOT firmwareType);
    void _setupMissionItemSignals(SimpleMissionItem* item);

    // MissiomItems signals

    enum {
        coordinateChangedSignalIndex = 0,
        missionItemMaxSignalIndex
    };

    enum {
        coordinateChangedSignalMask =           1 << coordinateChangedSignalIndex,
        missionItemMaxSignalMask =              1 << missionItemMaxSignalIndex,
    };

    // MissionController signals

    enum {
        visualItemsChangedSignalIndex = 0,
        waypointLinesChangedSignalIndex,
        missionControllerMaxSignalIndex
    };

    enum {
        visualItemsChangedSignalMask =                 1 << visualItemsChangedSignalIndex,
        waypointLinesChangedSignalMask =                1 << waypointLinesChangedSignalIndex,
    };

    MultiSignalSpy*     _multiSpyMissionController;
    static const size_t _cMissionControllerSignals = missionControllerMaxSignalIndex;
    const char*         _rgMissionControllerSignals[_cMissionControllerSignals];

    MultiSignalSpy*     _multiSpyMissionItem;
    static const size_t _cMissionItemSignals = missionItemMaxSignalIndex;
    const char*         _rgMissionItemSignals[_cMissionItemSignals];

    MissionController*  _missionController;
};

#endif
