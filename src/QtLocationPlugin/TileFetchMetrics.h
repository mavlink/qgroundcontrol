#pragma once

#include <QtCore/QString>

#include <atomic>
#include <cstdint>

// Lightweight, lock-free counters for tile-fetch outcomes. Plain struct snapshot
// for reads (no Q_GADGET/QML exposure) plus a process-wide aggregator updated
// from any thread the fetch/reply paths run on.
struct TileFetchStats {
    quint64 cacheHits = 0;
    quint64 cacheMisses = 0;
    quint64 networkSuccess = 0;
    quint64 networkErrors = 0;
    quint64 notModified = 0;
    quint64 bytesDownloaded = 0;
    quint64 totalLatencyMs = 0;
    quint64 latencySamples = 0;

    double averageLatencyMs() const {
        return (latencySamples > 0) ? (static_cast<double>(totalLatencyMs) / static_cast<double>(latencySamples)) : 0.0;
    }
    quint64 totalFetches() const { return cacheHits + networkSuccess + networkErrors + notModified; }
    QString toString() const;
};

class TileFetchMetrics
{
public:
    static TileFetchMetrics &instance();

    void recordCacheHit();
    void recordCacheMiss();
    void recordNotModified(quint64 latencyMs);
    void recordNetworkSuccess(quint64 bytes, quint64 latencyMs);
    void recordNetworkError(quint64 latencyMs);

    TileFetchStats snapshot() const;
    void reset();

private:
    TileFetchMetrics() = default;

    std::atomic<quint64> _cacheHits{0};
    std::atomic<quint64> _cacheMisses{0};
    std::atomic<quint64> _networkSuccess{0};
    std::atomic<quint64> _networkErrors{0};
    std::atomic<quint64> _notModified{0};
    std::atomic<quint64> _bytesDownloaded{0};
    std::atomic<quint64> _totalLatencyMs{0};
    std::atomic<quint64> _latencySamples{0};
};
