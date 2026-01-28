#pragma once

#include "MissionControllerManagerTest.h"

class MissionController;
class MultiSignalSpy;
class PlanMasterController;
class VisualMissionItem;

class MissionControllerTest : public MissionControllerManagerTest
{
    Q_OBJECT

public:
    MissionControllerTest() = default;

private slots:
    void cleanup() override;

    void _testLoadJsonSectionAvailable();
    void _testEmptyVehicleAPM();
    void _testEmptyVehiclePX4();
    void _testGlobalAltMode();
    void _testGimbalRecalc();
    void _testVehicleYawRecalc();
    void _testOfflineToOnlineAPM();
    void _testOfflineToOnlinePX4();

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _testEmptyVehicleWorker(MAV_AUTOPILOT firmwareType);
    void _testAddWaypointWorker(MAV_AUTOPILOT firmwareType);
    void _testOfflineToOnlineWorker(MAV_AUTOPILOT firmwareType);
    void _setupVisualItemSignals(VisualMissionItem* visualItem);

    MultiSignalSpy* _multiSpyMissionController = nullptr;
    MultiSignalSpy* _multiSpyMissionItem = nullptr;
    PlanMasterController* _masterController = nullptr;
    MissionController* _missionController = nullptr;
};
