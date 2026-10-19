#pragma once

#include "VehicleReadiness.h"

/// \brief ArduPilot guided action readiness.
///
/// Mirrors how APMFirmwarePlugin performs the guided actions: copters and VTOLs take off from Guided, and vehicles
/// other than planes and VTOLs arm in Guided before switching to Auto to start a mission.
class APMVehicleReadiness : public VehicleReadiness
{
    Q_OBJECT

public:
    explicit APMVehicleReadiness(Vehicle* vehicle);

protected:
    ActionFlightModes _actionFlightModes(GuidedAction action) const override;
};
