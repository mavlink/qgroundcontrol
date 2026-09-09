#include "GPSPositionSourceAdapter.h"

#include <utility>

#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GPSPositionSourceAdapterLog, "GPS.PositionManager.GPSPositionSourceAdapter")

GPSPositionSourceAdapter::GPSPositionSourceAdapter(QObject* parent)
    : QObject(parent)
    , _fallbackHealth(this)
{
    qCDebug(GPSPositionSourceAdapterLog) << this;
    connect(&_fallbackHealth, &GPSSourceHealth::positionChanged, this, &GPSPositionSourceAdapter::observationChanged);
}

GPSPositionSourceAdapter::~GPSPositionSourceAdapter()
{
    qCDebug(GPSPositionSourceAdapterLog) << this;
    _disconnectSource();
}

void GPSPositionSourceAdapter::_disconnectSource()
{
    for (const auto& connection : _connections) {
        QObject::disconnect(connection);
    }
    _connections.clear();
    const bool wasActive = std::exchange(_active, false);
    if (_source && wasActive && !_providedHealth) {
        _source->stopUpdates();
    }
}

void GPSPositionSourceAdapter::configure(QObject* producer, GPSSourceHealth* health, const QString& identity,
                                         bool platform)
{
    if (_producer == producer && _providedHealth == health && _identity == identity && _platform == platform) {
        return;
    }
    const QPointer<GPSPositionSourceAdapter> guard(this);
    const quint64 generation = ++_generation;
    _disconnectSource();
    if (!guard || generation != _generation) {
        return;
    }
    _producer = producer;
    _source = qobject_cast<QGeoPositionInfoSource*>(producer);
    _providedHealth = health;
    _identity = identity;
    _platform = platform;
    _fallbackHealth.reset();
    if (!guard || generation != _generation || !_producer) {
        return;
    }
    _connections.append(connect(producer, &QObject::destroyed, this, [this]() { emit bindingChanged(); }));
    if (health) {
        _connections.append(
            connect(health, &GPSSourceHealth::positionChanged, this, &GPSPositionSourceAdapter::observationChanged));
        _connections.append(connect(health, &QObject::destroyed, this, [this]() {
            _active = false;
            emit bindingChanged();
        }));
    }
    if (!_source) {
        return;
    }
    _connections.append(connect(_source, &QGeoPositionInfoSource::positionUpdated, this,
                                [this, generation](const QGeoPositionInfo& position) {
                                    if (_generation == generation && _active && !_providedHealth) {
                                        updatePosition(position);
                                    }
                                }));
    _connections.append(connect(
        _source, &QGeoPositionInfoSource::errorOccurred, this, [this, generation](QGeoPositionInfoSource::Error error) {
            if (_generation != generation || !_active || _providedHealth) {
                return;
            }
            const QPointer<GPSPositionSourceAdapter> errorGuard(this);
            emit backendError(error);
            if (errorGuard && _generation == generation && error != QGeoPositionInfoSource::NoError) {
                _fallbackHealth.invalidatePosition();
            }
        }));
}

void GPSPositionSourceAdapter::setActive(bool active)
{
    if (_active == active || !_producer) {
        return;
    }
    _active = active;
    if (_providedHealth || !_source) {
        return;
    }
    const QPointer<GPSPositionSourceAdapter> guard(this);
    const quint64 generation = _generation;
    if (!active) {
        _source->stopUpdates();
        if (guard && generation == _generation && !_active) {
            _fallbackHealth.reset();
        }
        return;
    }
    _source->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
    if (!guard || generation != _generation || !_source || !_active) {
        return;
    }
#if defined(Q_OS_DARWIN) || defined(Q_OS_IOS)
    if (!_platform) {
        _source->setUpdateInterval(0);
    }
#else
    _source->setUpdateInterval(updateInterval());
#endif
    if (guard && generation == _generation && _source && _active) {
        _source->startUpdates();
    }
}

int GPSPositionSourceAdapter::updateInterval() const
{
    return _platform && _source ? _source->minimumUpdateInterval() : 0;
}

void GPSPositionSourceAdapter::updatePosition(const QGeoPositionInfo& position)
{
    const QPointer<GPSPositionSourceAdapter> guard(this);
    const quint64 generation = _generation;
    emit backendError(QGeoPositionInfoSource::NoError);
    if (!guard || generation != _generation) {
        return;
    }
    GPSObservation observation;
    observation.position = position;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = GPSObservation::monotonicNowUs();
    observation.sourceId = _identity;
    _fallbackHealth.updateObservation(observation);
}
