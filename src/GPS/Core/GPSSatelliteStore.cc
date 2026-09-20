#include "GPSSatelliteStore.h"

#include <algorithm>
#include <chrono>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSSatelliteStoreLog, "GPS.Core.GPSSatelliteStore")

GPSSatelliteStore::GPSSatelliteStore(QObject* parent, int freshnessTimeoutMs, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _state(freshnessTimeoutMs)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _expiryTask(_scheduler, this)
{
    qCDebug(GPSSatelliteStoreLog) << this;
    if (_scheduler->thread() != thread()) {
        qCWarning(GPSSatelliteStoreLog) << "Scheduler must share the store thread";
        _scheduler = nullptr;
        return;
    }
    connect(_scheduler, &QObject::destroyed, this, [this]() {
        _scheduler = nullptr;
        _state.reset();
        _publish();
    });
}

GPSSatelliteStore::~GPSSatelliteStore()
{
    qCDebug(GPSSatelliteStoreLog) << this;
    if (_scheduler) {
        _scheduler->disconnect(this);
    }
}

void GPSSatelliteStore::beginSession(const QString& sourceId, quint64 sessionId)
{
    if (_observation.sourceId == sourceId && _observation.sessionId == sessionId) {
        return;
    }
    _state.reset();
    _observation = {};
    _observation.sourceId = sourceId;
    _observation.sessionId = sessionId;
    _publish();
}

void GPSSatelliteStore::reset()
{
    _state.reset();
    _observation = {};
    _publish();
}

void GPSSatelliteStore::clear()
{
    _state.clear(_scheduler ? _scheduler->nowUs() : 0);
    _publish();
}

void GPSSatelliteStore::setFreshnessTimeoutMs(int timeoutMs)
{
    if (_state.freshnessTimeoutMs() != std::max(1, timeoutMs)) {
        _state.setFreshnessTimeoutMs(timeoutMs);
        _publish();
    }
}

void GPSSatelliteStore::updateObservation(const GPSSatelliteObservation& observation)
{
    if (!_scheduler || _observation.sourceId.isEmpty() || observation.sessionId != _observation.sessionId ||
        (!observation.sourceId.isEmpty() && observation.sourceId != _observation.sourceId)) {
        return;
    }
    _state.updateObservation(observation, _scheduler->nowUs());
    _publish();
}

void GPSSatelliteStore::_publish()
{
    const quint64 nowUs = _scheduler ? _scheduler->nowUs() : 0;
    auto snapshot = _state.snapshot(nowUs);
    snapshot.sourceId = _observation.sourceId;
    snapshot.sessionId = _observation.sessionId;
    snapshot.revision = ++_revision;
    _observation = snapshot;
    _expiryTask.cancel();
    if (const auto deadline = _state.nextExpiryUs()) {
        _expiryTask.schedule(std::chrono::microseconds(*deadline > nowUs ? *deadline - nowUs : 0),
                             [this]() { _publish(); });
    }
    emit observationChanged(snapshot);
}
