#pragma once

#include "VehicleConfigUITestBase.h"

/// UI tests for the PX4 Ports setup page with a PX4 MockLink vehicle connected
class PX4SerialPortsUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

private slots:
    void _inUseFilterHidesUnusedPorts();
};
