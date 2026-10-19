#pragma once

#include "VehicleReadiness.h"

/// \brief PX4 guided action readiness.
///
/// PX4 sends libevents health reports. Its takeoff and mission actions switch to the takeoff or mission mode
/// before arming.
class PX4VehicleReadiness : public VehicleReadiness
{
    Q_OBJECT

public:
    explicit PX4VehicleReadiness(Vehicle* vehicle);

protected:
    ActionFlightModes _actionFlightModes(GuidedAction action) const override;

    bool _expectsHealthAndArmingChecks() const override { return true; }
};
