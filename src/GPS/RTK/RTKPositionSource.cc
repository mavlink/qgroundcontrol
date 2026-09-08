#include "RTKPositionSource.h"

#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>
#include <QtCore/QtMath>

#include <chrono>
#include <cmath>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTKPositionSourceLog, "GPS.RTK.RTKPositionSource")

RTKPositionSource::RTKPositionSource(QObject* parent)
    : QGeoPositionInfoSource(parent)
    , _requestTimer(this)
{
    qCDebug(RTKPositionSourceLog) << this;

    _requestTimer.setSingleShot(true);
    connect(&_requestTimer, &QTimer::timeout, this, [this]() {
        _error = UpdateTimeoutError;
        emit errorOccurred(_error);
    });
}

RTKPositionSource::~RTKPositionSource()
{
    qCDebug(RTKPositionSourceLog) << this;
}

QGeoPositionInfo RTKPositionSource::lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const
{
    return _lastPosition;
}

void RTKPositionSource::startUpdates()
{
    _started = true;
    _error = NoError;
}

void RTKPositionSource::stopUpdates()
{
    _started = false;
}

void RTKPositionSource::requestUpdate(int timeout)
{
    if (timeout < 0) {
        _error = UpdateTimeoutError;
        emit errorOccurred(_error);
        return;
    }
    _requestTimer.start(timeout == 0 ? 5000 : timeout);
}

void RTKPositionSource::reset()
{
    _lastPosition = {};
    const bool requested = _requestTimer.isActive();
    _requestTimer.stop();
    _error = ClosedError;
    if (_started || requested) {
        emit errorOccurred(_error);
    }
}

void RTKPositionSource::updatePosition(const sensor_gps_s& fix)
{
    _lastPosition = _positionInfo(fix);
    if (_lastPosition.isValid()) {
        _error = NoError;
        const bool requested = _requestTimer.isActive();
        _requestTimer.stop();
        qCDebug(RTKPositionSourceLog) << "Decoded receiver fix"
                                      << "fixType:" << fix.fix_type
                                      << "coordinate:" << _lastPosition.coordinate()
                                      << "horizontalAccuracy:" << fix.eph
                                      << "verticalAccuracy:" << fix.epv
                                      << "timestamp:" << _lastPosition.timestamp()
                                      << "forwarding:" << (_started || requested);
        if (_started || requested) {
            emit positionUpdated(_lastPosition);
        }
    } else if (_started) {
        _error = UpdateTimeoutError;
        emit errorOccurred(_error);
    }
}

QGeoPositionInfo RTKPositionSource::_positionInfo(const sensor_gps_s& fix)
{
    if (fix.fix_type < sensor_gps_s::FIX_TYPE_2D || fix.fix_type > sensor_gps_s::FIX_TYPE_RTK_FIXED) {
        qCDebug(RTKPositionSourceLog) << "Rejected receiver fix: unsupported fix type"
                                      << "fixType:" << fix.fix_type;
        return {};
    }
    if (!qIsFinite(fix.eph) || fix.eph <= 0) {
        qCDebug(RTKPositionSourceLog) << "Rejected receiver fix: invalid horizontal accuracy"
                                      << "horizontalAccuracy:" << fix.eph;
        return {};
    }
    // The PX4 adapter stamps fixes with steady_clock; queued GUI delivery must not revive old data.
    if (fix.timestamp != 0) {
        const auto now =
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
        if (fix.timestamp > static_cast<uint64_t>(now)) {
            qCDebug(RTKPositionSourceLog) << "Rejected receiver fix: future monotonic timestamp"
                                          << "fixTimestampUs:" << fix.timestamp
                                          << "nowUs:" << now;
            return {};
        }
        if (static_cast<uint64_t>(now) - fix.timestamp > 5000000) {
            qCDebug(RTKPositionSourceLog) << "Rejected receiver fix: stale before delivery"
                                          << "ageUs:" << (static_cast<uint64_t>(now) - fix.timestamp);
            return {};
        }
    }
    QGeoCoordinate coordinate(fix.latitude_deg, fix.longitude_deg);
    if (!coordinate.isValid()) {
        qCDebug(RTKPositionSourceLog) << "Rejected receiver fix: invalid coordinate"
                                      << "latitude:" << fix.latitude_deg
                                      << "longitude:" << fix.longitude_deg;
        return {};
    }
    const bool altitudeValid = fix.fix_type >= sensor_gps_s::FIX_TYPE_3D && qIsFinite(fix.altitude_msl_m);
    if (altitudeValid) {
        coordinate.setAltitude(fix.altitude_msl_m);
    }
    // Some receivers have a position before UTC is available. Use local reception time in that case.
    const QDateTime timestamp =
        fix.time_utc_usec > 0
            ? QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(fix.time_utc_usec / 1000), QTimeZone::UTC)
            : QDateTime::currentDateTimeUtc();
    if (!timestamp.isValid()) {
        qCDebug(RTKPositionSourceLog) << "Rejected receiver fix: invalid UTC timestamp"
                                      << "timeUtcUs:" << fix.time_utc_usec;
    }
    QGeoPositionInfo position(coordinate, timestamp);
    position.setAttribute(QGeoPositionInfo::HorizontalAccuracy, fix.eph);
    if (altitudeValid && qIsFinite(fix.epv) && fix.epv > 0) {
        position.setAttribute(QGeoPositionInfo::VerticalAccuracy, fix.epv);
    }
    if (fix.vel_ned_valid) {
        if (qIsFinite(fix.vel_m_s) && fix.vel_m_s >= 0) {
            position.setAttribute(QGeoPositionInfo::GroundSpeed, fix.vel_m_s);
        }
        if (qIsFinite(fix.vel_d_m_s)) {
            position.setAttribute(QGeoPositionInfo::VerticalSpeed, -fix.vel_d_m_s);
        }
        if (qIsFinite(fix.cog_rad)) {
            const double degrees = std::fmod(qRadiansToDegrees(static_cast<double>(fix.cog_rad)), 360.0);
            position.setAttribute(QGeoPositionInfo::Direction, degrees < 0 ? degrees + 360.0 : degrees);
            if (qIsFinite(fix.c_variance_rad) && fix.c_variance_rad > 0) {
                position.setAttribute(QGeoPositionInfo::DirectionAccuracy, qRadiansToDegrees(fix.c_variance_rad));
            }
        }
    }
    return position;
}
