#include "GPSRelativePositionStore.h"

#include <algorithm>

#include "QGCLoggingCategory.h"
#include "QtRuntimeScheduler.h"

QGC_LOGGING_CATEGORY(GPSRelativePositionStoreLog, "GPS.Core.GPSRelativePositionStore")

GPSRelativePositionStore::GPSRelativePositionStore(QObject* parent, int freshnessTimeoutMs, RuntimeScheduler* scheduler)
    : QObject(parent)
    , _scheduler(scheduler ? scheduler : new QtRuntimeScheduler(this))
    , _expiryTask(_scheduler, this)
    , _freshnessTimeoutMs((std::max) (1, freshnessTimeoutMs))
{
    qCDebug(GPSRelativePositionStoreLog) << this;
    connect(_scheduler, &QObject::destroyed, this, [this]() {
        _scheduler = nullptr;
        _fresh = false;
        _publish();
    });
}

GPSRelativePositionStore::~GPSRelativePositionStore()
{
    qCDebug(GPSRelativePositionStoreLog) << this;
    if (_scheduler) {
        _scheduler->disconnect(this);
    }
}

void GPSRelativePositionStore::beginSession(const QString& sourceId, quint64 sessionId)
{
    if (_sourceId == sourceId && _sessionId == sessionId) {
        return;
    }
    _sourceId = sourceId;
    _sessionId = sessionId;
    _observation = {};
    _fresh = false;
    _expiryTask.cancel();
    _publish();
}

void GPSRelativePositionStore::reset()
{
    _sourceId.clear();
    _sessionId = 0;
    _observation = {};
    _fresh = false;
    _expiryTask.cancel();
    _publish();
}

void GPSRelativePositionStore::updateObservation(const GPSRelativeObservation& observation)
{
    if (!_scheduler) {
        return;
    }
    const quint64 nowUs = _scheduler->nowUs();
    if (_sourceId.isEmpty() || observation.sessionId != _sessionId || !observation.monotonicTimestampUs ||
        observation.monotonicTimestampUs > nowUs ||
        observation.monotonicTimestampUs < _observation.monotonicTimestampUs ||
        nowUs - observation.monotonicTimestampUs >= static_cast<quint64>(_freshnessTimeoutMs) * 1000) {
        return;
    }
    _observation = observation;
    _fresh = true;
    const auto remainingUs =
        static_cast<quint64>(_freshnessTimeoutMs) * 1000 - (nowUs - observation.monotonicTimestampUs);
    _expiryTask.schedule(std::chrono::microseconds(remainingUs), [this]() { _expire(); });
    _publish();
}

void GPSRelativePositionStore::_expire()
{
    if (!_fresh) {
        return;
    }
    _fresh = false;
    _publish();
}

void GPSRelativePositionStore::_publish()
{
    const auto snapshot = _observation;
    emit observationChanged(snapshot);
}
