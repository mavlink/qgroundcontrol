#include "GPSSourceHealth.h"

#include <algorithm>
#include <chrono>

#include "GPSSatelliteObservation.h"
#include "MonotonicClock.h"
#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSSourceHealthLog, "GPS.Core.GPSSourceHealth")

GPSSourceHealth::GPSSourceHealth(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _positionTask(_scheduler, this)
    , _fixSatellitesTask(_scheduler, this)
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
    auto accepted = _position.observation.projected(use);
    if (accepted) {
        const int used = satellitesInUseCount();
        accepted->satellitesUsed = used >= 0 ? std::optional<int>(used) : std::nullopt;
    }
    return accepted;
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
    const QPointer<GPSSourceHealth> guard(this);
    const State previousState = state();
    const quint64 revision = ++_revision;
    ++_observationRevision;
    _positionTask.cancel();
    _position.observation = observation;
    // Temporal rejection lasts until the next observation.
    _position.invalidated = _age(observation.monotonicTimestampUs) < 0;
    _position.state = _updatedPositionState();
    _schedulePositionExpiry();
    const int previousUsed = satellitesInUseCount();
    _fixSatellites = {observation.hasNavigationSolution() ? observation.satellitesUsed.value_or(-1) : -1,
                      observation.monotonicTimestampUs};
    _scheduleFixSatelliteExpiry();
    _logStateChange(previousState);
    if (previousUsed != satellitesInUseCount()) {
        emit satellitesChanged();
    }
    if (!guard || revision != _revision) {
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
    const quint64 revision = _revision;
    _positionTask.schedule(remaining, [this, revision]() {
        if (_revision == revision) {
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
    ++_revision;
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
    const QPointer<GPSSourceHealth> guard(this);
    const State previousState = state();
    const quint64 revision = ++_revision;
    _positionTask.cancel();
    _position = {};
    _logStateChange(previousState);
    clearSatellites();
    if (guard && revision == _revision) {
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
    const QPointer<GPSSourceHealth> guard(this);
    const quint64 revision = ++_revision;
    const int previousUsed = satellitesInUseCount();
    _position.state = _updatedPositionState();
    _schedulePositionExpiry();
    _scheduleFixSatelliteExpiry();
    _logStateChange(previousState);
    if (previousUsed != satellitesInUseCount()) {
        emit satellitesChanged();
    }
    if (guard && revision == _revision) {
        emit positionChanged();
    }
}

void GPSSourceHealth::_scheduleFixSatelliteExpiry()
{
    _fixSatellitesTask.cancel();
    const auto remaining = _remaining(_fixSatellites.receivedAtUs);
    if (remaining == std::chrono::microseconds::zero()) {
        _fixSatellites.count = -1;
    }
    if (_fixSatellites.count >= 0) {
        _fixSatellitesTask.schedule(remaining, [this]() {
            const int previous = satellitesInUseCount();
            _fixSatellites.count = -1;
            if (previous != satellitesInUseCount()) {
                emit satellitesChanged();
            }
        });
    }
}

void GPSSourceHealth::clearSatellites()
{
    _fixSatellitesTask.cancel();
    const bool changed = _satelliteCounts.inView != -1 || satellitesInUseCount() != -1;
    _satelliteCounts = {};
    _fixSatellites = {};
    if (changed) {
        emit satellitesChanged();
    }
}

void GPSSourceHealth::applySatelliteObservation(const GPSSatelliteObservation& observation)
{
    // The observation store owns report expiry; the fix's independent GGA count retains its own deadline.
    const int previousInView = satellitesInViewCount();
    const int previousInUse = satellitesInUseCount();
    _satelliteCounts = {observation.satellitesInViewCount(), observation.satellitesInUseCount()};
    if (previousInView != satellitesInViewCount() || previousInUse != satellitesInUseCount()) {
        emit satellitesChanged();
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
