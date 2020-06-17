/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "AirspaceManager.h"
#include "AirspaceVehicleManager.h"
#include "Vehicle.h"
#include "MissionItem.h"

AirspaceVehicleManager::AirspaceVehicleManager(const Vehicle& vehicle)
    : _vehicle(vehicle)
{
    qCDebug(AirspaceManagementLog) << "Instatiating AirspaceVehicleManager";
    connect(&_vehicle, &Vehicle::armedChanged,           this, &AirspaceVehicleManager::_vehicleArmedChanged);
    connect(&_vehicle, &Vehicle::mavlinkMessageReceived, this, &AirspaceVehicleManager::vehicleMavlinkMessageReceived);
}

void AirspaceVehicleManager::_vehicleArmedChanged(bool armed)
{
    if (armed) {
        qCDebug(AirspaceManagementLog) << "Starting telemetry";
        startTelemetryStream();
        _vehicleWasInMissionMode = _vehicle.flightMode() == _vehicle.missionFlightMode();
    } else {
        qCDebug(AirspaceManagementLog) << "Stopping telemetry";
        stopTelemetryStream();
        // end the flight if we were in mission mode during arming or disarming
        // TODO: needs to be improved. for instance if we do RTL and then want to continue the mission...
        if (_vehicleWasInMissionMode || _vehicle.flightMode() == _vehicle.missionFlightMode()) {
            endFlight();
        }
    }
}

