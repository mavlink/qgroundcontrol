#include <algorithm>

#include "GPSNativeData_p.h"

namespace GPSNativeData {
GPSPositionReport position(const GPSNativePositionReport& source, const GPSNativeIntegrityReport& diagnostic,
                           uint64_t nowUs)
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
    integrity.jammingTimestampUs = diagnostic.jamming_state_timestamp;
    integrity.spoofingTimestampUs = diagnostic.spoofing_state_timestamp;
    integrity.rfTimestampUs = diagnostic.rf_timestamp;
    integrity.correctionTimestampUs = diagnostic.corrections_timestamp;
    integrity.jamming = GPSIntegrityReport::jammingStateFromValue(static_cast<int>(diagnostic.jamming_state));
    integrity.spoofing = GPSIntegrityReport::spoofingStateFromValue(static_cast<int>(diagnostic.spoofing_state));
    integrity.correctionUse =
        GPSIntegrityReport::correctionUseFromValue(static_cast<int>(diagnostic.corrections_msg_used));
    integrity.noisePerMillisecond = diagnostic.noise_per_ms;
    integrity.automaticGainControl = diagnostic.automatic_gain_control;
    integrity.jammingIndicator = diagnostic.jamming_indicator;
    integrity.correctionCrcFailed = diagnostic.corrections_crc_failed;
    // Navigation epochs retain their first receipt; diagnostics may arrive before that epoch is published.
    integrity = integrity.freshAt(nowUs ? nowUs : std::max(source.timestamp, diagnostic.timestamp));
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
        to.constellation = source.constellation.value_or(from.constellation);
        to.inViewTimestampUs = source.timestamp;
        to.inUseTimestampUs = source.usage ? source.usage->timestamp : (from.used ? source.timestamp : 0);
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

GPSSatelliteReport SatelliteSnapshot::update(const GPSNativeSatelliteReport& source, uint64_t nowUs)
{
    const auto projected = satellites(source);
    GPSSatelliteObservation observation;
    observation.monotonicTimestampUs = source.timestamp;
    observation.updateMode = source.constellation ? GPSSatelliteObservation::UpdateMode::ConstellationDelta
                                                  : GPSSatelliteObservation::UpdateMode::FullSnapshot;
    for (uint16_t i = 0; i < projected.count; ++i) {
        const auto& satellite = projected.satellites[i];
        observation.satellites.append({satellite.id, satellite.prn, satellite.constellation, satellite.used,
                                       satellite.elevationDegrees, satellite.signalStrength, satellite.azimuthDegrees});
    }
    if (source.constellation) {
        GPSSatelliteProvenance provenance;
        provenance.constellation = *source.constellation;
        provenance.inViewTimestampUs = source.timestamp;
        if (source.usage) {
            provenance.inUseTimestampUs = source.usage->timestamp;
            const auto count = std::min(source.usage->count, GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES);
            provenance.satellitesUsed = count;
            provenance.usedSatelliteIds.emplace(source.usage->ids.begin(), source.usage->ids.begin() + count);
        } else {
            int used = 0;
            bool known = !observation.satellites.isEmpty();
            for (const auto& satellite : observation.satellites) {
                known &= satellite.used.has_value();
                used += satellite.used.value_or(false);
            }
            if (known) {
                provenance.inUseTimestampUs = source.timestamp;
                provenance.satellitesUsed = used;
            }
        }
        observation.provenance.append(provenance);
    }
    _latestReceiptUs =
        nowUs ? nowUs : std::max({_latestReceiptUs, source.timestamp, source.usage ? source.usage->timestamp : 0});
    _state.updateObservation(observation, _latestReceiptUs);
    return _snapshot(_latestReceiptUs);
}

std::optional<GPSSatelliteReport> SatelliteSnapshot::expire(uint64_t nowUs)
{
    const auto deadline = _state.nextExpiryUs();
    if (!deadline || nowUs < *deadline) {
        return std::nullopt;
    }
    _latestReceiptUs = nowUs;
    return _snapshot(nowUs);
}

GPSSatelliteReport SatelliteSnapshot::_snapshot(uint64_t nowUs)
{
    const auto accepted = _state.snapshot(nowUs);
    GPSSatelliteReport snapshot;
    for (const auto& provenance : accepted.provenance) {
        snapshot.timestampUs = std::max<uint64_t>(snapshot.timestampUs, provenance.inViewTimestampUs);
    }
    for (const auto& satellite : accepted.satellites) {
        if (snapshot.count == GPSSatelliteReport::MAX_SATELLITES) {
            break;
        }
        auto& entry = snapshot.satellites[snapshot.count++];
        entry.id = satellite.id;
        entry.prn = satellite.prn;
        entry.constellation = satellite.constellation;
        entry.used = satellite.used;
        entry.elevationDegrees = satellite.elevationDegrees;
        entry.azimuthDegrees = satellite.normalizedAzimuthDegrees;
        entry.signalStrength = satellite.signalStrength;
        for (const auto& provenance : accepted.provenance) {
            if (provenance.constellation == satellite.constellation) {
                entry.inViewTimestampUs = provenance.inViewTimestampUs;
                entry.inUseTimestampUs = provenance.inUseTimestampUs;
                break;
            }
        }
    }
    return snapshot;
}
}  // namespace GPSNativeData
