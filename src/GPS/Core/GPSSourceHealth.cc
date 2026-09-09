#include "GPSSourceHealth.h"

#include <QtCore/QPointer>

#include <algorithm>
#include <chrono>

#include "GPSQtRuntimeScheduler.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSourceHealthLog, "GPS.Core.GPSSourceHealth")

GPSSourceHealth::GPSSourceHealth(QObject* parent, GPSRuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new GPSQtRuntimeScheduler(this))
{
    qCDebug(GPSSourceHealthLog) << this;
}

GPSSourceHealth::~GPSSourceHealth()
{
    qCDebug(GPSSourceHealthLog) << this;
    _cancel(_positionTask);
    _cancel(_fixSatellitesTask);
}

void GPSSourceHealth::_cancel(GPSRuntimeScheduler::TaskId& task)
{
    if (_scheduler && task) {
        _scheduler->cancel(task);
    }
    task = 0;
}

qint64 GPSSourceHealth::_age(quint64 timestampUs) const
{
    if (!_scheduler || !timestampUs || timestampUs > _scheduler->nowUs()) {
        return -1;
    }
    return static_cast<qint64>((_scheduler->nowUs() - timestampUs) / 1000);
}

qint64 GPSSourceHealth::ageMilliseconds(quint64 timestampUs)
{
    return GPSObservation::ageMilliseconds(timestampUs);
}

double GPSSourceHealth::horizontalAccuracy() const
{
    return usable() ? _observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy) : qQNaN();
}

std::optional<GPSObservation> GPSSourceHealth::acceptedObservation(GPSObservation::PositionUse use) const
{
    const qint64 age = _age(_observation.monotonicTimestampUs);
    const bool rawPolicy = use == GPSObservation::PositionUse::Diagnostics || use == GPSObservation::PositionUse::Gga;
    if ((!rawPolicy && !usable()) || (_positionInvalidated && use != GPSObservation::PositionUse::Diagnostics) ||
        age < 0 || age >= _freshnessTimeoutMs) {
        return std::nullopt;
    }
    GPSObservation accepted = _observation;
    accepted.position = _observation.acceptedPosition(use);
    if (use == GPSObservation::PositionUse::RemoteID && _observation.altitudeEllipsoidMeters &&
        qIsFinite(*_observation.altitudeEllipsoidMeters)) {
        accepted.altitudeDatum = GPSObservation::AltitudeDatum::Ellipsoid;
    }
    if (!accepted.position.isValid()) {
        return std::nullopt;
    }
    const int used = satellitesInUseCount();
    accepted.satellitesUsed = used >= 0 ? std::optional<int>(used) : std::nullopt;
    return accepted;
}

void GPSSourceHealth::updatePosition(const QGeoPositionInfo& position, qint64 ageMs)
{
    GPSObservation observation;
    observation.position = position;
    observation.receivedAt = ageMs >= 0 ? QDateTime::currentDateTimeUtc().addMSecs(-ageMs) : QDateTime();
    const quint64 now = _scheduler ? _scheduler->nowUs() : 0;
    observation.monotonicTimestampUs = ageMs < 0 ? now + 1000000 : now - static_cast<quint64>(ageMs) * 1000;
    updateObservation(observation);
}

void GPSSourceHealth::updateObservation(const GPSObservation& observation)
{
    const QPointer<GPSSourceHealth> guard(this);
    const quint64 revision = ++_revision;
    _cancel(_positionTask);
    _positionInvalidated = false;
    _observation = observation;
    const auto& position = observation.position;
    const qint64 ageMs = _age(observation.monotonicTimestampUs);
    if (ageMs < 0) {
        _state = Invalid;
    } else if (ageMs >= _freshnessTimeoutMs) {
        _state = Stale;
    } else {
        _state = _observation.usable() ? Usable : Invalid;
    }
    if (_state == Usable) {
        _positionTask =
            _scheduler->schedule(this, std::chrono::milliseconds(_freshnessTimeoutMs - ageMs), [this, revision]() {
                _positionTask = 0;
                if (_revision == revision) {
                    _setState(Stale);
                }
            });
    }
    const int previousUsed = satellitesInUseCount();
    const bool validFix = observation.position.isValid() && observation.receiverFixValid.value_or(true) &&
                          observation.fixQuality != GPSObservation::FixQuality::NoFix;
    _updateFixSatelliteCount(validFix ? observation.satellitesUsed.value_or(-1) : -1, ageMs);
    qCDebug(GPSSourceHealthLog) << this << "Position observation"
                               << "state:" << _state
                               << "coordinate:" << position.coordinate()
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

void GPSSourceHealth::_setState(State state)
{
    _cancel(_positionTask);
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
    _setState(Invalid);
}

void GPSSourceHealth::reset()
{
    const QPointer<GPSSourceHealth> guard(this);
    const quint64 revision = ++_revision;
    _cancel(_positionTask);
    _observation = {};
    _positionInvalidated = true;
    _state = NoData;
    clearSatellites();
    if (guard && revision == _revision) {
        emit positionChanged();
    }
}

void GPSSourceHealth::_updateFixSatelliteCount(int count, qint64 ageMs)
{
    _cancel(_fixSatellitesTask);
    _fixSatellitesInUseCount = count >= 0 && ageMs >= 0 && ageMs < _freshnessTimeoutMs ? count : -1;
    if (_fixSatellitesInUseCount >= 0 && _scheduler) {
        _fixSatellitesTask =
            _scheduler->schedule(this, std::chrono::milliseconds(_freshnessTimeoutMs - ageMs), [this]() {
                _fixSatellitesTask = 0;
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
    _cancel(_fixSatellitesTask);
    if (_satellitesInViewCount != -1 || satellitesInUseCount() != -1) {
        _satellitesInViewCount = -1;
        _satellitesInUseCount = -1;
        _fixSatellitesInUseCount = -1;
        emit satellitesChanged();
    }
}

void GPSSourceHealth::clearSatelliteReports()
{
    const int previousUsed = satellitesInUseCount();
    const bool hadVisible = _satellitesInViewCount >= 0;
    _satellitesInViewCount = -1;
    _satellitesInUseCount = -1;
    if (hadVisible || previousUsed != satellitesInUseCount()) {
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

void GPSSourceHealth::setFreshnessTimeoutMs(int timeoutMs)
{
    _freshnessTimeoutMs = std::max(1, timeoutMs);
    if (_state != NoData && !_positionInvalidated) {
        updateObservation(_observation);
    }
}
