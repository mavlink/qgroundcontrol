#pragma once

#include "MissionControllerManagerTest.h"
#include "UnitTest.h"

class MissionController;
class MultiSignalSpy;
class PlanMasterController;
class VisualMissionItem;

class MissionControllerTest : public MissionControllerManagerTest
{
    Q_OBJECT

private slots:
    void cleanup();

    void _testLoadJsonSectionAvailable();
    void _testGlobalAltMode();
    void _testGimbalRecalc();
    void _testVehicleYawRecalc();

    // Parameterized tests - runs once per autopilot type
    UT_PARAMETERIZED_TEST(_testEmptyVehicle);

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _setupVisualItemSignals(VisualMissionItem* visualItem);

    MultiSignalSpy* _multiSpyMissionController = nullptr;
    MultiSignalSpy* _multiSpyMissionItem = nullptr;
    PlanMasterController* _masterController = nullptr;
    MissionController* _missionController = nullptr;
};
