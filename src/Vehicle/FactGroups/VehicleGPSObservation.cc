#include "VehicleGPSObservation.h"

namespace {
template <typename Message>
VehicleGPSObservation convert(const Message& message)
{
    VehicleGPSObservation result;
    auto& observation = result.position;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.position =
        QGeoPositionInfo(QGeoCoordinate(message.lat * 1e-7, message.lon * 1e-7), observation.receivedAt);
    observation.sourceId = QStringLiteral("VehicleGPS");
    if (message.fix_type >= 3) {
        observation.position.setCoordinate(
            QGeoCoordinate(message.lat * 1e-7, message.lon * 1e-7, message.alt / 1000.0));
        observation.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
    }
    if (message.fix_type <= 1) {
        observation.fixQuality = GPSObservation::FixQuality::NoFix;
    } else if (message.fix_type <= 6) {
        observation.fixQuality = static_cast<GPSObservation::FixQuality>(message.fix_type);
    }
    if (message.eph != UINT16_MAX) {
        observation.horizontalDop = message.eph / 100.0;
    }
    if (message.epv != UINT16_MAX) {
        observation.verticalDop = message.epv / 100.0;
    }
    if (message.cog != UINT16_MAX) {
        observation.position.setAttribute(QGeoPositionInfo::Direction, message.cog / 100.0);
    }
    // MAVLink yaw: 0 means unsupported, 65535 unavailable, and 36000 is north.
    if (message.yaw > 0 && message.yaw <= 36000) {
        observation.trueHeadingDegrees = message.yaw == 36000 ? 0.0 : message.yaw / 100.0;
    }
    if (message.satellites_visible != UINT8_MAX) {
        result.satellitesVisible = message.satellites_visible;
    }
    result.fixType = message.fix_type;
    return result;
}
VehicleGPSObservation highLatencyPosition(int32_t latitude, int32_t longitude, double altitude,
                                         GPSObservation::FixQuality quality, int fixType)
{
    VehicleGPSObservation result;
    result.fixType = fixType;
    result.position.receivedAt = QDateTime::currentDateTimeUtc();
    result.position.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    result.position.sourceId = QStringLiteral("VehicleGPS");
    result.position.position = QGeoPositionInfo(QGeoCoordinate(latitude * 1e-7, longitude * 1e-7),
                                               result.position.receivedAt);
    result.position.fixQuality = quality;
    // High-latency altitude describes the fused vehicle position, not a separate raw GPS altitude.
    result.fusedPosition = result.position;
    result.fusedPosition.sourceId = QStringLiteral("VehicleEKF");
    result.fusedPosition.position.setCoordinate(QGeoCoordinate(latitude * 1e-7, longitude * 1e-7, altitude));
    result.fusedPosition.altitudeDatum = GPSObservation::AltitudeDatum::MeanSeaLevel;
    result.fusedPosition.fixQuality = GPSObservation::FixQuality::Extrapolated;
    return result;
}
}  // namespace

VehicleGPSObservation VehicleGPSObservation::fromMessage(const mavlink_gps_raw_int_t& message)
{
    return convert(message);
}

VehicleGPSObservation VehicleGPSObservation::fromMessage(const mavlink_gps2_raw_t& message)
{
    auto result = convert(message);
    result.position.sourceId = QStringLiteral("VehicleGPS2");
    return result;
}

VehicleGPSObservation VehicleGPSObservation::fromMessage(const mavlink_high_latency_t& message)
{
    const auto quality = message.gps_fix_type <= 1   ? GPSObservation::FixQuality::NoFix
                         : message.gps_fix_type <= 6 ? static_cast<GPSObservation::FixQuality>(message.gps_fix_type)
                                                    : GPSObservation::FixQuality::Unknown;
    return highLatencyPosition(message.latitude, message.longitude, message.altitude_amsl, quality,
                               message.gps_fix_type);
}

VehicleGPSObservation VehicleGPSObservation::fromMessage(const mavlink_high_latency2_t& message)
{
    const bool failed = (message.failure_flags & HL_FAILURE_FLAG_GPS) != 0;
    auto result = highLatencyPosition(message.latitude, message.longitude, message.altitude,
                                      failed ? GPSObservation::FixQuality::NoFix : GPSObservation::FixQuality::Unknown,
                                      failed ? GPS_FIX_TYPE_NO_FIX : GPS_FIX_TYPE_NO_GPS);
    // HL2 reports maximum position errors in decimetres; these are not dilution of precision.
    for (auto* observation : {&result.position, &result.fusedPosition}) {
        if (message.eph != UINT8_MAX) {
            observation->position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, message.eph / 10.0);
        }
        if (message.epv != UINT8_MAX) {
            observation->position.setAttribute(QGeoPositionInfo::VerticalAccuracy, message.epv / 10.0);
        }
    }
    return result;
}
