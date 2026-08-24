#pragma once

#include "BaseClasses/VehicleTest.h"

/// Verifies MockLink readiness failure injection is observable on the connected Vehicle:
/// SYS_STATUS sensor bits, GPS fix loss, battery-critical state and pre-arm denial.
class MockLinkReadinessInjectionTest : public VehicleTest
{
    Q_OBJECT

private slots:
    void _testSysStatusSensorBitsInjection();
    void _testGpsFixLostInjection();
    void _testBatteryCriticalInjection();
    void _testPreArmDeniedInjection();
};
