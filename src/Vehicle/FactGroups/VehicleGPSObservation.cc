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
