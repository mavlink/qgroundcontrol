#include "CustomAction.h"
#include "Vehicle.h"

void CustomAction::sendTo(Vehicle* vehicle) {
    if (vehicle) {
        const bool showError = true;
        vehicle->sendMavCommand(_compId, _mavCmd, showError, _params[0], _params[1], _params[2], _params[3], _params[4], _params[5], _params[6]);
    }
};
