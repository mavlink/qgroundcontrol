#include "GPSRelativePositionModel.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSRelativePositionModelLog, "GPS.Models.GPSRelativePositionModel")

GPSRelativePositionModel::GPSRelativePositionModel(QObject* parent, int freshnessTimeoutMs)
    : QObject(parent)
    , _expiryTimer(this)
    , _freshnessTimeoutMs(std::max(1, freshnessTimeoutMs))
{
    qCDebug(GPSRelativePositionModelLog) << this;
    _expiryTimer.setSingleShot(true);
    _expiryTimer.setTimerType(Qt::PreciseTimer);
    connect(&_expiryTimer, &QTimer::timeout, this, &GPSRelativePositionModel::_expire);
}

GPSRelativePositionModel::~GPSRelativePositionModel()
{
    qCDebug(GPSRelativePositionModelLog) << this;
}

double GPSRelativePositionModel::_positionValue(double value, bool accuracy) const
{
    return _fresh && _observation.fixValid && _observation.positionValid && std::isfinite(value) &&
                   (!accuracy || value >= 0)
               ? value
               : std::numeric_limits<double>::quiet_NaN();
}

double GPSRelativePositionModel::north() const
{
    return _positionValue(_observation.positionNedMeters[0]);
}

double GPSRelativePositionModel::east() const
{
    return _positionValue(_observation.positionNedMeters[1]);
}

double GPSRelativePositionModel::down() const
{
    return _positionValue(_observation.positionNedMeters[2]);
}

double GPSRelativePositionModel::northAccuracy() const
{
    return _positionValue(_observation.accuracyNedMeters[0], true);
}

double GPSRelativePositionModel::eastAccuracy() const
{
    return _positionValue(_observation.accuracyNedMeters[1], true);
}

double GPSRelativePositionModel::downAccuracy() const
{
    return _positionValue(_observation.accuracyNedMeters[2], true);
}

double GPSRelativePositionModel::length() const
{
    return _positionValue(_observation.lengthMeters, true);
}

double GPSRelativePositionModel::lengthAccuracy() const
{
    return _positionValue(_observation.lengthAccuracyMeters, true);
}

double GPSRelativePositionModel::heading() const
{
    const auto value = _observation.headingDegrees;
    return _fresh && _observation.fixValid && value && std::isfinite(*value) && *value >= 0 && *value <= 360
               ? (*value == 360 ? 0 : *value)
               : std::numeric_limits<double>::quiet_NaN();
}

double GPSRelativePositionModel::headingAccuracy() const
{
    const auto value = _observation.headingAccuracyDegrees;
    return std::isfinite(heading()) && value && std::isfinite(*value) && *value >= 0
               ? *value
               : std::numeric_limits<double>::quiet_NaN();
}

void GPSRelativePositionModel::beginSession(const QString& sourceId, quint64 sessionId)
{
    if (_sourceId == sourceId && _sessionId == sessionId) {
        return;
    }
    _sourceId = sourceId;
    _sessionId = sessionId;
    _observation = {};
    _fresh = false;
    _expiryTimer.stop();
    emit stateChanged();
}

void GPSRelativePositionModel::reset()
{
    _sourceId.clear();
    _sessionId = 0;
    _observation = {};
    _fresh = false;
    _expiryTimer.stop();
    emit stateChanged();
}

void GPSRelativePositionModel::updateObservation(const GPSRelativeObservation& observation)
{
    const quint64 nowUs = GPSObservation::monotonicNowUs();
    if (_sourceId.isEmpty() || observation.sessionId != _sessionId || !observation.monotonicTimestampUs ||
        observation.monotonicTimestampUs > nowUs ||
        observation.monotonicTimestampUs < _observation.monotonicTimestampUs ||
        (nowUs - observation.monotonicTimestampUs) >= static_cast<quint64>(_freshnessTimeoutMs) * 1000) {
        return;
    }
    _observation = observation;
    _fresh = true;
    _armTimer();
    emit stateChanged();
}

void GPSRelativePositionModel::_armTimer()
{
    const qint64 remaining = _freshnessTimeoutMs - GPSObservation::ageMilliseconds(_observation.monotonicTimestampUs);
    _expiryTimer.start(static_cast<int>(std::max<qint64>(1, remaining)));
}

void GPSRelativePositionModel::_expire()
{
    if (!_fresh) {
        return;
    }
    if (GPSObservation::ageMilliseconds(_observation.monotonicTimestampUs) < _freshnessTimeoutMs) {
        _armTimer();
        return;
    }
    _fresh = false;
    emit stateChanged();
}
