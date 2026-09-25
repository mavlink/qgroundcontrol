#include "GPSSourceHealth.h"

#include <algorithm>
#include <chrono>

#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSSourceHealthLog, "GPS.Core.GPSSourceHealth")

GPSSourceHealth::GPSSourceHealth(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _positionTask(_scheduler, this)
{
    qCDebug(GPSSourceHealthLog) << this;
}

GPSSourceHealth::~GPSSourceHealth()
{
    qCDebug(GPSSourceHealthLog) << this;
}

qint64 GPSSourceHealth::_age(quint64 timestampUs) const
{
    return MonotonicClock::ageMilliseconds(timestampUs, _scheduler->nowUs());
}

double GPSSourceHealth::horizontalAccuracy() const
{
    return usable() ? _position.observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy) : qQNaN();
}

std::optional<GPSObservation> GPSSourceHealth::acceptedObservation(
    GPSObservation::PositionUse use, std::optional<std::chrono::milliseconds> maximumAge) const
{
    if (_position.invalidated ||
        _remaining(_position.observation.monotonicTimestampUs, maximumAge) == std::chrono::microseconds::zero()) {
        return std::nullopt;
    }
    return _position.observation.projected(use);
}

std::chrono::microseconds GPSSourceHealth::_remaining(quint64 timestampUs,
                                                      std::optional<std::chrono::milliseconds> maximumAge) const
{
    const auto sourceLifetime = std::chrono::milliseconds(_freshnessTimeoutMs);
    const auto lifetime = maximumAge ? std::min(sourceLifetime, *maximumAge) : sourceLifetime;
    return lifetime > std::chrono::milliseconds::zero()
               ? MonotonicClock::remaining(timestampUs, _scheduler->nowUs(), lifetime)
               : std::chrono::microseconds::zero();
}

void GPSSourceHealth::updateObservation(const GPSObservation& observation)
{
    const State previousState = state();
    const auto update = _revision.advance(this);
    ++_observationRevision;
    _positionTask.cancel();
    _position.observation = observation;
    // Temporal rejection lasts until the next observation.
    _position.invalidated = _age(observation.monotonicTimestampUs) < 0;
    _position.state = _updatedPositionState();
    _schedulePositionExpiry();
    _logStateChange(previousState);
    if (!update.isCurrent()) {
        return;
    }
    emit positionChanged();
}

void GPSSourceHealth::_schedulePositionExpiry()
{
    _positionTask.cancel();
    const auto remaining = _remaining(_position.observation.monotonicTimestampUs);
    if (remaining == std::chrono::microseconds::zero()) {
        return;
    }
    _positionTask.schedule(remaining, [this, position = _revision.current(this)]() {
        if (position.isCurrent()) {
            _setState(State::Stale);
        }
    });
}

void GPSSourceHealth::_logStateChange(State previous) const
{
    if (previous != state()) {
        qCDebug(GPSSourceHealthLog) << this << "Position health changed:" << previous << "->" << state();
    }
}

void GPSSourceHealth::_setState(State state)
{
    if (_position.state != state) {
        const State previous = _position.state;
        _position.state = state;
        _logStateChange(previous);
        emit positionChanged();
    }
}

void GPSSourceHealth::invalidatePosition()
{
    _position.invalidated = true;
    _revision.invalidate();
    _schedulePositionExpiry();
    if (state() == State::Invalid) {
        emit positionChanged();
    } else {
        _setState(_age(_position.observation.monotonicTimestampUs) >= _freshnessTimeoutMs ? State::Stale
                                                                                          : State::Invalid);
    }
}

void GPSSourceHealth::reset()
{
    const State previousState = state();
    const auto update = _revision.advance(this);
    _positionTask.cancel();
    _position = {};
    _logStateChange(previousState);
    if (update.isCurrent()) {
        emit positionChanged();
    }
}

void GPSSourceHealth::setFreshnessTimeoutMs(int timeoutMs)
{
    _freshnessTimeoutMs = std::max(1, timeoutMs);
    if (state() == State::NoData) {
        return;
    }
    const State previousState = state();
    const auto update = _revision.advance(this);
    _position.state = _updatedPositionState();
    _schedulePositionExpiry();
    _logStateChange(previousState);
    if (update.isCurrent()) {
        emit positionChanged();
    }
}

GPSSourceHealth::State GPSSourceHealth::_updatedPositionState() const
{
    const qint64 ageMs = _age(_position.observation.monotonicTimestampUs);
    if (ageMs < 0) {
        return State::Invalid;
    }
    if (ageMs >= _freshnessTimeoutMs) {
        return State::Stale;
    }
    return _position.invalidated ? state() : (_position.observation.usable() ? State::Usable : State::Invalid);
}
