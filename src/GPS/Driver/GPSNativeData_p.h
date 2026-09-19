#pragma once

#include "GPSDriverReports.h"
#include "GPSNativeIntegrityReport.h"
#include "GPSNativePositionReport.h"
#include "GPSNativeSatelliteReport.h"
#include "GPSNativeSurveyReport.h"

namespace GPSNativeData {
GPSPositionReport position(const GPSNativePositionReport& source, const GPSNativeIntegrityReport& diagnostic);
GPSSatelliteReport satellites(const GPSNativeSatelliteReport& source);
GPSSurveyReport survey(const GPSNativeSurveyReport& source);
}  // namespace GPSNativeData
