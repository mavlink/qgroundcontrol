#pragma once

#include "VehicleConfigUITestBase.h"

/// UI tests for the PX4 Battery setup page with a PX4 MockLink vehicle connected
class PX4BatteryUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

private slots:
    void _lateParamShownAndFilterChosenOnceLoaded();
    void _filterTogglesUnusedBattery();
};
