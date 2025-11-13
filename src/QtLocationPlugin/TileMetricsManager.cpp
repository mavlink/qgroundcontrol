/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "TileMetricsManager.h"

#include <QtCore/QSettings>

QGC_LOGGING_CATEGORY(TileMetricsManagerLog, "qgc.qtlocationplugin.tilemetricsmanager")

TileMetricsManager::TileMetricsManager(QObject *parent)
    : QObject(parent)
{
    _updateTimer = new QTimer(this);
    connect(_updateTimer, &QTimer::timeout, this, &TileMetricsManager::metricsChanged);
    _updateTimer->start(kUpdateIntervalMs);

    _bandwidthTimer.start();

    loadFromPersistentStorage();
}

TileMetricsManager* TileMetricsManager::instance()
{
    static TileMetricsManager *_instance = nullptr;
    if (!_instance) {
        _instance = new TileMetricsManager();
    }
    return _instance;
}

QUuid TileMetricsManager::startTrace(int mapId, int x, int y, int zoom)
{
    QMutexLocker locker(&_mutex);

    TileRequestTrace trace;
    trace.requestId = QUuid::createUuid();
    trace.mapId = mapId;
    trace.x = x;
    trace.y = y;
    trace.zoom = zoom;
    trace.requestTime = QDateTime::currentDateTime();
    trace.success = false;

    _activeTraces[trace.requestId] = trace;

    return trace.requestId;
}

void TileMetricsManager::completeTrace(const QUuid &requestId, bool success, int httpStatusCode, quint64 bytesReceived, const QString &errorMessage)
{
    QMutexLocker locker(&_mutex);

    auto it = _activeTraces.find(requestId);
    if (it == _activeTraces.end()) {
        qCWarning(TileMetricsManagerLog) << "Unknown trace ID:" << requestId;
        return;
    }

    TileRequestTrace trace = it.value();
    trace.responseTime = QDateTime::currentDateTime();
    trace.success = success;
    trace.httpStatusCode = httpStatusCode;
    trace.bytesReceived = bytesReceived;
    trace.errorMessage = errorMessage;

    const qint64 durationMs = trace.durationMs();

    updateProviderMetrics(trace.mapId, success, durationMs, bytesReceived);

    _completedTraces.append(trace);
    _activeTraces.erase(it);

    if (_completedTraces.size() > kTracesPruneThreshold) {
        pruneOldTraces();
    }

    emit metricsChanged();
    emit providerMetricsChanged(trace.mapId);
}

void TileMetricsManager::recordTileRequest(int mapId, int x, int y, int zoom)
{
    startTrace(mapId, x, y, zoom);
}

void TileMetricsManager::recordTileSuccess(const QUuid &requestId, int httpStatusCode, quint64 bytesReceived, qint64 durationMs)
{
    Q_UNUSED(durationMs)
    completeTrace(requestId, true, httpStatusCode, bytesReceived);
}

void TileMetricsManager::recordTileFailure(const QUuid &requestId, const QString &errorMessage)
{
    completeTrace(requestId, false, 0, 0, errorMessage);
}

void TileMetricsManager::updateProviderMetrics(int mapId, bool success, qint64 durationMs, quint64 bytesReceived)
{
    ProviderMetrics &metrics = _providerMetrics[mapId];
    if (metrics.mapId == 0) {
        metrics.mapId = mapId;
    }

    metrics.totalRequests++;
    if (success) {
        metrics.successfulRequests++;
        metrics.totalResponseTimeMs += durationMs;
        metrics.totalBytesDownloaded += bytesReceived;
    } else {
        metrics.failedRequests++;
        metrics.lastFailure = QDateTime::currentDateTime();
    }

    calculateProviderHealth(metrics);
}

void TileMetricsManager::calculateProviderHealth(ProviderMetrics &metrics)
{
    // Health = success rate * 0.7 + recency factor * 0.3
    const double successRate = metrics.successRate();

    double recencyFactor = 1.0;
    if (metrics.lastFailure.isValid()) {
        const qint64 msSinceFailure = metrics.lastFailure.msecsTo(QDateTime::currentDateTime());
        constexpr qint64 kFailureDecayMs = 60000;  // 1 minute
        recencyFactor = qMin(1.0, static_cast<double>(msSinceFailure) / kFailureDecayMs);
    }

    metrics.healthScore = successRate * 0.7 + recencyFactor * 0.3;
}

void TileMetricsManager::recordCacheHit()
{
    QMutexLocker locker(&_mutex);
    _cacheMetrics.totalHits++;
    emit cacheMetricsChanged();
}

void TileMetricsManager::recordCacheMiss()
{
    QMutexLocker locker(&_mutex);
    _cacheMetrics.totalMisses++;
    emit cacheMetricsChanged();
}

