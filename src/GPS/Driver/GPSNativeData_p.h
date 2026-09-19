#pragma once

#include <map>
#include <vector>

#include "GPSDriverReports.h"
#include "GPSNativeIntegrityReport.h"
#include "GPSNativePositionReport.h"
#include "GPSNativeSatelliteReport.h"
#include "GPSNativeSurveyReport.h"

namespace GPSNativeData {
GPSPositionReport position(const GPSNativePositionReport& source, const GPSNativeIntegrityReport& diagnostic);
GPSSatelliteReport satellites(const GPSNativeSatelliteReport& source);
GPSSurveyReport survey(const GPSNativeSurveyReport& source);

/// Projection state only; native epoch assembly remains owned by the protocol.
class SatelliteSnapshot
{
public:
    GPSSatelliteReport update(const GPSNativeSatelliteReport& source);

private:
    std::map<GPSConstellation, std::vector<GPSSatelliteReport::Satellite>> _constellations;
};
}  // namespace GPSNativeData
