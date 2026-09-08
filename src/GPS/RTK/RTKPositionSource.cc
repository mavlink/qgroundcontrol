#include "RTKPositionSource.h"

#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>
#include <QtCore/QtMath>

#include <chrono>
#include <cmath>
#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(RTKPositionSourceLog, "GPS.RTK.RTKPositionSource")

namespace {
uint64_t monotonicTimeUs()
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}
}  // namespace

RTKPositionSource::RTKPositionSource(QObject* parent)
    : QGeoPositionInfoSource(parent)
    , _requestTimer(this)
    , _updateTimer(this)
{
    qCDebug(RTKPositionSourceLog) << this;

    _requestTimer.setSingleShot(true);
    connect(&_requestTimer, &QTimer::timeout, this, [this]() {
        _error = UpdateTimeoutError;
        emit errorOccurred(_error);
    });
    connect(&_updateTimer, &QTimer::timeout, this, [this]() {
        if (_pendingFix) {
            _emitPendingUpdate();
        } else {
            const bool missedPreviousInterval = _noUpdateLastInterval;
            _noUpdateLastInterval = true;
            if (missedPreviousInterval) {
                _reportUpdateTimeout();
            }
        }
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
    if (_started) {
        return;
    }
    _error = NoError;
    _started = true;
    _updateTimeoutSent = false;
    _noUpdateLastInterval = false;
    if (updateInterval() > 0) {
        _updateTimer.start(updateInterval());
    }
}

void RTKPositionSource::stopUpdates()
{
    _started = false;
    _updateTimer.stop();
    _pendingFix.reset();
}

void RTKPositionSource::setUpdateInterval(int msec)
{
    const int interval = qMax(0, msec);
    if (interval == updateInterval()) {
        return;
    }
    QGeoPositionInfoSource::setUpdateInterval(interval);
    _updateTimer.stop();
    _noUpdateLastInterval = false;
    if (_started) {
        if (interval > 0 && _error != ClosedError) {
            _updateTimer.start(interval);
        } else if (interval == 0) {
            _emitPendingUpdate();
        }
    }
}

void RTKPositionSource::requestUpdate(int timeout)
{
    if (_requestTimer.isActive()) {
        return;
    }
    _error = NoError;
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
    _pendingFix.reset();
    _updateTimer.stop();
    _noUpdateLastInterval = false;
    _updateTimeoutSent = false;
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
            _pendingFix = fix;
            if (_pendingFix->timestamp == 0) {
                _pendingFix->timestamp = monotonicTimeUs();
            }
            if (_pendingFix->time_utc_usec == 0) {
                _pendingFix->time_utc_usec =
                    static_cast<uint64_t>(_lastPosition.timestamp().toMSecsSinceEpoch()) * 1000;
            }
            if (_started && updateInterval() > 0 && !_updateTimer.isActive()) {
                _updateTimer.start(updateInterval());
            }
            if (requested || !_updateTimer.isActive() || _noUpdateLastInterval || _updateTimeoutSent) {
                _emitPendingUpdate();
            }
        }
    } else {
        _pendingFix.reset();
        if (_started) {
            _reportUpdateTimeout();
        }
    }
}

void RTKPositionSource::_emitPendingUpdate()
{
    const auto pending = std::exchange(_pendingFix, std::nullopt);
    if (!pending) {
        return;
    }
    // Recheck age after interval throttling; a queued fix must not become a fresh live position.
    const QGeoPositionInfo position = _positionInfo(*pending);
    if (!position.isValid()) {
        _reportUpdateTimeout();
        return;
    }
    _updateTimeoutSent = false;
    _noUpdateLastInterval = false;
    _error = NoError;
    emit positionUpdated(position);
}

void RTKPositionSource::_reportUpdateTimeout()
{
    if (_updateTimeoutSent) {
        return;
    }
    _updateTimeoutSent = true;
    _error = UpdateTimeoutError;
    emit errorOccurred(_error);
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
        const uint64_t now = monotonicTimeUs();
        if (fix.timestamp > now) {
            qCDebug(RTKPositionSourceLog) << "Rejected receiver fix: future monotonic timestamp"
                                          << "fixTimestampUs:" << fix.timestamp
                                          << "nowUs:" << now;
            return {};
        }
        if (now - fix.timestamp > 5000000) {
            qCDebug(RTKPositionSourceLog) << "Rejected receiver fix: stale before delivery"
                                          << "ageUs:" << (now - fix.timestamp);
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
