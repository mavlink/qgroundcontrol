#pragma once

#include "GPSDecodedBatch.h"
#include "GPSDriverReports.h"
#include "GPSSatelliteState.h"

namespace GPSNativeData {
GPSPositionReport position(const GPSNativePositionReport& source, const GPSIntegrityReport& diagnostic,
                           uint64_t nowUs = 0);

/// Projection state only; native epoch assembly remains owned by the protocol.
class SatelliteSnapshot
{
public:
    GPSSatelliteReport update(const GPSNativeSatelliteReport& source, uint64_t nowUs = 0);
    GPSSatelliteReport update(const GPSNativeSatelliteUsageReport& source, uint64_t nowUs = 0);
    /// Poll on receive turns, including turns with position/correction traffic but no satellite report.
    std::optional<GPSSatelliteReport> expire(uint64_t nowUs);

private:
    enum class UsageSelection
    {
        ViewThenCountOnly,
        CountOnly
    };

    struct SatelliteProjection
    {
        GPSSatelliteReport report;
        std::optional<int> viewUsed = std::nullopt;
        uint64_t viewUsageReceiptUs = 0;
    };

    SatelliteProjection _project(uint64_t nowUs);
    GPSSatelliteReport _snapshot(uint64_t nowUs, UsageSelection usageSelection);
    GPSSatelliteState _state;
    std::optional<int> _countOnlyUsage = std::nullopt;
    uint64_t _countOnlyUsageReceiptUs = 0;
    uint64_t _latestReceiptUs = 0;
    bool _countOnlyUsageIsLatest = false;
};
}  // namespace GPSNativeData
