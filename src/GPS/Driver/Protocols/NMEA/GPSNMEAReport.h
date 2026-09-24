#pragma once

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
    report.navigation.satellitesUsed = fix.satellitesUsed.value_or(std::numeric_limits<uint8_t>::max());
    report.navigation.fixType = NMEA::fixQuality(fix.quality, GPSFixQuality::Fix3D);
    report.navigation.timestampUs = receivedAtUs;
    report.velocityValid = false;
}

inline GPSNativeSatelliteReport gpsNMEASatelliteReport(const NMEA::SatelliteSystem& system)
{
    GPSNativeSatelliteReport report;
    report.fullSnapshot = false;
    auto* constellation = report.ensureConstellation(system.constellation);
    if (!constellation) {
        return report;
    }
    constellation->inViewTimestampUs = system.inViewTimestampUs;
    constellation->inView = system.inView;
    if (system.usedIds) {
        constellation->inUseTimestampUs = system.inUseTimestampUs;
        constellation->inUse = static_cast<int>(system.usedIds->size());
    }
    return report;
}
