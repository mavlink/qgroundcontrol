#pragma once

#include "BaseClasses/VehicleTest.h"

class PlanMasterController;

class PlanMasterControllerTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void init() final;
    void cleanup() final;

    void _testMissionPlannerFileLoad();
    void _testActiveVehicleChanged();

private:
    PlanMasterController* _masterController = nullptr;
};
