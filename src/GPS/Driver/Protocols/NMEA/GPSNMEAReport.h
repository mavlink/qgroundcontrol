#pragma once

#include <algorithm>
#include <iterator>
#include <limits>

#include "GPSNativePositionReport.h"
#include "GPSNativeSatelliteReport.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentence.h"

inline void applyNMEAGGA(GPSNativePositionReport& report, const NMEA::GGA& fix, uint64_t receivedAtUs)
{
    report.navigation.latitudeDegrees = fix.latitude;
    report.navigation.longitudeDegrees = fix.longitude;
    report.navigation.altitudeMslMeters = fix.altitude;
    report.navigation.altitudeEllipsoidMeters = fix.altitude + fix.geoidSeparation;
    report.navigation.horizontalDop = fix.hdop;
    report.dop_timestamp = receivedAtUs;
    report.navigation.satellitesUsed = fix.satellitesUsed.value_or(std::numeric_limits<uint8_t>::max());
    switch (fix.quality) {
        case NMEA::GgaQuality::INVALID:
            report.navigation.fixType = GPSPositionReport::FixType::NoFix;
            break;
        case NMEA::GgaQuality::GPS:
            report.navigation.fixType = GPSPositionReport::FixType::Fix3D;
            break;
        case NMEA::GgaQuality::DIFFERENTIAL:
            report.navigation.fixType = GPSPositionReport::FixType::Differential;
            break;
        case NMEA::GgaQuality::RTK_FIXED:
            report.navigation.fixType = GPSPositionReport::FixType::RTKFixed;
            break;
        case NMEA::GgaQuality::RTK_FLOAT:
            report.navigation.fixType = GPSPositionReport::FixType::RTKFloat;
            break;
        case NMEA::GgaQuality::ESTIMATED:
            report.navigation.fixType = GPSPositionReport::FixType::Extrapolated;
            break;
        default:
            report.navigation.fixType = GPSPositionReport::FixType::Unknown;
            break;
    }
    report.navigation.timestampUs = receivedAtUs;
    report.vel_ned_valid = false;
}

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
