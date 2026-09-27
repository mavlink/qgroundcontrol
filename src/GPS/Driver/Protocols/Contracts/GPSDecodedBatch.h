#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "GPSConstellation.h"
#include "GPSDriverReports.h"

/// Decoded navigation plus whether the producer accepted its velocity solution.
struct GPSDecodedPosition
{
    GPSNavigationValues navigation{};
    bool velocityValid{};
};

struct GPSDecodedSurvey
{
    uint64_t timestamp = 0;
    GPSSurveyReport survey{};
};

struct GPSDecodedSatellites
{
    static constexpr uint16_t SAT_INFO_MAX_SATELLITES = 128;
    static constexpr uint8_t MAX_CONSTELLATIONS = 8;

    struct Constellation
    {
        GPSConstellation constellation = GPSConstellation::Unknown;
        uint64_t inViewTimestampUs = 0;
        int inView = 0;
        uint64_t inUseTimestampUs = 0;
        std::optional<int> inUse = std::nullopt;
    };

    Constellation* ensureConstellation(GPSConstellation constellation)
    {
        for (uint8_t index = 0; index < count; ++index) {
            if (constellations[index].constellation == constellation) {
                return &constellations[index];
            }
        }
        if (count >= constellations.size()) {
            return nullptr;
        }
        auto& report = constellations[count++];
        report.constellation = constellation;
        return &report;
    }

    /// Counts one satellite in view of @a constellation, and in use when @a used. A full report drops it.
    void addSatellite(GPSConstellation constellation, bool used)
    {
        if (auto* system = ensureConstellation(constellation)) {
            ++system->inView;
            system->inUse = system->inUse.value_or(0) + (used ? 1 : 0);
        }
    }

    // Full snapshots retire omitted constellations; deltas preserve them.
    bool fullSnapshot = true;
    uint8_t count = 0;
    std::array<Constellation, MAX_CONSTELLATIONS> constellations{};
};

struct GPSDecodedSatelliteUsage
{
    uint64_t timestampUs = 0;
    std::optional<int> usedCount = std::nullopt;
};

/// A reported used-satellite count; receivers report UINT8_MAX and above when the count is unknown.
inline std::optional<uint8_t> gpsSatellitesUsed(std::optional<unsigned> count)
{
    return count && *count < UINT8_MAX ? std::optional<uint8_t>(static_cast<uint8_t>(*count)) : std::nullopt;
}

struct GPSRTCMReport
{
    std::array<uint8_t, 1029> bytes{};
    size_t size = 0;
};

using GPSDecodedEvent = std::variant<GPSDecodedPosition, GPSIntegrityReport, GPSDecodedSatellites,
                                     GPSDecodedSatelliteUsage, GPSDecodedSurvey, GPSRTCMReport>;

/// A bounded, owned sequence. The caller decides whether position epochs may be coalesced.
struct GPSDecodedBatch
{
    static constexpr size_t MAX_EVENTS = 8;
    static constexpr int POSITION_UPDATE = 1;
    static constexpr int SATELLITES_UPDATE = 2;
    static constexpr int PROTOCOL_ACTIVITY = 4;
    std::vector<GPSDecodedEvent> events;
    int updates = 0;
};

struct GPSDecodeResult
{
    size_t bytesConsumed = 0;
    GPSDecodedBatch batch;
};
