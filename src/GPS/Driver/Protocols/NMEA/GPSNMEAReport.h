#pragma once

#include <limits>

#include "GPSPositionReport.h"
#include "NMEASentence.h"

inline void applyNMEAGGA(GPSPositionReport& report, const NMEA::GGA& fix, uint64_t receivedAtUs)
{
    report.latitude_deg = fix.latitude;
    report.longitude_deg = fix.longitude;
    report.altitude_msl_m = fix.altitude;
    report.altitude_ellipsoid_m = fix.altitude + fix.geoidSeparation;
    report.hdop = fix.hdop;
    report.dop_timestamp = receivedAtUs;
    report.satellites_used = fix.satellitesUsed.value_or(std::numeric_limits<uint8_t>::max());
    report.fix_type = fix.quality == 1 ? 3 : fix.quality == 2 ? 4 : fix.quality == 4 ? 6 : fix.quality == 5 ? 5 : 0;
    report.timestamp = receivedAtUs;
    report.vel_ned_valid = false;
}
