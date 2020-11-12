/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "AirspaceManager.h"
#include "AirspaceVehicleManager.h"
#include "AirMapSharedState.h"
#include "AirMapFlightManager.h"
#include "AirMapTelemetry.h"
#include "AirMapTrafficMonitor.h"

/// AirMap per vehicle management class.

class AirMapVehicleManager : public AirspaceVehicleManager
{
    Q_OBJECT
public:
    AirMapVehicleManager        (AirMapSharedState& shared, const Vehicle& vehicle);
    ~AirMapVehicleManager       () override = default;

    void startTelemetryStream   () override;
    void stopTelemetryStream    () override;
    bool isTelemetryStreaming   () override;

signals:
    void error                  (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

public slots:
    void endFlight              () override;

protected slots:
    void vehicleMavlinkMessageReceived(const mavlink_message_t& message) override;

private slots:
    void _flightIDChanged       (QString flightID);

private:
    AirMapSharedState&           _shared;
    AirMapFlightManager          _flightManager;
    AirMapTelemetry              _telemetry;
    AirMapTrafficMonitor         _trafficMonitor;
};
