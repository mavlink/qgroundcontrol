#include <algorithm>
#include <bit>
#include <limits>

#include "GPSPx4Data_p.h"
#include "satellite_info.h"
#include "sensor_gps.h"

namespace GPSPx4Data {

void initialize(sensor_gps_s& position)
{
    position = {};
    position.latitude_deg = position.longitude_deg = position.altitude_msl_m = position.altitude_ellipsoid_m =
        std::numeric_limits<double>::quiet_NaN();
    position.s_variance_m_s = position.c_variance_rad = position.eph = position.epv = position.hdop = position.vdop =
        position.vel_m_s = position.vel_n_m_s = position.vel_e_m_s = position.vel_d_m_s = position.cog_rad =
            position.heading = position.heading_accuracy = std::numeric_limits<float>::quiet_NaN();
    position.noise_per_ms = position.jamming_indicator = -1;
    position.automatic_gain_control = (std::numeric_limits<uint16_t>::max)();
    position.satellites_used = (std::numeric_limits<uint8_t>::max)();
}

GPSPositionReport position(const sensor_gps_s& source)
{
    GPSPositionReport result;
    result.timestampUs = source.timestamp;
    result.utcTimeUs = source.time_utc_usec;
    switch (source.fix_type) {
        case sensor_gps_s::FIX_TYPE_NONE:
            result.fixType = GPSPositionReport::FixType::NoFix;
            break;
        case sensor_gps_s::FIX_TYPE_2D:
            result.fixType = GPSPositionReport::FixType::Fix2D;
            break;
        case sensor_gps_s::FIX_TYPE_3D:
            result.fixType = GPSPositionReport::FixType::Fix3D;
            break;
        case sensor_gps_s::FIX_TYPE_RTCM_CODE_DIFFERENTIAL:
            result.fixType = GPSPositionReport::FixType::Differential;
            break;
        case sensor_gps_s::FIX_TYPE_RTK_FLOAT:
            result.fixType = GPSPositionReport::FixType::RTKFloat;
            break;
        case sensor_gps_s::FIX_TYPE_RTK_FIXED:
            result.fixType = GPSPositionReport::FixType::RTKFixed;
            break;
        case sensor_gps_s::FIX_TYPE_EXTRAPOLATED:
            result.fixType = GPSPositionReport::FixType::Extrapolated;
            break;
        default:
            break;
    }
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
    if (source.satellites_used != (std::numeric_limits<uint8_t>::max)()) {
        result.satellitesUsed = source.satellites_used;
    }

    auto& integrity = result.integrity;
    switch (source.jamming_state) {
        case sensor_gps_s::JAMMING_STATE_OK:
            integrity.jamming = GPSIntegrityReport::JammingState::Ok;
            break;
        case sensor_gps_s::JAMMING_STATE_WARNING:
            integrity.jamming = GPSIntegrityReport::JammingState::Warning;
            break;
        case sensor_gps_s::JAMMING_STATE_CRITICAL:
            integrity.jamming = GPSIntegrityReport::JammingState::Critical;
            break;
        default:
            break;
    }
    switch (source.spoofing_state) {
        case sensor_gps_s::SPOOFING_STATE_NONE:
            integrity.spoofing = GPSIntegrityReport::SpoofingState::None;
            break;
        case sensor_gps_s::SPOOFING_STATE_INDICATED:
            integrity.spoofing = GPSIntegrityReport::SpoofingState::Indicated;
            break;
        case sensor_gps_s::SPOOFING_STATE_MULTIPLE:
            integrity.spoofing = GPSIntegrityReport::SpoofingState::Multiple;
            break;
        default:
            break;
    }
    if (source.noise_per_ms >= 0) {
        integrity.noisePerMillisecond = source.noise_per_ms;
    }
    if (source.automatic_gain_control != (std::numeric_limits<uint16_t>::max)()) {
        integrity.automaticGainControl = source.automatic_gain_control;
    }
    if (source.jamming_indicator >= 0) {
        integrity.jammingIndicator = source.jamming_indicator;
    }
    if (source.rtcm_msg_used == sensor_gps_s::RTCM_MSG_USED_NOT_USED) {
        integrity.correctionUse = GPSIntegrityReport::CorrectionUse::NotUsed;
    } else if (source.rtcm_msg_used == sensor_gps_s::RTCM_MSG_USED_USED) {
        integrity.correctionUse = GPSIntegrityReport::CorrectionUse::Used;
    }
    // The legacy struct has no receipt/validity bit for a clean, unknown-use RTCM report.
    if (source.rtcm_crc_failed || integrity.correctionUse != GPSIntegrityReport::CorrectionUse::Unknown) {
        integrity.correctionCrcFailed = source.rtcm_crc_failed;
    }
    return result;
}

GPSSatelliteReport satellites(const satellite_info_s& source, GPSType type)
{
    GPSSatelliteReport result;
    result.timestampUs = source.timestamp;
    result.count = (std::min) (source.count, satellite_info_s::SAT_INFO_MAX_SATELLITES);
    if (type == GPSType::septentrio) {
        // The PX4 SBF wrapper publishes a count, not per-satellite observations.
        return result;
    }
    for (uint16_t i = 0; i < result.count; ++i) {
        auto& satellite = result.satellites[i];
        satellite.id = source.svid[i];
        satellite.prn = source.prn[i];
        satellite.used = source.used[i] != 0;
        if (satellite.id == 0) {
            continue;
        }
        satellite.signalStrength = source.snr[i];
        const int elevation = std::bit_cast<int8_t>(source.elevation[i]);
        if (elevation >= -90 && elevation <= 90) {
            satellite.elevationDegrees = static_cast<float>(elevation);
        }
        if (type == GPSType::ublox) {
            satellite.azimuthDegrees = static_cast<float>(source.azimuth[i]) * 360.0f / 255.0f;
        }
        // Ashtech truncates degree azimuths to uint8_t; the original angle is unrecoverable.
    }
    return result;
}

}  // namespace GPSPx4Data
