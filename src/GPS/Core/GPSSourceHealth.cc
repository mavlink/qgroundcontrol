#include "GPSSourceHealth.h"

#include <algorithm>
#include <chrono>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSSourceHealthLog, "GPS.Core.GPSSourceHealth")

GPSSourceHealth::GPSSourceHealth(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent), _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this)), _positionTask(_scheduler, this)
{
    qCDebug(GPSSourceHealthLog) << this;
    if (_scheduler->thread() != thread()) {
        qCWarning(GPSSourceHealthLog) << "Scheduler must share the store thread";
        _scheduler = nullptr;
        return;
    }
    connect(_scheduler, &QObject::destroyed, this, [this]() {
        _scheduler = nullptr;
        reset();
    });
}

GPSSourceHealth::~GPSSourceHealth()
{
    qCDebug(GPSSourceHealthLog) << this;
    if (_scheduler) {
        _scheduler->disconnect(this);
    }
}

qint64 GPSSourceHealth::_age(quint64 timestampUs) const
{
    return _scheduler ? MonotonicClock::ageMilliseconds(timestampUs, _scheduler->nowUs()) : -1;
}

double GPSSourceHealth::horizontalAccuracy() const
{
    return usable() ? _observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy) : qQNaN();
}

std::optional<GPSObservation> GPSSourceHealth::acceptedObservation() const
{
    const qint64 age = _age(_observation.monotonicTimestampUs);
    if (!usable() || _positionInvalidated || age < 0 || age >= _freshnessTimeoutMs) {
        return std::nullopt;
    }
    GPSObservation accepted = _observation;
    accepted.position = _observation.acceptedPosition();
    if (!accepted.position.isValid()) {
        return std::nullopt;
    }
    return accepted;
}

void GPSSourceHealth::updateObservation(const GPSObservation& observation)
{
    ++_revision;
    _positionTask.cancel();
    _positionInvalidated = false;
    _observation = observation;
    const auto& position = observation.position;
    const qint64 ageMs = _age(observation.monotonicTimestampUs);
    if (ageMs < 0) {
        _state = State::Invalid;
    } else if (ageMs >= _freshnessTimeoutMs) {
        _state = State::Stale;
    } else {
        _state = _observation.usable() ? State::Usable : State::Invalid;
    }
    _schedulePositionExpiry();
    qCDebug(GPSSourceHealthLog) << this << "Position observation"
                                << "state:" << _state << "coordinate:" << position.coordinate()
                                << "horizontalAccuracy:" << position.attribute(QGeoPositionInfo::HorizontalAccuracy)
                                << "ageMs:" << ageMs;
    emit positionChanged();
}

void GPSSourceHealth::_schedulePositionExpiry()
{
    _positionTask.cancel();
    const qint64 age = _age(_observation.monotonicTimestampUs);
    if (age < 0 || age >= _freshnessTimeoutMs || !_scheduler) {
        return;
    }
    const quint64 revision = _revision;
    _positionTask.schedule(std::chrono::milliseconds(_freshnessTimeoutMs - age), [this, revision]() {
        if (_revision == revision) {
            _setState(State::Stale);
        }
    });
}

void GPSSourceHealth::_setState(State state)
{
    if (_state != state) {
        _state = state;
        qCDebug(GPSSourceHealthLog) << this << "Position health:" << state;
        emit positionChanged();
    }
}

void GPSSourceHealth::invalidatePosition()
{
    _positionInvalidated = true;
    ++_revision;
    _schedulePositionExpiry();
    if (_state == State::Invalid) {
        emit positionChanged();
    } else {
        _setState(_age(_observation.monotonicTimestampUs) >= _freshnessTimeoutMs ? State::Stale : State::Invalid);
    }
}

void GPSSourceHealth::reset()
{
    ++_revision;
    _positionTask.cancel();
    _observation = {};
    _positionInvalidated = true;
    _state = State::NoData;
    emit positionChanged();
}

void GPSSourceHealth::setFreshnessTimeoutMs(int timeoutMs)
{
    _freshnessTimeoutMs = std::max(1, timeoutMs);
    if (_state == State::NoData) {
        return;
    }
    if (!_positionInvalidated) {
        updateObservation(_observation);
    } else {
        ++_revision;
        const qint64 ageMs = _age(_observation.monotonicTimestampUs);
        _schedulePositionExpiry();
        if (ageMs >= _freshnessTimeoutMs) {
            _setState(State::Stale);
        }
    }
}
