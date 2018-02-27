/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "AirMapVehicleManager.h"
#include "AirMapManager.h"

#include "Vehicle.h"

//-----------------------------------------------------------------------------
AirMapVehicleManager::AirMapVehicleManager(AirMapSharedState& shared, const Vehicle& vehicle)
    : AirspaceVehicleManager(vehicle)
    , _shared(shared)
    , _flightManager(shared)
    , _telemetry(shared)
    , _trafficMonitor(shared)
{
    connect(&_flightManager,  &AirMapFlightManager::error,                      this, &AirMapVehicleManager::error);
    connect(&_telemetry,      &AirMapTelemetry::error,                          this, &AirMapVehicleManager::error);
    connect(&_trafficMonitor, &AirMapTrafficMonitor::error,                     this, &AirMapVehicleManager::error);
    connect(&_trafficMonitor, &AirMapTrafficMonitor::trafficUpdate,             this, &AirspaceVehicleManager::trafficUpdate);
}

//-----------------------------------------------------------------------------
void
AirMapVehicleManager::startTelemetryStream()
{
    if (!_flightManager.flightID().isEmpty()) {
        //-- TODO: This will start telemetry using the current flight ID in memory
        _telemetry.startTelemetryStream(_flightManager.flightID());
    }
}

//-----------------------------------------------------------------------------
void
AirMapVehicleManager::stopTelemetryStream()
{
    _telemetry.stopTelemetryStream();
}

//-----------------------------------------------------------------------------
bool
AirMapVehicleManager::isTelemetryStreaming()
{
    return _telemetry.isTelemetryStreaming();
}

//-----------------------------------------------------------------------------
void
AirMapVehicleManager::endFlight()
{
    if (!_flightManager.flightID().isEmpty()) {
        _flightManager.endFlight(_flightManager.flightID());
    }
    _trafficMonitor.stop();
}

//-----------------------------------------------------------------------------
void
AirMapVehicleManager::vehicleMavlinkMessageReceived(const mavlink_message_t& message)
{
    if (isTelemetryStreaming()) {
        _telemetry.vehicleMessageReceived(message);
    }
}
