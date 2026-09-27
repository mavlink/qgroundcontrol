#include "GPSPositionBackendAdapter.h"

#include <utility>

#include <QtCore/QDateTime>

#include "RuntimeScheduler.h"

GPSPositionBackendAdapter::GPSPositionBackendAdapter(QGeoPositionInfoSource* backend, const QString& identity,
                                                     bool platform, quint64 sessionId, RuntimeScheduler* scheduler,
                                                     QObject* parent)
    : QObject(parent)
    , _backend(backend)
    , _scheduler(scheduler)
    , _health(nullptr, scheduler)
    , _identity(identity)
    , _sessionId(sessionId)
    , _platform(platform)
{
    if (!_backend) {
        return;
    }
    _connections.append(connect(_backend, &QGeoPositionInfoSource::positionUpdated, this,
                                &GPSPositionBackendAdapter::_positionUpdated));
    _connections.append(
        connect(_backend, &QGeoPositionInfoSource::errorOccurred, this, &GPSPositionBackendAdapter::_errorOccurred));
}

GPSPositionBackendAdapter::~GPSPositionBackendAdapter()
{
    retire();
}

void GPSPositionBackendAdapter::retire()
{
    _eventHandler = {};
    for (const auto& connection : std::exchange(_connections, {})) {
        disconnect(connection);
    }
    _active = false;
    _eventRevision.invalidate();
    const QPointer<QGeoPositionInfoSource> backend = std::exchange(_backend, nullptr);
    if (std::exchange(_updatesStarted, false) && backend) {
        // Stopping may re-enter the owner or delete this adapter; nothing below touches members.
        backend->stopUpdates();
    }
}

void GPSPositionBackendAdapter::setActive(bool enabled)
{
    if (_active == enabled || !_backend) {
        return;
    }
    _active = enabled;
    const auto change = _eventRevision.advance(this);
    if (!enabled) {
        if (std::exchange(_updatesStarted, false)) {
            _backend->stopUpdates();
        }
        if (change.isCurrent()) {
            _health.reset();
        }
        return;
    }
    _backend->setPreferredPositioningMethods(QGeoPositionInfoSource::SatellitePositioningMethods);
    if (!change.isCurrent() || !_backend) {
        return;
    }
#if defined(Q_OS_DARWIN) || defined(Q_OS_IOS)
    if (!_platform) {
        _backend->setUpdateInterval(0);
    }
#else
    _backend->setUpdateInterval(updateInterval());
#endif
    if (change.isCurrent() && _backend) {
        _updatesStarted = true;
        _backend->startUpdates();
    }
}

int GPSPositionBackendAdapter::updateInterval() const
{
    return _platform && _backend ? _backend->minimumUpdateInterval() : 0;
}

bool GPSPositionBackendAdapter::_notify(QGeoPositionInfoSource::Error error)
{
    if (!_active) {
        return false;
    }
    const auto event = _eventRevision.advance(this);
    if (_eventHandler) {
        const auto handler = _eventHandler;
        handler(error);
    }
    return event.isCurrent() && _active && _backend;
}

void GPSPositionBackendAdapter::_positionUpdated(const QGeoPositionInfo& position)
{
    if (!_notify(QGeoPositionInfoSource::NoError)) {
        return;
    }
    GPSObservation observation;
    observation.position = position;
    observation.receivedAt = QDateTime::currentDateTimeUtc();
    observation.monotonicTimestampUs = _scheduler->nowUs();
    observation.sourceId = _identity;
    observation.sessionId = _sessionId;
    _health.updateObservation(observation);
}

void GPSPositionBackendAdapter::_errorOccurred(QGeoPositionInfoSource::Error error)
{
    // A timeout means no new fix, which freshness already expires; only real failures invalidate the fix.
    if (_notify(error) && error != QGeoPositionInfoSource::NoError &&
        error != QGeoPositionInfoSource::UpdateTimeoutError) {
        _health.invalidatePosition();
    }
}
