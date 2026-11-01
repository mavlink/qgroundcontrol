#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Feeds VFR_HUD and NAV_CONTROLLER_OUTPUT to a VehicleFactGroup for a PX4 or ArduPilot MockLink vehicle,
/// whose firmware type and version decide the sign of NAV_CONTROLLER_OUTPUT.alt_error.
class VehicleFactGroupTest : public VehicleTestManualConnect
{
    Q_OBJECT

public:
    explicit VehicleFactGroupTest(QObject* parent = nullptr)
        : VehicleTestManualConnect(parent)
    {}

private slots:
    void _altitudeTuningSetpoint_data();
    void _altitudeTuningSetpoint();
};
