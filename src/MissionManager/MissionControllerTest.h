/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef MissionControllerTest_H
#define MissionControllerTest_H

#include "UnitTest.h"
#include "MockLink.h"
#include "MissionManager.h"
#include "MultiSignalSpy.h"
#include "MissionControllerManagerTest.h"
#include "PlanMasterController.h"
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

    void _testGimbalRecalc(void);
    void _testLoadJsonSectionAvailable(void);
    void _testEmptyVehicleAPM(void);
    void _testEmptyVehiclePX4(void);

private:
#if 0
    void _testOfflineToOnlineAPM(void);
    void _testOfflineToOnlinePX4(void);
#endif

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _testEmptyVehicleWorker(MAV_AUTOPILOT firmwareType);
    void _testAddWaypointWorker(MAV_AUTOPILOT firmwareType);
#if 0
    void _testOfflineToOnlineWorker(MAV_AUTOPILOT firmwareType);
#endif
    void _setupVisualItemSignals(VisualMissionItem* visualItem);

    // MissiomItems signals

    enum {
        coordinateChangedSignalIndex = 0,
        visualItemMaxSignalIndex
    };

    enum {
        coordinateChangedSignalMask =   1 << coordinateChangedSignalIndex,
        visualItemMaxSignalMask =       1 << visualItemMaxSignalIndex,
    };

    // MissionController signals

    enum {
        visualItemsChangedSignalIndex = 0,
        missionControllerMaxSignalIndex
    };

    enum {
        visualItemsChangedSignalMask =                 1 << visualItemsChangedSignalIndex,
    };

    MultiSignalSpy*     _multiSpyMissionController;
    static const size_t _cMissionControllerSignals = missionControllerMaxSignalIndex;
    const char*         _rgMissionControllerSignals[_cMissionControllerSignals];

    MultiSignalSpy*     _multiSpyMissionItem;
    static const size_t _cVisualItemSignals = visualItemMaxSignalIndex;
    const char*         _rgVisualItemSignals[_cVisualItemSignals];

    PlanMasterController*   _masterController;
    MissionController*      _missionController;
};

#endif
