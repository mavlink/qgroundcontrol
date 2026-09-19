#include <cmath>
#include <iostream>

#include "GPSDriverData.h"
#include "satellite_info.h"
#include "sensor_gps.h"

int main()
{
    sensor_gps_s raw{};
    GPSDriverData::initialize(raw);
    auto report = GPSDriverData::position(raw);
    if (!std::isnan(report.latitudeDegrees) || !std::isnan(report.altitudeEllipsoidMeters) || report.satellitesUsed ||
        report.integrity.noisePerMillisecond || report.integrity.automaticGainControl ||
        report.integrity.jammingIndicator || report.integrity.correctionCrcFailed) {
        std::cerr << "Unreported PX4 fields were converted to known values\n";
        return 1;
    }
    raw.timestamp = 123;
    raw.time_utc_usec = 456;
    raw.latitude_deg = 0;
    raw.longitude_deg = -8.2;
    raw.altitude_msl_m = -25;
    raw.altitude_ellipsoid_m = 7.5;
    raw.fix_type = sensor_gps_s::FIX_TYPE_RTK_FIXED;
    raw.satellites_used = 0;
    raw.noise_per_ms = 0;
    raw.automatic_gain_control = 0;
    raw.jamming_indicator = 0;
    raw.jamming_state = sensor_gps_s::JAMMING_STATE_WARNING;
    raw.spoofing_state = sensor_gps_s::SPOOFING_STATE_MULTIPLE;
    raw.rtcm_msg_used = sensor_gps_s::RTCM_MSG_USED_USED;
    report = GPSDriverData::position(raw);
    if (report.timestampUs != 123 || report.utcTimeUs != 456 || report.latitudeDegrees != 0 ||
        report.longitudeDegrees != -8.2 || report.altitudeMslMeters != -25 || report.altitudeEllipsoidMeters != 7.5 ||
        report.fixType != GPSPositionReport::FixType::RTKFixed || report.satellitesUsed != 0 ||
        report.integrity.noisePerMillisecond != 0 || report.integrity.automaticGainControl != 0 ||
        report.integrity.jammingIndicator != 0 || report.integrity.correctionCrcFailed != false ||
        report.integrity.jamming != GPSIntegrityReport::JammingState::Warning ||
        report.integrity.spoofing != GPSIntegrityReport::SpoofingState::Multiple ||
        report.integrity.correctionUse != GPSIntegrityReport::CorrectionUse::Used ||
        report.integrity.timestampUs != 0) {
        std::cerr << "Native position/diagnostic values or receipt uncertainty were lost\n";
        return 2;
    }
    raw.fix_type = raw.jamming_state = raw.spoofing_state = raw.rtcm_msg_used = 255;
    raw.rtcm_crc_failed = true;
    report = GPSDriverData::position(raw);
    if (report.fixType != GPSPositionReport::FixType::Unknown ||
        report.integrity.jamming != GPSIntegrityReport::JammingState::Unknown ||
        report.integrity.spoofing != GPSIntegrityReport::SpoofingState::Unknown ||
        report.integrity.correctionUse != GPSIntegrityReport::CorrectionUse::Unknown ||
        report.integrity.correctionCrcFailed != true) {
        std::cerr << "Reserved native states must not become valid enum values\n";
        return 3;
    }
    GPSDriverData::initialize(raw);
    report = GPSDriverData::position(raw);
    if (report.timestampUs != 0 || report.integrity.correctionCrcFailed ||
        report.integrity.jamming != GPSIntegrityReport::JammingState::Unknown) {
        std::cerr << "Reconfiguration retained old diagnostics\n";
        return 4;
    }

    satellite_info_s satellites{};
    satellites.count = 255;
    satellites.timestamp = 42;
    satellites.svid[0] = 9;
    satellites.prn[0] = 9;
    satellites.used[0] = 1;
    satellites.elevation[0] = static_cast<uint8_t>(-10);
    satellites.azimuth[0] = 128;
    satellites.snr[0] = 0;
    const auto ublox = GPSDriverData::satellites(satellites, GPSType::ublox);
    if (ublox.count != satellite_info_s::SAT_INFO_MAX_SATELLITES || ublox.timestampUs != 42 ||
        ublox.satellites[0].id != 9 || ublox.satellites[0].prn != 9 || ublox.satellites[0].used != true ||
        ublox.satellites[0].signalStrength != 0 || ublox.satellites[0].elevationDegrees != -10 ||
        !ublox.satellites[0].azimuthDegrees || std::abs(*ublox.satellites[0].azimuthDegrees - 180.70588f) > 0.001f ||
        ublox.satellites[satellite_info_s::SAT_INFO_MAX_SATELLITES].used) {
        std::cerr << "Native satellite conversion lost metadata or exceeded the legacy array\n";
        return 5;
    }
    const auto sbf = GPSDriverData::satellites(satellites, GPSType::septentrio);
    const auto ashtech = GPSDriverData::satellites(satellites, GPSType::trimble);
    if (sbf.count != ublox.count || sbf.satellites[0].id != 0 || sbf.satellites[0].used ||
        sbf.satellites[0].signalStrength || ashtech.satellites[0].azimuthDegrees) {
        std::cerr << "The bridge manufactured unavailable family-specific satellite metadata\n";
        return 6;
    }
    return 0;
}
