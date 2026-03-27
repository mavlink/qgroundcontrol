#include "GPSPositionSource.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>

#include <cmath>

QGC_LOGGING_CATEGORY(GPSPositionSourceLog, "GPS.GPSPositionSource")

GPSPositionSource::GPSPositionSource(QObject *parent)
    : QGeoPositionInfoSource(parent)
{
    qCDebug(GPSPositionSourceLog) << this;
}

GPSPositionSource::~GPSPositionSource()
{
    qCDebug(GPSPositionSourceLog) << this;
}

QGeoPositionInfo GPSPositionSource::lastKnownPosition(bool) const
{
    return _lastPosition;
}

QGeoPositionInfoSource::PositioningMethods GPSPositionSource::supportedPositioningMethods() const
{
    return SatellitePositioningMethods;
}

int GPSPositionSource::minimumUpdateInterval() const
{
    return kMinUpdateIntervalMs;
}

QGeoPositionInfoSource::Error GPSPositionSource::error() const
{
    return QGeoPositionInfoSource::NoError;
}

void GPSPositionSource::startUpdates()
{
    _running = true;
}

void GPSPositionSource::stopUpdates()
{
    _running = false;
}

void GPSPositionSource::requestUpdate(int)
{
    if (_lastPosition.isValid()) {
        emit positionUpdated(_lastPosition);
    }
}

void GPSPositionSource::updateFromSensorGps(const sensor_gps_s &gps)
{
    if (!_running) {
        return;
    }

    if (gps.fix_type < sensor_gps_s::FIX_TYPE_2D) {
        return;
    }

    QGeoCoordinate coord(gps.latitude_deg, gps.longitude_deg);
    if (gps.fix_type >= sensor_gps_s::FIX_TYPE_3D) {
        coord.setAltitude(gps.altitude_msl_m);
    }

    QDateTime timestamp;
    if (gps.time_utc_usec > 0) {
        timestamp = QDateTime::fromMSecsSinceEpoch(
            static_cast<qint64>(gps.time_utc_usec / 1000), QTimeZone::UTC);
    } else {
        timestamp = QDateTime::currentDateTimeUtc();
    }

    QGeoPositionInfo info(coord, timestamp);

    if (gps.eph > 0 && std::isfinite(gps.eph)) {
        info.setAttribute(QGeoPositionInfo::HorizontalAccuracy, static_cast<qreal>(gps.eph));
    }

    if (gps.epv > 0 && std::isfinite(gps.epv)) {
        info.setAttribute(QGeoPositionInfo::VerticalAccuracy, static_cast<qreal>(gps.epv));
    }

    if (gps.vel_m_s >= 0 && std::isfinite(gps.vel_m_s)) {
        info.setAttribute(QGeoPositionInfo::GroundSpeed, static_cast<qreal>(gps.vel_m_s));
    }

    if (std::isfinite(gps.vel_d_m_s)) {
        // NED convention: vel_d_m_s is positive downward, Qt expects positive upward
        info.setAttribute(QGeoPositionInfo::VerticalSpeed, static_cast<qreal>(-gps.vel_d_m_s));
    }

    if (std::isfinite(gps.cog_rad)) {
        double deg = static_cast<qreal>(gps.cog_rad) * 180.0 / M_PI;
        if (deg < 0.0) deg += 360.0;
        info.setAttribute(QGeoPositionInfo::Direction, deg);
    }

    if (gps.heading_accuracy > 0 && std::isfinite(gps.heading_accuracy)) {
        info.setAttribute(QGeoPositionInfo::DirectionAccuracy,
                          static_cast<qreal>(gps.heading_accuracy));
    }

    _lastPosition = info;
    emit positionUpdated(info);
}
