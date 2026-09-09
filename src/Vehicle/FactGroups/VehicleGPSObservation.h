#pragma once

#include "GPSObservation.h"
#include "QGCMAVLink.h"

/// MAVLink satellite visibility and lock codes retain their vehicle telemetry meaning.
struct VehicleGPSObservation
{
    GPSObservation position;
    int satellitesVisible = -1;
    int fixType = 0;
    GPSObservation fusedPosition = {};

    static VehicleGPSObservation fromMessage(const mavlink_gps_raw_int_t& message);
    static VehicleGPSObservation fromMessage(const mavlink_gps2_raw_t& message);
    static VehicleGPSObservation fromMessage(const mavlink_high_latency_t& message);
    static VehicleGPSObservation fromMessage(const mavlink_high_latency2_t& message);
};
