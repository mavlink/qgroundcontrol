/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "TileMetrics.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QTimer>

Q_DECLARE_LOGGING_CATEGORY(TileMetricsManagerLog)

class TileMetricsManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(CacheMetrics cacheMetrics READ cacheMetrics NOTIFY cacheMetricsChanged)
    Q_PROPERTY(NetworkMetrics networkMetrics READ networkMetrics NOTIFY networkMetricsChanged)
    Q_PROPERTY(double overallSuccessRate READ overallSuccessRate NOTIFY metricsChanged)
    Q_PROPERTY(quint64 overallAvgResponseTimeMs READ overallAvgResponseTimeMs NOTIFY metricsChanged)
    Q_PROPERTY(double cacheHitRate READ cacheHitRate NOTIFY cacheMetricsChanged)

public:
    static TileMetricsManager* instance();

    // Metrics recording
    void recordTileRequest(int mapId, int x, int y, int zoom);
    void recordTileSuccess(const QUuid &requestId, int httpStatusCode, quint64 bytesReceived, qint64 durationMs);
    void recordTileFailure(const QUuid &requestId, const QString &errorMessage);
    void recordCacheHit();
    void recordCacheMiss();
    void updateCacheSize(quint64 sizeBytes, quint32 tileCount);
    void updateNetworkStats(quint64 downloadBytesPerSec, quint64 uploadBytesPerSec, quint32 activeConnections, quint32 queuedRequests);
    void setThrottlingActive(bool active);

    // Metrics retrieval
    TileMetricsSnapshot snapshot() const;
    ProviderMetrics providerMetrics(int mapId) const;
    CacheMetrics cacheMetrics() const { QMutexLocker locker(&_mutex); return _cacheMetrics; }
    NetworkMetrics networkMetrics() const { QMutexLocker locker(&_mutex); return _networkMetrics; }

    // Aggregates for QML
    double overallSuccessRate() const;
    quint64 overallAvgResponseTimeMs() const;
    double cacheHitRate() const { return cacheMetrics().hitRate(); }

    // Persistence
    void saveToPersistentStorage();
    void loadFromPersistentStorage();
    void reset();

    // Request tracing
    QUuid startTrace(int mapId, int x, int y, int zoom);
    void completeTrace(const QUuid &requestId, bool success, int httpStatusCode, quint64 bytesReceived, const QString &errorMessage = QString());
    TileRequestTrace getTrace(const QUuid &requestId) const;
    QList<TileRequestTrace> recentTraces(int maxCount = 100) const;

signals:
    void metricsChanged();
    void cacheMetricsChanged();
    void networkMetricsChanged();
    void providerMetricsChanged(int mapId);

private:
    explicit TileMetricsManager(QObject *parent = nullptr);

    void updateProviderMetrics(int mapId, bool success, qint64 durationMs, quint64 bytesReceived);
    void calculateProviderHealth(ProviderMetrics &metrics);
    void pruneOldTraces();

    mutable QMutex _mutex;
    QMap<int, ProviderMetrics> _providerMetrics;  // mapId -> metrics
    QMap<QUuid, TileRequestTrace> _activeTraces;
    QList<TileRequestTrace> _completedTraces;
    CacheMetrics _cacheMetrics;
    NetworkMetrics _networkMetrics;
    QTimer *_updateTimer = nullptr;
    QElapsedTimer _bandwidthTimer;
    quint64 _lastBytesDownloaded = 0;

    static constexpr int kUpdateIntervalMs = 1000;  // Update metrics every second
    static constexpr int kMaxTraceHistory = 1000;
    static constexpr int kTracesPruneThreshold = 1200;
};
