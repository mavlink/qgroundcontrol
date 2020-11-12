/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
        sendCompleteSignalIndex,
        inProgressChangedSignalIndex,
        errorSignalIndex,
        maxSignalIndex
    } MissionManagerSignalIndex_t;

    typedef enum {
        newMissionItemsAvailableSignalMask =    1 << newMissionItemsAvailableSignalIndex,
        sendCompleteSignalMask =                1 << sendCompleteSignalIndex,
        inProgressChangedSignalMask =           1 << inProgressChangedSignalIndex,
        errorSignalMask =                       1 << errorSignalIndex,
    } MissionManagerSignalMask_t;

    MultiSignalSpy*     _multiSpyMissionManager;
    static const size_t _cMissionManagerSignals = maxSignalIndex;
    const char*         _rgMissionManagerSignals[_cMissionManagerSignals];

    static const int _missionManagerSignalWaitTime = MissionManager::_ackTimeoutMilliseconds * MissionManager::_maxRetryCount * 2;
};

#endif
