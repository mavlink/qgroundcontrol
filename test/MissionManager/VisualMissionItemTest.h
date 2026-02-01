#pragma once

#include "BaseClasses/MissionTest.h"

class MultiSignalSpy;
class VisualMissionItem;

/// Unit test for SimpleMissionItem
class VisualMissionItemTest : public OfflineMissionTest
{
    Q_OBJECT

protected:
    void init() override;

    void _createSpy(VisualMissionItem* visualItem, MultiSignalSpy** visualSpy);

    Vehicle* _controllerVehicle = nullptr;
};
