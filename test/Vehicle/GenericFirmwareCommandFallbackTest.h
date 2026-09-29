#pragma once

#include "BaseClasses/VehicleTestManualConnect.h"

/// Regression tests for MavCommandQueue's lambda-fallback path on a MAV_AUTOPILOT_GENERIC vehicle.
///
/// The generic firmware plugin creates no FirmwarePluginInstanceData, so the command-support
/// cache is unavailable and every send must probe the command, falling back when it is NAKed.
class GenericFirmwareCommandFallbackTest : public VehicleTestManualConnect
{
    Q_OBJECT

public:
    explicit GenericFirmwareCommandFallbackTest(QObject* parent = nullptr)
        : VehicleTestManualConnect(parent)
    {}

private slots:
    void _setCurrentMissionSequence_probesAndFallsBack();
};
