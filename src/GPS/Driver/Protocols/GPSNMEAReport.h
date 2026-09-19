#pragma once

#include <limits>

#include "GPSNativePositionReport.h"
#include "NMEASentence.h"

inline void applyNMEAGGA(GPSNativePositionReport& report, const NMEA::GGA& fix, uint64_t receivedAtUs)
{
    report.latitude_deg = fix.latitude;
    report.longitude_deg = fix.longitude;
    report.altitude_msl_m = fix.altitude;
    report.altitude_ellipsoid_m = fix.altitude + fix.geoidSeparation;
    report.hdop = fix.hdop;
    report.dop_timestamp = receivedAtUs;
    report.satellites_used = fix.satellitesUsed.value_or(std::numeric_limits<uint8_t>::max());
    switch (fix.quality) {
        case NMEA::GgaQuality::INVALID:
            report.fix_type = GPSNativePositionReport::FIX_TYPE_NONE;
            break;
        case NMEA::GgaQuality::GPS:
            report.fix_type = GPSNativePositionReport::FIX_TYPE_3D;
            break;
        case NMEA::GgaQuality::DIFFERENTIAL:
            report.fix_type = GPSNativePositionReport::FIX_TYPE_RTCM_CODE_DIFFERENTIAL;
            break;
        case NMEA::GgaQuality::RTK_FIXED:
            report.fix_type = GPSNativePositionReport::FIX_TYPE_RTK_FIXED;
            break;
        case NMEA::GgaQuality::RTK_FLOAT:
            report.fix_type = GPSNativePositionReport::FIX_TYPE_RTK_FLOAT;
            break;
        case NMEA::GgaQuality::ESTIMATED:
            report.fix_type = GPSNativePositionReport::FIX_TYPE_EXTRAPOLATED;
            break;
        default:
            report.fix_type = GPSNativePositionReport::FIX_TYPE_UNKNOWN;
            break;
    }
    report.timestamp = receivedAtUs;
    report.vel_ned_valid = false;
}
