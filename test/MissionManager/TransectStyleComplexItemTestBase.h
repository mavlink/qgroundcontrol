#pragma once

#include "BaseClasses/MissionTest.h"

class PlanViewSettings;

/// Base class for all TransectStyleComplexItem unit tests
class TransectStyleComplexItemTestBase : public OfflineMissionTest
{
    Q_OBJECT

protected:
    void init() override;
    void cleanup() override;

    void _printItemCommands(QList<MissionItem*> items);

    Vehicle* _controllerVehicle = nullptr;
    PlanViewSettings* _planViewSettings = nullptr;
};
