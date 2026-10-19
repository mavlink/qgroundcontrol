#include "PX4VehicleReadiness.h"

#include "FirmwarePlugin.h"
#include "Vehicle.h"

PX4VehicleReadiness::PX4VehicleReadiness(Vehicle* vehicle)
    : VehicleReadiness(vehicle)
{}

VehicleReadiness::ActionFlightModes PX4VehicleReadiness::_actionFlightModes(GuidedAction action) const
{
    QString flightMode;
    switch (action) {
        case GuidedAction::Takeoff:
            // guidedModeTakeoff and startTakeoff both switch to Takeoff mode, then arm
            flightMode = _vehicle->firmwarePlugin()->takeOffFlightMode();
            break;
        case GuidedAction::StartMission:
            // startMission switches to Mission mode, then arms
            flightMode = _vehicle->firmwarePlugin()->missionFlightMode();
            break;
    }
    return ActionFlightModes{.armFlightMode = flightMode, .targetFlightMode = flightMode};
}
