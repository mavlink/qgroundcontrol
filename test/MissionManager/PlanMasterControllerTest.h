#pragma once

#include "UnitTest.h"

class PlanMasterController;

class PlanMasterControllerTest : public UnitTest
{
    Q_OBJECT

public:
    PlanMasterControllerTest(void);

private slots:
    void init(void) final;
    void cleanup(void) final;

    void _testMissionPlannerFileLoad(void);
    void _testActiveVehicleChanged(void);

private:
    PlanMasterController*   _masterController;
};
