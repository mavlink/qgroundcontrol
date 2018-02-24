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

AirMapVehicleManager::AirMapVehicleManager(AirMapSharedState& shared, const Vehicle& vehicle, QGCToolbox& toolbox)
    : AirspaceVehicleManager(vehicle)
    , _shared(shared)
    , _flightManager(shared)
    , _telemetry(shared)
    , _trafficMonitor(shared)
    , _toolbox(toolbox)
{
    connect(&_flightManager,  &AirMapFlightManager::flightPermitStatusChanged,  this, &AirMapVehicleManager::flightPermitStatusChanged);
    connect(&_flightManager,  &AirMapFlightManager::flightPermitStatusChanged,  this, &AirMapVehicleManager::_flightPermitStatusChanged);
    connect(&_flightManager,  &AirMapFlightManager::error,                      this, &AirMapVehicleManager::error);
    connect(&_telemetry,      &AirMapTelemetry::error,                          this, &AirMapVehicleManager::error);
    connect(&_trafficMonitor, &AirMapTrafficMonitor::error,                     this, &AirMapVehicleManager::error);
    connect(&_trafficMonitor, &AirMapTrafficMonitor::trafficUpdate,             this, &AirspaceVehicleManager::trafficUpdate);
}

void
AirMapVehicleManager::createFlight(const QList<MissionItem*>& missionItems)
{
    if (!_shared.client()) {
        qCDebug(AirMapManagerLog) << "No AirMap client instance. Will not create a flight";
        return;
    }
    //_flightManager.createFlight(missionItems);
}

AirspaceFlightPlanProvider::PermitStatus
AirMapVehicleManager::flightPermitStatus() const
{
    return _flightManager.flightPermitStatus();
}

void
AirMapVehicleManager::startTelemetryStream()
{
    if (_flightManager.flightID() != "") {
        _telemetry.startTelemetryStream(_flightManager.flightID());
    }
}

void
AirMapVehicleManager::stopTelemetryStream()
{
    _telemetry.stopTelemetryStream();
}

bool
AirMapVehicleManager::isTelemetryStreaming() const
{
    return _telemetry.isTelemetryStreaming();
}

void
AirMapVehicleManager::endFlight()
{
    _flightManager.endFlight();
    _trafficMonitor.stop();
}

void
AirMapVehicleManager::vehicleMavlinkMessageReceived(const mavlink_message_t& message)
{
    if (isTelemetryStreaming()) {
        _telemetry.vehicleMavlinkMessageReceived(message);
    }
}

void
AirMapVehicleManager::_flightPermitStatusChanged()
{
    // activate traffic alerts
    if (_flightManager.flightPermitStatus() == AirspaceFlightPlanProvider::PermitAccepted) {
        qCDebug(AirMapManagerLog) << "Subscribing to Traffic Alerts";
        // since we already created the flight, we know that we have a valid login token
        _trafficMonitor.startConnection(_flightManager.flightID());
    }
}

