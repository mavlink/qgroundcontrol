#pragma once

#include "QGCMAVLink.h"

struct MissionFlightStatus_t {
    double                      maxTelemetryDistance;
    double                      totalDistance;
    double                      plannedDistance;
    double                      totalTime;
    double                      hoverDistance;
    double                      hoverTime;
    double                      cruiseDistance;
    double                      cruiseTime;
    int                         mAhBattery;             ///< 0 for not available
    double                      hoverAmps;              ///< Amp consumption during hover
    double                      cruiseAmps;             ///< Amp consumption during cruise
    double                      ampMinutesAvailable;    ///< Amp minutes available from single battery
    double                      hoverAmpsTotal;         ///< Total hover amps used
    double                      cruiseAmpsTotal;        ///< Total cruise amps used
    int                         batteryChangePoint;     ///< -1 for not supported, 0 for not needed
    int                         batteriesRequired;      ///< -1 for not supported
    double                      vehicleYaw;
    double                      gimbalYaw;              ///< NaN signals yaw was never changed
    double                      gimbalPitch;            ///< NaN signals pitch was never changed
    // The following values are the state prior to executing this item
    QGCMAVLink::VehicleClass_t  vtolMode;               ///< Either VehicleClassFixedWing, VehicleClassMultiRotor, VehicleClassGeneric (mode unknown)
    double                      cruiseSpeed;
    double                      hoverSpeed;
    double                      vehicleSpeed;           ///< Either cruise or hover speed based on vehicle type and vtol state
};
