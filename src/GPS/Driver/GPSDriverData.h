#pragma once

#include "GPSObservation.h"
#include "GPSType.h"

struct sensor_gps_s;
struct satellite_info_s;
struct sensor_gnss_relative_s;

/// Internal adapter boundary; vendor types must not escape through the receiver API.
namespace GPSDriverData {
GPSObservation position(const sensor_gps_s& fix);
GPSSatelliteObservation satellites(const satellite_info_s& report, std::optional<GPSType> type = std::nullopt);
GPSRelativeObservation relativePosition(const sensor_gnss_relative_s& report);
void initialize(sensor_gps_s& fix);
}  // namespace GPSDriverData
