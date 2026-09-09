#include "GPSSourceHealth.h"

#include <chrono>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSourceHealthLog, "GPS.GPSSourceHealth")

GPSSourceHealth::GPSSourceHealth(QObject* parent)
    : QObject(parent)
    , _positionTimer(this)
    , _satellitesInViewTimer(this)
    , _satellitesInUseTimer(this)
{
    qCDebug(GPSSourceHealthLog) << this;
    for (auto* timer : {&_positionTimer, &_satellitesInViewTimer, &_satellitesInUseTimer}) {
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
        _satellitesInUseCount = -1;
        emit satellitesChanged();
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
    qCDebug(GPSSourceHealthLog) << this << "Position observation"
                               << "state:" << _state
                               << "coordinate:" << position.coordinate()
                               << "horizontalAccuracy:" << position.attribute(QGeoPositionInfo::HorizontalAccuracy)
                               << "ageMs:" << ageMs;
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
    _setState(Invalid);
}

void GPSSourceHealth::reset()
{
    _positionTimer.stop();
    _observation = {};
    _state = NoData;
    clearSatellites();
    emit positionChanged();
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
    if (_satellitesInViewCount != -1 || _satellitesInUseCount != -1) {
        _satellitesInViewCount = -1;
        _satellitesInUseCount = -1;
        emit satellitesChanged();
    }
}
