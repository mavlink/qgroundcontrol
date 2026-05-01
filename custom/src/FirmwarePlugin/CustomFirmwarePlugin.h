#pragma once

#include "PX4FirmwarePlugin.h"

class AutoPilotPlugin;
class Vehicle;

class CustomFirmwarePlugin : public PX4FirmwarePlugin
{
    Q_OBJECT

public:
    CustomFirmwarePlugin();

    // FirmwarePlugin overrides

    AutoPilotPlugin *autopilotPlugin(Vehicle *vehicle) const final;
    const QVariantList &toolIndicators(const Vehicle *vehicle) final;
    /// Tells QGC that your vehicle has a gimbal on it. This will in turn cause thing like gimbal commands to point
    /// the camera straight down for surveys to be automatically added to Plans.
    bool hasGimbal(Vehicle *vehicle, bool &rollSupported, bool &pitchSupported, bool &yawSupported) const final;
    void updateAvailableFlightModes(FlightModeList &modeList) final;

private:
    QVariantList _toolIndicatorList;
};
