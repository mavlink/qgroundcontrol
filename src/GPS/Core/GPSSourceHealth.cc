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

std::optional<GPSObservation> GPSSourceHealth::acceptedObservation(
    GPSObservation::PositionUse use, std::optional<std::chrono::milliseconds> maximumAge) const
{
    if (_positionInvalidated ||
        _remaining(_observation.monotonicTimestampUs, maximumAge) == std::chrono::microseconds::zero()) {
        return std::nullopt;
    }
    auto accepted = _observation.projected(use);
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
    return _scheduler && lifetime > std::chrono::milliseconds::zero()
               ? MonotonicClock::remaining(timestampUs, _scheduler->nowUs(), lifetime)
               : std::chrono::microseconds::zero();
}

void GPSSourceHealth::updateObservation(const GPSObservation& observation)
{
    const QPointer<GPSSourceHealth> guard(this);
    const quint64 revision = ++_revision;
    ++_observationRevision;
    _positionTask.cancel();
    _observation = observation;
    const auto& position = observation.position;
    const qint64 ageMs = _age(observation.monotonicTimestampUs);
    // Temporal rejection lasts until the next observation.
    _positionInvalidated = ageMs < 0;
    if (ageMs < 0) {
        _state = State::Invalid;
    } else if (ageMs >= _freshnessTimeoutMs) {
        _state = State::Stale;
    } else {
        _state = _observation.usable() ? State::Usable : State::Invalid;
    }
    _schedulePositionExpiry();
    const int previousUsed = satellitesInUseCount();
    const bool validFix = observation.position.isValid() && observation.receiverFixValid.value_or(true) &&
                          observation.fixQuality != GPSObservation::FixQuality::NoFix;
    _fixSatellitesInUseCount = validFix ? observation.satellitesUsed.value_or(-1) : -1;
    _fixSatellitesTimestampUs = observation.monotonicTimestampUs;
    _scheduleFixSatelliteExpiry();
    qCDebug(GPSSourceHealthLog) << this << "Position observation"
                                << "state:" << _state << "coordinate:" << position.coordinate()
                                << "horizontalAccuracy:" << position.attribute(QGeoPositionInfo::HorizontalAccuracy)
                                << "ageMs:" << ageMs;
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
    const auto remaining = _remaining(_observation.monotonicTimestampUs);
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
    const QPointer<GPSSourceHealth> guard(this);
    const quint64 revision = ++_revision;
    _positionTask.cancel();
    _observation = {};
    _positionInvalidated = true;
    _state = State::NoData;
    clearSatellites();
    if (guard && revision == _revision) {
        emit positionChanged();
    }
}

void GPSSourceHealth::setFreshnessTimeoutMs(int timeoutMs)
{
    _freshnessTimeoutMs = std::max(1, timeoutMs);
    if (_state == State::NoData) {
        return;
    }
    const QPointer<GPSSourceHealth> guard(this);
    const quint64 revision = ++_revision;
    const int previousUsed = satellitesInUseCount();
    const qint64 ageMs = _age(_observation.monotonicTimestampUs);
    if (ageMs < 0) {
        _state = State::Invalid;
    } else if (ageMs >= _freshnessTimeoutMs) {
        _state = State::Stale;
    } else if (!_positionInvalidated) {
        _state = _observation.usable() ? State::Usable : State::Invalid;
    }
    _schedulePositionExpiry();
    _scheduleFixSatelliteExpiry();
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
    const auto remaining = _remaining(_fixSatellitesTimestampUs);
    if (remaining == std::chrono::microseconds::zero()) {
        _fixSatellitesInUseCount = -1;
    }
    if (_fixSatellitesInUseCount >= 0) {
        _fixSatellitesTask.schedule(remaining, [this]() {
            const int previous = satellitesInUseCount();
            _fixSatellitesInUseCount = -1;
            if (previous != satellitesInUseCount()) {
                emit satellitesChanged();
            }
        });
    }
}

void GPSSourceHealth::clearSatellites()
{
    _fixSatellitesTask.cancel();
    _fixSatellitesTimestampUs = 0;
    if (_satellitesInViewCount != -1 || satellitesInUseCount() != -1) {
        _satellitesInViewCount = -1;
        _satellitesInUseCount = -1;
        _fixSatellitesInUseCount = -1;
        emit satellitesChanged();
    }
}

void GPSSourceHealth::applySatelliteObservation(const GPSSatelliteObservation& observation)
{
    // The observation store owns report expiry; the fix's independent GGA count retains its own deadline.
    _satellitesInViewCount = observation.satellitesInViewCount();
    _satellitesInUseCount = observation.satellitesInUseCount();
    emit satellitesChanged();
}
