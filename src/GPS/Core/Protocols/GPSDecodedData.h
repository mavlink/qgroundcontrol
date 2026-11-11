#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <optional>

#include "GPSProtocolEvent.h"
#include "GPSReceiverReports.h"

/// How GPSDriver adapts decoded events to public reports.
namespace GPSDecodedData {
GPSPositionReport position(const GPSDecodedPosition& source, const GPSIntegrityReport& diagnostic, uint64_t nowUs = 0);

/// The satellite counts GPSDriver reports. Each constellation keeps its latest in-view and used counts, each with its
/// own receipt, until FRESHNESS_TIMEOUT has passed. A full snapshot replaces every constellation; a delta replaces only
/// the counts it carries, and never with an older receipt. The used count sums the constellations in view, and is zero
/// when none has satellites in view. Only while that sum is unknown, before any view is known or while a constellation
/// with satellites in view lacks a used count, does the latest count-only report stand in, under the same timeout from
/// its own receipt.
class SatelliteCounts
{
public:
    static constexpr std::chrono::milliseconds FRESHNESS_TIMEOUT{5000};

    GPSSatelliteReport update(const GPSDecodedSatellites& source, uint64_t nowUs);
    /// Records a count-only used count. @return the report, whose used count is this one only while the constellations'
    /// sum is unknown.
    GPSSatelliteReport update(const GPSDecodedSatelliteUsage& source, uint64_t nowUs);
    /// Drops the counts that expired by @a nowUs. Poll on every receive, including those without satellite reports.
    /// @return the report when a count expired.
    std::optional<GPSSatelliteReport> expire(uint64_t nowUs);

private:
    struct Count
    {
        /// Zero when there is no fresh count.
        uint64_t receivedAtUs = 0;
        std::optional<int> value = std::nullopt;
    };

    struct Constellation
    {
        Count inView;
        Count used;
    };

    static void _accept(Count& count, uint64_t receivedAtUs, std::optional<int> value, uint64_t nowUs);
    bool _expire(uint64_t nowUs);
    GPSSatelliteReport _report() const;

    std::map<GPSConstellation, Constellation> _constellations;
    Count _countOnlyUsed;
};
}  // namespace GPSDecodedData
