#pragma once

#include "GPSExecutionContext.h"
#include "GPSObservation.h"
#include "GPSType.h"

struct GPSPositionReport;
struct GPSSatelliteReport;
struct GPSSatelliteUsageReport;
struct GPSRelativeReport;

/// Internal adapter boundary; vendor types must not escape through the receiver API.
namespace GPSDriverData {
GPSObservation position(const GPSPositionReport& fix, const GPSExecutionContext& context = {});
GPSSatelliteObservation satellites(const GPSSatelliteReport& report, const GPSExecutionContext& context = {});
GPSSatelliteObservation satellites(const GPSSatelliteUsageReport& report, const GPSExecutionContext& context = {});
GPSRelativeObservation relativePosition(const GPSRelativeReport& report, const GPSExecutionContext& context = {});
void initialize(GPSPositionReport& fix);
}  // namespace GPSDriverData
