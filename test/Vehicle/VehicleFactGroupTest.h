#pragma once

#include "BaseClasses/VehicleTest.h"

/// Feeds VFR_HUD and NAV_CONTROLLER_OUTPUT to a VehicleFactGroup for a PX4 MockLink vehicle,
/// whose firmware version decides the sign of NAV_CONTROLLER_OUTPUT.alt_error.
class VehicleFactGroupTest : public VehicleTest
{
    Q_OBJECT

public:
    explicit VehicleFactGroupTest(QObject* parent = nullptr)
        : VehicleTest(parent)
    {}

private slots:
    void _altitudeTuningSetpoint_data();
    void _altitudeTuningSetpoint();
};
