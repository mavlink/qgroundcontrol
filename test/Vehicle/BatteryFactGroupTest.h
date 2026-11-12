#pragma once

#include "UnitTest.h"

class BatteryFactGroupTest : public UnitTest
{
    Q_OBJECT

private slots:
    void _batteryStatusVoltage_data();
    void _batteryStatusVoltage();
};
