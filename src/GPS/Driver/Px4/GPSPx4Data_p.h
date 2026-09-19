#pragma once

#include "GPSDriverReports.h"
#include "GPSType.h"

struct sensor_gps_s;
struct satellite_info_s;

/// Private compatibility boundary for the current PX4 driver.
namespace GPSPx4Data {
void initialize(sensor_gps_s& position);
GPSPositionReport position(const sensor_gps_s& position);
GPSSatelliteReport satellites(const satellite_info_s& satellites, GPSType type);
}  // namespace GPSPx4Data
