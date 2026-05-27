#pragma once

#include "QmlUITestBase.h"

class MockLink;

/// UI smoke test that boots the full QML UI with a PX4 MockLink vehicle
/// connected and navigates through all vehicle configuration pages.
class VehicleConfigUITest : public QmlUITestBase
{
    Q_OBJECT

public:
    VehicleConfigUITest() = default;

private slots:
    void _testNavigateVehicleConfig();
};
