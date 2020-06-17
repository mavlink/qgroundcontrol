/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "AirspaceFlightPlanProvider.h"
#include "QGCMAVLink.h"

#include <QObject>
#include <QList>
#include <QGeoCoordinate>

class Vehicle;

//-----------------------------------------------------------------------------
/**
 * @class AirspaceVehicleManager
 * Base class for per-vehicle management (each vehicle has one (or zero) of these)
 */

class AirspaceVehicleManager : public QObject {
    Q_OBJECT
public:
    AirspaceVehicleManager           (const Vehicle& vehicle);
    virtual ~AirspaceVehicleManager  () = default;

    /**
     * Setup the connection and start sending telemetry
     */
    virtual void startTelemetryStream   () = 0;
    virtual void stopTelemetryStream    () = 0;
    virtual bool isTelemetryStreaming   () = 0;

public slots:
    virtual void endFlight              () = 0;

signals:
    void trafficUpdate                  (bool alert, QString traffic_id, QString vehicle_id, QGeoCoordinate location, float heading);
    void flightPermitStatusChanged      ();

protected slots:
    virtual void vehicleMavlinkMessageReceived(const mavlink_message_t& message) = 0;

protected:
    const Vehicle& _vehicle;

private slots:
    void _vehicleArmedChanged           (bool armed);

private:
    bool _vehicleWasInMissionMode = false; ///< true if the vehicle was in mission mode when arming
};
