#pragma once

#include <QtPositioning/QGeoCoordinate>

#include "BaseClasses/VehicleTestManualConnect.h"
#include "MissionManager.h"

class MultiSignalSpy;

/// This is the base class for the MissionManager and MissionController unit tests.
class MissionControllerManagerTest : public VehicleTestManualConnect
{
    Q_OBJECT

protected slots:
    void cleanup() override;

protected:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _checkInProgressValues(bool inProgress);

    MissionManager* _missionManager;

    typedef struct
    {
        int sequenceNumber;
        QGeoCoordinate coordinate;
        MAV_CMD command;
        double param1;
        double param2;
        double param3;
        double param4;
        bool autocontinue;
        bool isCurrentItem;
        MAV_FRAME frame;
    } ItemInfo_t;

    typedef struct
    {
        const char* itemStream;
        const ItemInfo_t expectedItem;
    } TestCase_t;

    MultiSignalSpy* _multiSpyMissionManager;

    static const int _missionManagerSignalWaitTime =
        MissionManager::_ackTimeoutMilliseconds * MissionManager::_maxRetryCount * 2;
};
