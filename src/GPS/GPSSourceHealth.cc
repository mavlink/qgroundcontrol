#include "GPSSourceHealth.h"

#include <QtCore/QPointer>

#include <chrono>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSourceHealthLog, "GPS.GPSSourceHealth")

GPSSourceHealth::GPSSourceHealth(QObject* parent)
    : QObject(parent)
    , _positionTimer(this)
    , _satellitesInViewTimer(this)
    , _satellitesInUseTimer(this)
    , _fixSatellitesInUseTimer(this)
{
    qCDebug(GPSSourceHealthLog) << this;
    for (auto* timer : {&_positionTimer, &_satellitesInViewTimer, &_satellitesInUseTimer, &_fixSatellitesInUseTimer}) {
        timer->setSingleShot(true);
        timer->setTimerType(Qt::PreciseTimer);
        timer->setInterval(FRESHNESS_TIMEOUT_MS);
    }
    connect(&_positionTimer, &QTimer::timeout, this, [this]() { _setState(Stale); });
    connect(&_satellitesInViewTimer, &QTimer::timeout, this, [this]() {
        _satellitesInViewCount = -1;
        emit satellitesChanged();
    });
    connect(&_satellitesInUseTimer, &QTimer::timeout, this, [this]() {
        const int previous = satellitesInUseCount();
        _satellitesInUseCount = -1;
        if (previous != satellitesInUseCount()) {
            emit satellitesChanged();
        }
    });
    connect(&_fixSatellitesInUseTimer, &QTimer::timeout, this, [this]() {
        const int previous = satellitesInUseCount();
        _fixSatellitesInUseCount = -1;
        if (previous != satellitesInUseCount()) {
            emit satellitesChanged();
        }
    });
}

GPSSourceHealth::~GPSSourceHealth()
{
    qCDebug(GPSSourceHealthLog) << this;
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
    const qint64 age = _observation.ageMilliseconds();
    if (!usable() || age < 0 || age >= _freshnessTimeoutMs) {
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
    const quint64 now = GPSObservation::monotonicNowUs();
    observation.monotonicTimestampUs = ageMs < 0 ? now + 1000000 : now - static_cast<quint64>(ageMs) * 1000;
    updateObservation(observation);
}

void GPSSourceHealth::updateObservation(const GPSObservation& observation)
{
    const QPointer<GPSSourceHealth> guard(this);
    const quint64 revision = ++_revision;
    _positionTimer.stop();
    _observation = observation;
    const auto& position = observation.position;
    const qint64 ageMs = observation.ageMilliseconds();
    if (ageMs < 0) {
        _state = Invalid;
    } else if (ageMs >= _freshnessTimeoutMs) {
        _state = Stale;
    } else {
        _state = _observation.usable() ? Usable : Invalid;
    }
    if (_state == Usable) {
        _positionTimer.start(_freshnessTimeoutMs - static_cast<int>(ageMs));
    }
    const int previousUsed = satellitesInUseCount();
    _updateSatelliteCount(observation.position.isValid() ? observation.satellitesUsed.value_or(-1) : -1, ageMs,
                          _fixSatellitesInUseCount, _fixSatellitesInUseTimer);
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
    _positionTimer.stop();
    if (_state != state) {
        _state = state;
        qCDebug(GPSSourceHealthLog) << this << "Position health:" << state;
        emit positionChanged();
    }
}

void GPSSourceHealth::invalidatePosition()
{
    ++_revision;
    _setState(Invalid);
}

void GPSSourceHealth::reset()
{
    const QPointer<GPSSourceHealth> guard(this);
    const quint64 revision = ++_revision;
    _positionTimer.stop();
    _observation = {};
    _state = NoData;
    clearSatellites();
    if (guard && revision == _revision) {
        emit positionChanged();
    }
}

void GPSSourceHealth::_updateSatelliteCount(int count, qint64 ageMs, int& stored, QTimer& timer)
{
    timer.stop();
    stored = count >= 0 && ageMs >= 0 && ageMs < _freshnessTimeoutMs ? count : -1;
    if (stored >= 0) {
        timer.start(_freshnessTimeoutMs - static_cast<int>(ageMs));
    }
}

void GPSSourceHealth::updateSatellitesInView(int count, qint64 ageMs)
{
    _updateSatelliteCount(count, ageMs, _satellitesInViewCount, _satellitesInViewTimer);
    emit satellitesChanged();
}

void GPSSourceHealth::updateSatellitesInUse(int count, qint64 ageMs)
{
    _updateSatelliteCount(count, ageMs, _satellitesInUseCount, _satellitesInUseTimer);
    emit satellitesChanged();
}

void GPSSourceHealth::updateSatelliteCounts(int inView, int inUse, qint64 ageMs)
{
    _updateSatelliteCount(inView, ageMs, _satellitesInViewCount, _satellitesInViewTimer);
    _updateSatelliteCount(inUse, ageMs, _satellitesInUseCount, _satellitesInUseTimer);
    emit satellitesChanged();
}

void GPSSourceHealth::clearSatellites()
{
    _satellitesInViewTimer.stop();
    _satellitesInUseTimer.stop();
    _fixSatellitesInUseTimer.stop();
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
    _satellitesInViewTimer.stop();
    _satellitesInUseTimer.stop();
    _satellitesInViewCount = -1;
    _satellitesInUseCount = -1;
    if (hadVisible || previousUsed != satellitesInUseCount()) {
        emit satellitesChanged();
    }
}
