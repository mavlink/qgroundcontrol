#pragma once

#include "UnitTest.h"

class PlanViewSettings;
class PlanMasterController;
class Vehicle;

/// Base class for all TransectStyleComplexItem unit tests
class TransectStyleComplexItemTestBase : public UnitTest
{
    Q_OBJECT

public:
    TransectStyleComplexItemTestBase(void);

protected:
    void init   (void) override;
    void cleanup(void) override;

    void _printItemCommands(QList<MissionItem*> items);

    PlanMasterController*   _masterController =     nullptr;
    Vehicle*                _controllerVehicle =    nullptr;
    PlanViewSettings*       _planViewSettings =     nullptr;
};
