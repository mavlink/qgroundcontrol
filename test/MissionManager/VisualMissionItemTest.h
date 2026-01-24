#pragma once

#include "TestFixtures.h"

class PlanMasterController;
class MultiSignalSpy;
class VisualMissionItem;

/// Base class for VisualMissionItem unit tests.
/// Uses OfflineTest since these tests work with offline PlanMasterController.
class VisualMissionItemTest : public OfflineTest
{
    Q_OBJECT

public:
    VisualMissionItemTest() = default;

    void init() override;
    void cleanup() override;

protected:
    void _createSpy(VisualMissionItem* visualItem, MultiSignalSpy** visualSpy);

    PlanMasterController* _masterController = nullptr;
    Vehicle* _controllerVehicle = nullptr;
};
