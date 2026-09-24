#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "GPSConstellation.h"

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
