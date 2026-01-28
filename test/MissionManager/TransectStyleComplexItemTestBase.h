#pragma once

#include "TestFixtures.h"

class PlanViewSettings;
class PlanMasterController;
class Vehicle;

/// Base class for all TransectStyleComplexItem unit tests.
/// Uses OfflineTest since these tests work with offline PlanMasterController.
class TransectStyleComplexItemTestBase : public OfflineTest
{
    Q_OBJECT

public:
    TransectStyleComplexItemTestBase() = default;

protected:
    void init() override;
    void cleanup() override;

    void _printItemCommands(QList<MissionItem*> items);

    PlanMasterController* _masterController = nullptr;
    Vehicle* _controllerVehicle = nullptr;
    PlanViewSettings* _planViewSettings = nullptr;
};