void TileMetricsManager::updateCacheSize(quint64 sizeBytes, quint32 tileCount)
{
    QMutexLocker locker(&_mutex);
    _cacheMetrics.currentSizeBytes = sizeBytes;
    _cacheMetrics.tileCount = tileCount;
    emit cacheMetricsChanged();
}

void TileMetricsManager::updateNetworkStats(quint64 downloadBytesPerSec, quint64 uploadBytesPerSec, quint32 activeConnections, quint32 queuedRequests)
{
    QMutexLocker locker(&_mutex);
    _networkMetrics.currentDownloadBytesPerSec = downloadBytesPerSec;
    _networkMetrics.currentUploadBytesPerSec = uploadBytesPerSec;
    _networkMetrics.activeConnections = activeConnections;
    _networkMetrics.queuedRequests = queuedRequests;
    emit networkMetricsChanged();
}

void TileMetricsManager::setThrottlingActive(bool active)
{
    QMutexLocker locker(&_mutex);
    _networkMetrics.throttlingActive = active;
    emit networkMetricsChanged();
}

TileMetricsSnapshot TileMetricsManager::snapshot() const
{
    QMutexLocker locker(&_mutex);
    TileMetricsSnapshot snap;
    snap.providerMetrics = _providerMetrics;
    snap.cacheMetrics = _cacheMetrics;
    snap.networkMetrics = _networkMetrics;
    snap.snapshotTime = QDateTime::currentDateTime();
    return snap;
}

ProviderMetrics TileMetricsManager::providerMetrics(int mapId) const
{
    QMutexLocker locker(&_mutex);
    return _providerMetrics.value(mapId);
}

double TileMetricsManager::overallSuccessRate() const
{
    QMutexLocker locker(&_mutex);
    const ProviderMetrics aggregate = snapshot().aggregateProviderMetrics();
    return aggregate.successRate();
}

quint64 TileMetricsManager::overallAvgResponseTimeMs() const
{
    QMutexLocker locker(&_mutex);
    const ProviderMetrics aggregate = snapshot().aggregateProviderMetrics();
    return aggregate.avgResponseTimeMs();
}

TileRequestTrace TileMetricsManager::getTrace(const QUuid &requestId) const
{
    QMutexLocker locker(&_mutex);
    return _activeTraces.value(requestId);
}

QList<TileRequestTrace> TileMetricsManager::recentTraces(int maxCount) const
{
    QMutexLocker locker(&_mutex);
    const int start = qMax(0, _completedTraces.size() - maxCount);
    return _completedTraces.mid(start);
}

void TileMetricsManager::pruneOldTraces()
{
    while (_completedTraces.size() > kMaxTraceHistory) {
        _completedTraces.removeFirst();
    }
}

void TileMetricsManager::saveToPersistentStorage()
{
    QSettings settings;
    settings.beginGroup("TileMetrics");

    settings.setValue("cacheHits", _cacheMetrics.totalHits);
    settings.setValue("cacheMisses", _cacheMetrics.totalMisses);

    settings.beginWriteArray("providers", _providerMetrics.size());
    int idx = 0;
    for (const auto &metrics : _providerMetrics) {
        settings.setArrayIndex(idx++);
        settings.setValue("mapId", metrics.mapId);
        settings.setValue("totalRequests", metrics.totalRequests);
        settings.setValue("successfulRequests", metrics.successfulRequests);
        settings.setValue("failedRequests", metrics.failedRequests);
        settings.setValue("totalBytesDownloaded", metrics.totalBytesDownloaded);
    }
    settings.endArray();

    settings.endGroup();
}

void TileMetricsManager::loadFromPersistentStorage()
{
    QSettings settings;
    settings.beginGroup("TileMetrics");

    _cacheMetrics.totalHits = settings.value("cacheHits", 0ULL).toULongLong();
    _cacheMetrics.totalMisses = settings.value("cacheMisses", 0ULL).toULongLong();

    const int size = settings.beginReadArray("providers");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        ProviderMetrics metrics;
        metrics.mapId = settings.value("mapId").toInt();
        metrics.totalRequests = settings.value("totalRequests", 0ULL).toULongLong();
        metrics.successfulRequests = settings.value("successfulRequests", 0ULL).toULongLong();
        metrics.failedRequests = settings.value("failedRequests", 0ULL).toULongLong();
        metrics.totalBytesDownloaded = settings.value("totalBytesDownloaded", 0ULL).toULongLong();

        if (metrics.mapId > 0) {
            _providerMetrics[metrics.mapId] = metrics;
        }
    }
    settings.endArray();

    settings.endGroup();
}

void TileMetricsManager::reset()
{
    QMutexLocker locker(&_mutex);
    _providerMetrics.clear();
    _activeTraces.clear();
    _completedTraces.clear();
    _cacheMetrics = CacheMetrics();
    _networkMetrics = NetworkMetrics();

    emit metricsChanged();
    emit cacheMetricsChanged();
    emit networkMetricsChanged();
}
