#pragma once

#include "GPSExecutionContext.h"
#include "GPSIntegrityObservation.h"
#include "GPSObservation.h"
#include "GPSSurveyInStatus.h"
#include "GPSType.h"

struct GPSPositionReport;
struct GPSIntegrityReport;
struct GPSSatelliteReport;
struct GPSSatelliteUsageReport;
struct GPSRelativeReport;
struct GPSSurveyReport;

/// Internal adapter boundary; vendor types must not escape through the receiver API.
namespace GPSDriverData {
GPSIntegrityObservation integrity(const GPSIntegrityReport& report, const GPSExecutionContext& context = {});
GPSObservation position(const GPSPositionReport& fix, const GPSExecutionContext& context = {});
GPSSatelliteObservation satellites(const GPSSatelliteReport& report, const GPSExecutionContext& context = {});
GPSSatelliteObservation satellites(const GPSSatelliteUsageReport& report, const GPSExecutionContext& context = {});
GPSRelativeObservation relativePosition(const GPSRelativeReport& report, const GPSExecutionContext& context = {});
GPSSurveyInStatus survey(const GPSSurveyReport& report, const GPSExecutionContext& context = {});
}  // namespace GPSDriverData
