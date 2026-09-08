#include "GPSSourceHealth.h"

#include <chrono>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSSourceHealthLog, "GPS.GPSSourceHealth")

bool GPSObservation::usable() const
{
    const double accuracy = position.attribute(QGeoPositionInfo::HorizontalAccuracy);
    return position.isValid() && position.hasAttribute(QGeoPositionInfo::HorizontalAccuracy) && qIsFinite(accuracy) &&
           accuracy > 0 && accuracy <= 100;
}

QGeoCoordinate GPSObservation::coordinate() const
{
    if (!usable()) {
        return {};
    }
    QGeoCoordinate coordinate(position.coordinate().latitude(), position.coordinate().longitude());
    const double accuracy = position.attribute(QGeoPositionInfo::VerticalAccuracy);
    if (position.hasAttribute(QGeoPositionInfo::VerticalAccuracy) && qIsFinite(accuracy) && accuracy > 0 &&
        accuracy <= 10 && qIsFinite(position.coordinate().altitude())) {
        coordinate.setAltitude(position.coordinate().altitude());
    }
    return coordinate;
}

double GPSObservation::heading() const
{
    const double direction = position.attribute(QGeoPositionInfo::Direction);
    const double speed = position.attribute(QGeoPositionInfo::GroundSpeed);
    const double accuracy = position.attribute(QGeoPositionInfo::DirectionAccuracy);
    const bool accuracyAcceptable = !position.hasAttribute(QGeoPositionInfo::DirectionAccuracy) ||
                                    (qIsFinite(accuracy) && accuracy >= 0 && accuracy <= 30);
    // Both decoders report course over ground, which is unreliable when nearly stationary.
    if (!usable() || !position.hasAttribute(QGeoPositionInfo::Direction) || !qIsFinite(direction) || direction < 0 ||
        direction > 360 || !position.hasAttribute(QGeoPositionInfo::GroundSpeed) || !qIsFinite(speed) || speed < 0.5 ||
        !accuracyAcceptable) {
        return qQNaN();
    }
    return direction == 360 ? 0 : direction;
}

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
    if (timestampUs == 0) {
        return 0;
    }
    const auto now = static_cast<quint64>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
    return timestampUs > now ? -1 : static_cast<qint64>((now - timestampUs) / 1000);
}

double GPSSourceHealth::horizontalAccuracy() const
{
    return usable() ? _observation.position.attribute(QGeoPositionInfo::HorizontalAccuracy) : qQNaN();
}

void GPSSourceHealth::updatePosition(const QGeoPositionInfo& position, qint64 ageMs)
{
    _positionTimer.stop();
    _observation = {position, ageMs >= 0 ? QDateTime::currentDateTimeUtc().addMSecs(-ageMs) : QDateTime()};
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
