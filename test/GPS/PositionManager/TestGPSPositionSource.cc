#include "TestGPSPositionSource.h"

#include <QtCore/QDateTime>
#include <QtCore/QTimeZone>
#include <QtCore/QtMath>

#include <chrono>
#include <cmath>
#include <utility>

#include "GPSSourceHealth.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(TestGPSPositionSourceLog, "GPS.Test.TestGPSPositionSource")

TestGPSPositionSource::TestGPSPositionSource(QObject* parent)
    : QGeoPositionInfoSource(parent)
    , _requestTimer(this)
    , _updateTimer(this)
{
    qCDebug(TestGPSPositionSourceLog) << this;

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

TestGPSPositionSource::~TestGPSPositionSource()
{
    qCDebug(TestGPSPositionSourceLog) << this;
}

QGeoPositionInfo TestGPSPositionSource::lastKnownPosition(bool /*fromSatellitePositioningMethodsOnly*/) const
{
    return _lastPosition;
}

void TestGPSPositionSource::startUpdates()
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

void TestGPSPositionSource::stopUpdates()
{
    _started = false;
    _updateTimer.stop();
    _pendingFix.reset();
}

void TestGPSPositionSource::setUpdateInterval(int msec)
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

void TestGPSPositionSource::requestUpdate(int timeout)
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

void TestGPSPositionSource::reset()
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

void TestGPSPositionSource::updatePosition(const GPSObservation& fix)
{
    _lastPosition = _positionInfo(fix);
    if (_lastPosition.isValid()) {
        _error = NoError;
        const bool requested = _requestTimer.isActive();
        _requestTimer.stop();
        qCDebug(TestGPSPositionSourceLog)
            << "Decoded receiver fix"
            << "fixType:" << static_cast<int>(fix.fixQuality) << "coordinate:" << _lastPosition.coordinate()
            << "horizontalAccuracy:" << fix.position.attribute(QGeoPositionInfo::HorizontalAccuracy)
            << "verticalAccuracy:" << fix.position.attribute(QGeoPositionInfo::VerticalAccuracy)
            << "timestamp:" << _lastPosition.timestamp() << "forwarding:" << (_started || requested);
        if (_started || requested) {
            _pendingFix = fix;
            if (_pendingFix->monotonicTimestampUs == 0) {
                _pendingFix->monotonicTimestampUs = GPSObservation::monotonicNowUs();
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

void TestGPSPositionSource::_emitPendingUpdate()
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

void TestGPSPositionSource::_reportUpdateTimeout()
{
    if (_updateTimeoutSent) {
        return;
    }
    _updateTimeoutSent = true;
    _error = UpdateTimeoutError;
    emit errorOccurred(_error);
}

QGeoPositionInfo TestGPSPositionSource::_positionInfo(const GPSObservation& fix)
{
    const qint64 age = fix.ageMilliseconds();
    if (age < 0 || age >= GPSSourceHealth::FRESHNESS_TIMEOUT_MS) {
        return {};
    }
    return fix.acceptedPosition(GPSObservation::PositionUse::GroundStation);
}
