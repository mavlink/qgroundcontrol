#pragma once

#include "UnitTest.h"
#include "PlanManager.h"

#include <QtPositioning/QGeoCoordinate>

class MultiSignalSpy;

class PlanManagerTest : public UnitTest
{
    Q_OBJECT

public:
    PlanManagerTest(void);

private slots:
    void cleanup(void);

    void _testInitialState();
    void _testInProgressTracking();
    void _testLoadFromVehicle();
    void _testLoadFromVehicleCancel();
    void _testWriteMissionItems();
    void _testWriteMissionItemsEmpty();
    void _testRemoveAll();
    void _testRemoveAllNoItems();
    void _testConcurrentOperationsPrevented();
    void _testTimeoutRetry();
    void _testMaxRetryExceeded();
    void _testMissionTypeMismatch();

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _setupSignalSpy();
    void _checkInProgressValues(bool inProgress);

    PlanManager* _planManager = nullptr;

    typedef enum {
        newMissionItemsAvailableSignalIndex = 0,
        inProgressChangedSignalIndex,
        errorSignalIndex,
        removeAllCompleteSignalIndex,
        sendCompleteSignalIndex,
        maxSignalIndex
    } PlanManagerSignalIndex_t;

    typedef enum {
        newMissionItemsAvailableSignalMask = 1 << newMissionItemsAvailableSignalIndex,
        inProgressChangedSignalMask = 1 << inProgressChangedSignalIndex,
        errorSignalMask = 1 << errorSignalIndex,
        removeAllCompleteSignalMask = 1 << removeAllCompleteSignalIndex,
        sendCompleteSignalMask = 1 << sendCompleteSignalIndex,
    } PlanManagerSignalMask_t;

    MultiSignalSpy* _multiSpyPlanManager = nullptr;
    static const size_t _cPlanManagerSignals = maxSignalIndex;
    const char* _rgPlanManagerSignals[_cPlanManagerSignals];

    static const int _planManagerSignalWaitTime = PlanManager::_ackTimeoutMilliseconds * PlanManager::_maxRetryCount * 2;
};
