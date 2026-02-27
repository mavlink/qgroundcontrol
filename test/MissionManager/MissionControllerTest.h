#pragma once

#include <memory>

#include "MissionControllerManagerTest.h"
#include "UnitTest.h"

class MissionController;
class PlanMasterController;
class VisualMissionItem;

class MissionControllerTest : public MissionControllerManagerTest
{
    Q_OBJECT

public:
    ~MissionControllerTest() override;

private slots:
    void cleanup() override;

    void _testLoadJsonSectionAvailable();
    void _testGlobalAltMode();
    void _testGimbalRecalc();
    void _testVehicleYawRecalc();

    // Parameterized tests - runs once per autopilot type
    UT_PARAMETERIZED_TEST(_testEmptyVehicle);

private:
    void _initForFirmwareType(MAV_AUTOPILOT firmwareType);
    void _setupVisualItemSignals(VisualMissionItem* visualItem);

    std::unique_ptr<PlanMasterController> _masterController;
    MissionController* _missionController = nullptr;
};
