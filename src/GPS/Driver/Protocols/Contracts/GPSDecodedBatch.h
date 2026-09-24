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
struct GPSNativePositionReport
{
    GPSNavigationValues navigation{};
    bool velocityValid{};
};

struct GPSNativeSurveyReport
{
    uint64_t timestamp = 0;
    GPSSurveyReport survey{};
};

struct GPSNativeSatelliteReport
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

    // Full snapshots retire omitted constellations; deltas preserve them.
    bool fullSnapshot = true;
    uint8_t count = 0;
    std::array<Constellation, MAX_CONSTELLATIONS> constellations{};
};

struct GPSNativeSatelliteUsageReport
{
    uint64_t timestampUs = 0;
    std::optional<int> usedCount = std::nullopt;
};

struct GPSRTCMReport
{
    std::array<uint8_t, 1029> bytes{};
    size_t size = 0;
};

using GPSDecodedEvent = std::variant<GPSNativePositionReport, GPSIntegrityReport, GPSNativeSatelliteReport,
                                     GPSNativeSatelliteUsageReport, GPSNativeSurveyReport, GPSRTCMReport>;

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
