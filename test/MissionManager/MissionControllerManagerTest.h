#pragma once

#include "TestFixtures.h"
#include "MissionManager.h"
#include "PlanManager.h"

#include <QtPositioning/QGeoCoordinate>

class MultiSignalSpy;

/// This is the base class for the MissionManager and MissionController unit tests.
/// Uses UnitTest directly since it requires custom firmware-type initialization.
class MissionControllerManagerTest : public UnitTest
{
    Q_OBJECT

public:
    MissionControllerManagerTest() = default;

protected slots:
    void cleanup() override;

protected:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _checkInProgressValues(bool inProgress);

    MissionManager* _missionManager = nullptr;

    struct ItemInfo_t {
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
    };

    struct TestCase_t {
        const char* itemStream;
        const ItemInfo_t expectedItem;
    };

    MultiSignalSpy* _multiSpyMissionManager = nullptr;

    // Full timeout for failure cases that need to wait for all retries
    // In unit tests, uses the much shorter kTestAckTimeoutMs (50ms) instead of production _ackTimeoutMilliseconds (1500ms)
    static constexpr int _missionManagerSignalWaitTime = PlanManager::kTestAckTimeoutMs * PlanManager::_maxRetryCount * 2;

    // Shorter timeout for success cases where MockLink responds quickly
    static constexpr int _missionManagerSuccessWaitTime = 1000;
};
