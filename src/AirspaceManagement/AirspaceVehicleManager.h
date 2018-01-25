/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "AirspaceAuthorization.h"
#include "QGCMAVLink.h"

#include <QObject>
#include <QList>
#include <QGeoCoordinate>

class MissionItem;
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
     * create/upload a flight from a mission. This should update the flight permit status.
     * There can only be one active flight for each vehicle.
     */
    virtual void createFlight       (const QList<MissionItem*>& missionItems) = 0;

    /**
     * get the current flight permit status
     */
    virtual AirspaceAuthorization::PermitStatus flightPermitStatus() const = 0;

    /**
     * Setup the connection and start sending telemetry
     */
    virtual void startTelemetryStream   () = 0;
    virtual void stopTelemetryStream    () = 0;
    virtual bool isTelemetryStreaming   () const = 0;

public slots:
    virtual void endFlight              () = 0;

signals:
    void trafficUpdate                  (QString traffic_id, QString vehicle_id, QGeoCoordinate location, float heading);
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
