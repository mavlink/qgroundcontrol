#include "APMVehicleReadiness.h"

#include "APMFirmwarePlugin.h"
#include "Vehicle.h"
#include "VehicleSupports.h"

APMVehicleReadiness::APMVehicleReadiness(Vehicle* vehicle)
    : VehicleReadiness(vehicle)
{}

VehicleReadiness::ActionFlightModes APMVehicleReadiness::_actionFlightModes(GuidedAction action) const
{
    const auto* const firmwarePlugin = qobject_cast<const APMFirmwarePlugin*>(_vehicle->firmwarePlugin());
    if (!firmwarePlugin) {
        return ActionFlightModes{};
    }

    const QString guidedFlightMode = firmwarePlugin->guidedFlightMode();
    const QString missionFlightMode = firmwarePlugin->missionFlightMode();

    switch (action) {
        case GuidedAction::Takeoff: {
            // guidedModeTakeoff arms in Guided and takes off without a mode change; startTakeoff arms in Takeoff mode
            const QString takeoffFlightMode = _vehicle->supports()->guidedTakeoffWithAltitude()
                                                  ? guidedFlightMode
                                                  : firmwarePlugin->takeOffFlightMode();
            return ActionFlightModes{.armFlightMode = takeoffFlightMode, .targetFlightMode = takeoffFlightMode};
        }
        case GuidedAction::StartMission:
            if (_vehicle->fixedWing() || _vehicle->vtol()) {
                return ActionFlightModes{.armFlightMode = missionFlightMode, .targetFlightMode = missionFlightMode};
            }
            // MAV_CMD_MISSION_START switches to Auto after arming in Guided
            return ActionFlightModes{.armFlightMode = guidedFlightMode, .targetFlightMode = missionFlightMode};
    }

    return ActionFlightModes{};
}
