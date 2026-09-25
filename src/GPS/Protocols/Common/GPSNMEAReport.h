#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

#include "GPSDecodedBatch.h"
#include "NMEANavigationEpoch.h"
#include "NMEASatelliteEpoch.h"
#include "NMEASentence.h"

inline void applyNMEAGGA(GPSDecodedPosition& report, const NMEA::GGA& fix, uint64_t receivedAtUs)
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

inline void applyNMEANavigationEpoch(GPSDecodedPosition& report, const NMEA::NavigationEpoch& epoch)
{
    report.navigation.latitudeDegrees = epoch.latitude;
    report.navigation.longitudeDegrees = epoch.longitude;
    report.navigation.altitudeMslMeters = epoch.altitudeMslMeters.value_or(std::numeric_limits<double>::quiet_NaN());
    report.navigation.altitudeEllipsoidMeters =
        epoch.altitudeEllipsoidMeters().value_or(std::numeric_limits<double>::quiet_NaN());
    report.navigation.horizontalDop = static_cast<float>(epoch.horizontalDop.value_or(NAN));
    report.navigation.verticalDop = static_cast<float>(epoch.verticalDop.value_or(NAN));
    report.navigation.horizontalAccuracyMeters = static_cast<float>(epoch.horizontalAccuracyMeters.value_or(NAN));
    report.navigation.verticalAccuracyMeters = static_cast<float>(epoch.verticalAccuracyMeters.value_or(NAN));
    report.navigation.satellitesUsed = gpsSatellitesUsed(epoch.satellitesUsed);
    report.navigation.fixType = epoch.fixQuality;
    report.navigation.timestampUs = epoch.positionReceivedAtUs;
    report.velocityValid = false;
}

inline GPSDecodedSatellites gpsNMEASatelliteReport(const NMEA::SatelliteSystem& system)
{
    GPSDecodedSatellites report;
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
