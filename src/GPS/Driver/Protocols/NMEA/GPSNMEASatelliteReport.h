#pragma once

#include <algorithm>
#include <iterator>

#include "GPSNativeSatelliteReport.h"
#include "NMEASatelliteEpoch.h"

inline GPSNativeSatelliteReport gpsNMEASatelliteReport(const NMEA::SatelliteSystem& system)
{
    GPSNativeSatelliteReport report;
    report.timestamp = system.inViewTimestampUs;
    report.constellation = system.constellation;
    report.count = static_cast<uint16_t>(std::min(system.satellites.size(), report.entries.size()));
    for (size_t index = 0; index < report.count; ++index) {
        const auto& source = system.satellites[index];
        report.entries[index] = {source.id,        source.prn,     source.constellation, std::nullopt,
                                 source.elevation, source.azimuth, source.signal};
    }
    if (system.usedIds) {
        report.usage = GPSNativeSatelliteReport::Usage{};
        report.usage->timestamp = system.inUseTimestampUs;
        report.usage->count = static_cast<uint16_t>(std::min(system.usedIds->size(), report.usage->ids.size()));
        std::copy_n(system.usedIds->begin(), report.usage->count, report.usage->ids.begin());
        for (size_t index = 0; index < report.count; ++index) {
            report.entries[index].used = system.usedIds->contains(report.entries[index].id);
        }
    }
    return report;
}
