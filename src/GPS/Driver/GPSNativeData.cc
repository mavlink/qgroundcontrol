#include <algorithm>

#include "GPSNativeData_p.h"

namespace {
GPSSatellite satelliteObservation(const GPSNativeSatelliteData& source, GPSConstellation constellation)
{
    GPSSatellite result;
    result.id = source.id;
    result.prn = source.prn;
    result.used = source.used;
    result.constellation = constellation;
    if (source.elevation && *source.elevation >= -90 && *source.elevation <= 90) {
        result.elevationDegrees = static_cast<float>(*source.elevation);
    }
    if (source.azimuth && *source.azimuth >= 0 && *source.azimuth <= 360) {
        result.normalizedAzimuthDegrees = static_cast<float>(*source.azimuth);
    }
    if (source.signal && *source.signal >= 0 && *source.signal <= 255) {
        result.signalStrength = static_cast<uint8_t>(*source.signal);
    }
    return result;
}
}  // namespace

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

GPSSurveyReport survey(const GPSNativeSurveyReport& source)
{
    GPSSurveyReport result;
    result.position.latitudeDegrees = source.latitude;
    result.position.longitudeDegrees = source.longitude;
    if (source.altitudeDatum == GPSNativeSurveyReport::AltitudeDatum::Ellipsoid) {
        result.position.altitudeMeters = source.altitude;
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
    GPSSatelliteObservation observation;
    observation.monotonicTimestampUs = source.timestamp;
    observation.updateMode = source.constellation ? GPSSatelliteObservation::UpdateMode::ConstellationDelta
                                                  : GPSSatelliteObservation::UpdateMode::FullSnapshot;
    const auto viewCount =
        std::min({source.count, GPSNativeSatelliteReport::SAT_INFO_MAX_SATELLITES, GPSSatelliteReport::MAX_SATELLITES});
    for (uint16_t i = 0; i < viewCount; ++i) {
        const auto& satellite = source.entries[i];
        observation.satellites.append(
            satelliteObservation(satellite, source.constellation.value_or(satellite.constellation)));
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
