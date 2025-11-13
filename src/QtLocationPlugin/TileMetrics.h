/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QUuid>

struct ProviderMetrics {
    int mapId = 0;
    QString name;
    quint64 totalRequests = 0;
    quint64 successfulRequests = 0;
    quint64 failedRequests = 0;
    quint64 totalResponseTimeMs = 0;
    quint64 totalBytesDownloaded = 0;
    QDateTime lastFailure;
    double healthScore = 1.0;  // 0.0 - 1.0

    double successRate() const {
        return totalRequests > 0 ? static_cast<double>(successfulRequests) / totalRequests : 0.0;
    }

    quint64 avgResponseTimeMs() const {
        return successfulRequests > 0 ? totalResponseTimeMs / successfulRequests : 0;
    }
};

struct CacheMetrics {
    quint64 totalHits = 0;
    quint64 totalMisses = 0;
    quint64 currentSizeBytes = 0;
    quint64 maxSizeBytes = 0;
    quint32 tileCount = 0;

    double hitRate() const {
        const quint64 total = totalHits + totalMisses;
        return total > 0 ? static_cast<double>(totalHits) / total : 0.0;
    }

    double usagePercent() const {
        return maxSizeBytes > 0 ? (static_cast<double>(currentSizeBytes) / maxSizeBytes) * 100.0 : 0.0;
    }
};

struct NetworkMetrics {
    quint64 totalBytesDownloaded = 0;
    quint64 totalBytesUploaded = 0;
    quint64 currentDownloadBytesPerSec = 0;
    quint64 currentUploadBytesPerSec = 0;
    quint32 activeConnections = 0;
    quint32 queuedRequests = 0;
    bool throttlingActive = false;
};

struct TileRequestTrace {
    QUuid requestId;
    int mapId;
    int x;
    int y;
    int zoom;
    QDateTime requestTime;
    QDateTime responseTime;
    bool success;
    int httpStatusCode;
    quint64 bytesReceived;
    QString errorMessage;

    qint64 durationMs() const {
        return requestTime.msecsTo(responseTime);
    }
};

struct TileMetricsSnapshot {
    QMap<int, ProviderMetrics> providerMetrics;  // mapId -> metrics
    CacheMetrics cacheMetrics;
    NetworkMetrics networkMetrics;
    QDateTime snapshotTime;

    ProviderMetrics aggregateProviderMetrics() const {
        ProviderMetrics aggregate;
        aggregate.name = "All Providers";
        for (const auto &pm : providerMetrics) {
            aggregate.totalRequests += pm.totalRequests;
            aggregate.successfulRequests += pm.successfulRequests;
            aggregate.failedRequests += pm.failedRequests;
            aggregate.totalResponseTimeMs += pm.totalResponseTimeMs;
            aggregate.totalBytesDownloaded += pm.totalBytesDownloaded;
        }
        return aggregate;
    }
};

Q_DECLARE_METATYPE(ProviderMetrics)
Q_DECLARE_METATYPE(CacheMetrics)
Q_DECLARE_METATYPE(NetworkMetrics)
Q_DECLARE_METATYPE(TileRequestTrace)
Q_DECLARE_METATYPE(TileMetricsSnapshot)
