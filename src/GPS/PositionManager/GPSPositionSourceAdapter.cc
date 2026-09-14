#include "GPSPositionSourceAdapter.h"

#include <QtCore/QThread>

#include <utility>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSPositionSourceAdapterLog, "GPS.PositionManager.GPSPositionSourceAdapter")

GPSPositionSourceAdapter::GPSPositionSourceAdapter(QObject* parent, RuntimeScheduler* scheduler)
    : QObject(parent),
      _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this)),
      _fallbackHealth(this, _scheduler)
{
    qCDebug(GPSPositionSourceAdapterLog) << this;
    if (_scheduler && _scheduler->thread() != thread()) {
        _scheduler = nullptr;
    }
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
    _active = false;
    const bool updatesStarted = std::exchange(_updatesStarted, false);
    if (_source && updatesStarted) {
        _source->stopUpdates();
    }
}

void GPSPositionSourceAdapter::configure(QObject* producer, GPSSourceHealth* health, const QString& identity,
                                         bool platform, quint64 sessionId)
{
    if (QThread::currentThread() != thread() || (producer && producer->thread() != thread()) ||
        (health && health->thread() != thread()) || (producer && !_scheduler)) {
        qCWarning(GPSPositionSourceAdapterLog) << "Position source requires matching thread affinity and a scheduler";
        return;
    }
    if (_producer == producer && _providedHealth == health && _identity == identity && _platform == platform &&
        _sessionId == sessionId) {
        return;
    }
    const QPointer<GPSPositionSourceAdapter> guard(this);
    const quint64 generation = ++_generation;
    const QPointer<QObject> producerGuard(producer);
    const QPointer<GPSSourceHealth> healthGuard(health);
    _disconnectSource();
    if (!guard || generation != _generation) {
        return;
    }
    _producer = producerGuard;
    _source = qobject_cast<QGeoPositionInfoSource*>(producerGuard.data());
    _providedHealth = healthGuard;
    _identity = identity;
    _platform = platform;
    _sessionId = sessionId;
    _fallbackHealth.reset();
    if (!guard || generation != _generation || !_producer) {
        return;
    }
    _connections.append(connect(_producer, &QObject::destroyed, this, [this]() { emit bindingChanged(); }));
    if (_providedHealth) {
        _connections.append(connect(_providedHealth, &GPSSourceHealth::positionChanged, this,
                                    &GPSPositionSourceAdapter::observationChanged));
        _connections.append(connect(_providedHealth, &QObject::destroyed, this, [this]() {
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
                                        _updatePosition(position);
                                    }
                                }));
    _connections.append(connect(
        _source, &QGeoPositionInfoSource::errorOccurred, this, [this, generation](QGeoPositionInfoSource::Error error) {
            if (_generation != generation || !_active || _providedHealth) {
                return;
            }
            const QPointer<GPSPositionSourceAdapter> errorGuard(this);
            const quint64 revision = ++_backendRevision;
            emit backendError(error);
            if (errorGuard && _generation == generation && revision == _backendRevision && _active && _source &&
                !_providedHealth && error != QGeoPositionInfoSource::NoError &&
                error != QGeoPositionInfoSource::UpdateTimeoutError) {
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
    ++_backendRevision;
    if (_providedHealth || !_source) {
        return;
    }
    const QPointer<GPSPositionSourceAdapter> guard(this);
    const quint64 generation = _generation;
    if (!active) {
        if (std::exchange(_updatesStarted, false)) {
            _source->stopUpdates();
        }
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
        _updatesStarted = true;
        _source->startUpdates();
    }
}

int GPSPositionSourceAdapter::updateInterval() const
{
    return _platform && _source ? _source->minimumUpdateInterval() : 0;
}

void GPSPositionSourceAdapter::_updatePosition(const QGeoPositionInfo& position)
{
    const QPointer<GPSPositionSourceAdapter> guard(this);
    const quint64 generation = _generation;
    const quint64 revision = ++_backendRevision;
    emit backendError(QGeoPositionInfoSource::NoError);
    if (!guard || generation != _generation || revision != _backendRevision || !_active || !_source ||
        _providedHealth) {
        return;
    }
    GPSObservation observation;
    observation.position = position;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = _scheduler ? _scheduler->nowUs() : 0;
    observation.sourceId = _identity;
    observation.sessionId = _sessionId;
    _fallbackHealth.updateObservation(observation);
}
