#pragma once

#include "GPSObservation.h"
#include "GPSType.h"

struct GPSPositionReport;
struct GPSSatelliteReport;
struct GPSRelativeReport;

/// Internal adapter boundary; vendor types must not escape through the receiver API.
namespace GPSDriverData {
GPSObservation position(const GPSPositionReport& fix);
GPSSatelliteObservation satellites(const GPSSatelliteReport& report);
GPSRelativeObservation relativePosition(const GPSRelativeReport& report);
void initialize(GPSPositionReport& fix);
}  // namespace GPSDriverData
