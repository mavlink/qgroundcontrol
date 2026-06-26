#include "TileFetchMetrics.h"

QString TileFetchStats::toString() const
{
    return QStringLiteral("tiles=%1 cacheHits=%2 cacheMisses=%3 net=%4 errors=%5 notModified=%6 bytes=%7 avgLatencyMs=%8")
        .arg(totalFetches())
        .arg(cacheHits)
        .arg(cacheMisses)
        .arg(networkSuccess)
        .arg(networkErrors)
        .arg(notModified)
        .arg(bytesDownloaded)
        .arg(averageLatencyMs(), 0, 'f', 1);
}

TileFetchMetrics &TileFetchMetrics::instance()
{
    static TileFetchMetrics s_instance;
    return s_instance;
}

void TileFetchMetrics::recordCacheHit()
{
    _cacheHits.fetch_add(1, std::memory_order_relaxed);
}

void TileFetchMetrics::recordCacheMiss()
{
    _cacheMisses.fetch_add(1, std::memory_order_relaxed);
}

void TileFetchMetrics::recordNotModified(quint64 latencyMs)
{
    _notModified.fetch_add(1, std::memory_order_relaxed);
    _totalLatencyMs.fetch_add(latencyMs, std::memory_order_relaxed);
    _latencySamples.fetch_add(1, std::memory_order_relaxed);
}

void TileFetchMetrics::recordNetworkSuccess(quint64 bytes, quint64 latencyMs)
{
    _networkSuccess.fetch_add(1, std::memory_order_relaxed);
    _bytesDownloaded.fetch_add(bytes, std::memory_order_relaxed);
    _totalLatencyMs.fetch_add(latencyMs, std::memory_order_relaxed);
    _latencySamples.fetch_add(1, std::memory_order_relaxed);
}

void TileFetchMetrics::recordNetworkError(quint64 latencyMs)
{
    _networkErrors.fetch_add(1, std::memory_order_relaxed);
    _totalLatencyMs.fetch_add(latencyMs, std::memory_order_relaxed);
    _latencySamples.fetch_add(1, std::memory_order_relaxed);
}

TileFetchStats TileFetchMetrics::snapshot() const
{
    TileFetchStats s;
    s.cacheHits = _cacheHits.load(std::memory_order_relaxed);
    s.cacheMisses = _cacheMisses.load(std::memory_order_relaxed);
    s.networkSuccess = _networkSuccess.load(std::memory_order_relaxed);
    s.networkErrors = _networkErrors.load(std::memory_order_relaxed);
    s.notModified = _notModified.load(std::memory_order_relaxed);
    s.bytesDownloaded = _bytesDownloaded.load(std::memory_order_relaxed);
    s.totalLatencyMs = _totalLatencyMs.load(std::memory_order_relaxed);
    s.latencySamples = _latencySamples.load(std::memory_order_relaxed);
    return s;
}

void TileFetchMetrics::reset()
{
    _cacheHits.store(0, std::memory_order_relaxed);
    _cacheMisses.store(0, std::memory_order_relaxed);
    _networkSuccess.store(0, std::memory_order_relaxed);
    _networkErrors.store(0, std::memory_order_relaxed);
    _notModified.store(0, std::memory_order_relaxed);
    _bytesDownloaded.store(0, std::memory_order_relaxed);
    _totalLatencyMs.store(0, std::memory_order_relaxed);
    _latencySamples.store(0, std::memory_order_relaxed);
}
