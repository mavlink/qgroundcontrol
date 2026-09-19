#include <algorithm>

#include "GPSNativeData_p.h"

namespace GPSNativeData {
GPSPositionReport position(const GPSNativePositionReport& source, const GPSNativeIntegrityReport& diagnostic)
{
    GPSPositionReport result;
    result.timestampUs = source.timestamp;
    result.utcTimeUs = source.time_utc_usec;
    result.fixType = GPSPositionReport::fixTypeFromValue(static_cast<int>(source.fix_type));
    result.latitudeDegrees = source.latitude_deg;
    result.longitudeDegrees = source.longitude_deg;
    result.altitudeMslMeters = source.altitude_msl_m;
    result.altitudeEllipsoidMeters = source.altitude_ellipsoid_m;
    result.horizontalAccuracyMeters = source.eph;
    result.verticalAccuracyMeters = source.epv;
    result.horizontalDop = source.hdop;
    result.verticalDop = source.vdop;
    if (source.vel_ned_valid) {
        result.speedMetersPerSecond = source.vel_m_s;
        result.courseRadians = source.cog_rad;
    }
    result.headingRadians = source.heading;
    result.headingAccuracyRadians = source.heading_accuracy;
    if (source.satellites_used != std::numeric_limits<uint8_t>::max()) {
        result.satellitesUsed = source.satellites_used;
    }
    auto& integrity = result.integrity;
    integrity.timestampUs = diagnostic.timestamp;
    integrity.jamming = GPSIntegrityReport::jammingStateFromValue(static_cast<int>(diagnostic.jamming_state));
    integrity.spoofing = GPSIntegrityReport::spoofingStateFromValue(static_cast<int>(diagnostic.spoofing_state));
    integrity.correctionUse =
        GPSIntegrityReport::correctionUseFromValue(static_cast<int>(diagnostic.corrections_msg_used));
    integrity.noisePerMillisecond = diagnostic.noise_per_ms;
    integrity.automaticGainControl = diagnostic.automatic_gain_control;
    integrity.jammingIndicator = diagnostic.jamming_indicator;
    integrity.correctionCrcFailed = diagnostic.corrections_crc_failed;
    return result;
}

GPSSatelliteReport satellites(const GPSNativeSatelliteReport& source)
{
    GPSSatelliteReport result;
    result.timestampUs = source.timestamp;
    result.count = std::min(source.count, GPSSatelliteReport::MAX_SATELLITES);
    for (uint16_t i = 0; i < result.count; ++i) {
        const auto& from = source.entries[i];
        auto& to = result.satellites[i];
        to.id = from.id;
        to.prn = from.prn;
        to.used = from.used;
        if (from.elevation && *from.elevation >= -90 && *from.elevation <= 90) {
            to.elevationDegrees = static_cast<float>(*from.elevation);
        }
        if (from.azimuth && *from.azimuth >= 0 && *from.azimuth <= 360) {
            to.azimuthDegrees = static_cast<float>(*from.azimuth);
        }
        if (from.signal && *from.signal >= 0 && *from.signal <= 255) {
            to.signalStrength = static_cast<uint8_t>(*from.signal);
        }
    }
    return result;
}

GPSSurveyReport survey(const GPSNativeSurveyReport& source)
{
    GPSSurveyReport result;
    result.latitudeDegrees = source.latitude;
    result.longitudeDegrees = source.longitude;
    if (source.altitudeDatum == GPSNativeSurveyReport::AltitudeDatum::Ellipsoid) {
        result.altitudeEllipsoidMeters = source.altitude;
    }
    if (source.accuracyKnown) {
        result.meanAccuracyMeters = static_cast<double>(source.mean_accuracy) / 1000.0;
    }
    result.duration = std::chrono::seconds(source.duration);
    result.valid = (source.flags & 1) != 0;
    result.active = (source.flags & 2) != 0;
    return result;
}

GPSSatelliteReport SatelliteSnapshot::update(const GPSNativeSatelliteReport& source)
{
    const auto projected = satellites(source);
    if (source.constellation) {
        _constellations[*source.constellation].clear();
    } else {
        _constellations.clear();
    }
    for (uint16_t i = 0; i < projected.count; ++i) {
        _constellations[source.constellation.value_or(source.entries[i].constellation)].push_back(
            projected.satellites[i]);
    }
    if (!source.constellation) {
        return projected;
    }
    GPSSatelliteReport snapshot;
    snapshot.timestampUs = projected.timestampUs;
    for (const auto& [constellation, entries] : _constellations) {
        for (const auto& entry : entries) {
            if (snapshot.count == GPSSatelliteReport::MAX_SATELLITES) {
                return snapshot;
            }
            snapshot.satellites[snapshot.count++] = entry;
        }
    }
    return snapshot;
}
}  // namespace GPSNativeData
