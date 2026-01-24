#pragma once

#include "TestFixtures.h"

/// Unit test for PlanMasterController.
/// Uses OfflinePlanTest since tests work with offline PlanMasterController.
class PlanMasterControllerTest : public OfflinePlanTest
{
    Q_OBJECT

public:
    PlanMasterControllerTest() = default;

private slots:
    void cleanup() final;

    void _testMissionPlannerFileLoad();
    void _testActiveVehicleChanged();
};
