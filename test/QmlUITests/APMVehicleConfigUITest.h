#pragma once

#include "VehicleConfigUITestBase.h"

#include <functional>

class MockLink;

/// UI smoke test that boots the full QML UI with each ArduPilot MockLink
/// vehicle type connected and navigates through all vehicle configuration pages.
class APMVehicleConfigUITest : public VehicleConfigUITestBase
{
    Q_OBJECT

public:
    APMVehicleConfigUITest() = default;

private slots:
    void _testArduCopter();
    void _testArduPlane();
    void _testArduSub();
    void _testArduRover();

private:
    /// Shared implementation: connect a MockLink via \a factory, navigate to
    /// the Configure view, and click through every vehicle component.
    /// \a vehicleName is used only in QVERIFY2 failure messages.
    void _runNavigateVehicleConfig(const std::function<MockLink *()> &factory,
                                   const QString &vehicleName);
};
