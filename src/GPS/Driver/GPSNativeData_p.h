#pragma once

#include "../Core/GPSSatelliteState.h"
#include "GPSDriverReports.h"
#include "GPSNativeIntegrityReport.h"
#include "GPSNativePositionReport.h"
#include "GPSNativeSatelliteReport.h"
#include "GPSNativeSurveyReport.h"

namespace GPSNativeData {
GPSPositionReport position(const GPSNativePositionReport& source, const GPSNativeIntegrityReport& diagnostic,
                           uint64_t nowUs = 0);
GPSSurveyReport survey(const GPSNativeSurveyReport& source);

/// Projection state only; native epoch assembly remains owned by the protocol.
class SatelliteSnapshot
{
public:
    GPSSatelliteReport update(const GPSNativeSatelliteReport& source, uint64_t nowUs = 0);
    /// Poll on receive turns, including turns with position/correction traffic but no satellite report.
    std::optional<GPSSatelliteReport> expire(uint64_t nowUs);

private:
    GPSSatelliteReport _snapshot(uint64_t nowUs);
    GPSSatelliteState _state;
    uint64_t _latestReceiptUs = 0;
};
}  // namespace GPSNativeData
