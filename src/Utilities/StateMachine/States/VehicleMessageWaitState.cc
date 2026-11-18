#include "VehicleMessageWaitState.h"

#include "Vehicle.h"

void VehicleMessageWaitState::connectWaitSignal()
{
    Vehicle* const sourceVehicle = vehicleOrWarn();
    if (!sourceVehicle) {
        waitFailed();
        return;
    }

    connect(sourceVehicle, &Vehicle::mavlinkMessageReceived, this, &VehicleMessageWaitState::handleMessage,
            Qt::UniqueConnection);
}

void VehicleMessageWaitState::disconnectWaitSignal()
{
    if (Vehicle* const sourceVehicle = vehicle()) {
        disconnect(sourceVehicle, &Vehicle::mavlinkMessageReceived, this, &VehicleMessageWaitState::handleMessage);
    }
}
