#include "GstHwPathTelemetry.h"

#include <array>
#include <atomic>

namespace GstHwPathTelemetry {

namespace {

constexpr size_t kPathCount = size_t(HwVideoBufferPath::Vulkan) + 1;

struct PathCounters
{
    std::atomic<quint64> mapFailures{0};
    std::atomic<quint64> textureReuseHits{0};
    std::atomic<quint64> syncCpuWaits{0};
    std::atomic<quint64> syncGpuWaits{0};
    std::atomic<quint64> imageCacheHits{0};
    std::atomic<quint64> imageCacheMisses{0};
    std::atomic<quint64> delivered{0};
    std::atomic<quint64> mapDurationUsEwma{0};
    std::atomic<quint64> fenceTimeouts{0};
    std::atomic<quint64> mmapBarrierHits{0};
    std::atomic<quint64> explicitFenceWaits{0};
    std::atomic<quint64> streamDemotions{0};
};

std::array<PathCounters, kPathCount> s_counters{};

constexpr size_t kReasonCount = size_t(HwFallbackReason::_Count);
std::array<std::array<std::atomic<quint64>, kReasonCount>, kPathCount> s_fallbackReasons{};

inline PathCounters& slot(HwVideoBufferPath path) noexcept
{
    return s_counters[size_t(path)];
}

}  // namespace

void recordMapFailure(HwVideoBufferPath path) noexcept
{
    slot(path).mapFailures.fetch_add(1, std::memory_order_relaxed);
}

quint64 takeMapFailureCount(HwVideoBufferPath path) noexcept
{
    return slot(path).mapFailures.exchange(0, std::memory_order_relaxed);
}

quint64 peekMapFailureCount(HwVideoBufferPath path) noexcept
{
    return slot(path).mapFailures.load(std::memory_order_relaxed);
}

void recordTextureReuse(HwVideoBufferPath path) noexcept
{
    slot(path).textureReuseHits.fetch_add(1, std::memory_order_relaxed);
}

quint64 takeTextureReuseHits(HwVideoBufferPath path) noexcept
{
    return slot(path).textureReuseHits.exchange(0, std::memory_order_relaxed);
}

void recordSyncWait(HwVideoBufferPath path, bool gpuSide) noexcept
{
    auto& target = gpuSide ? slot(path).syncGpuWaits : slot(path).syncCpuWaits;
    target.fetch_add(1, std::memory_order_relaxed);
}

quint64 takeSyncWaitCounts(HwVideoBufferPath path, quint64& gpuWaits) noexcept
{
    gpuWaits = slot(path).syncGpuWaits.exchange(0, std::memory_order_relaxed);
    return slot(path).syncCpuWaits.exchange(0, std::memory_order_relaxed);
}

void recordImageCacheHit(HwVideoBufferPath path) noexcept
{
    slot(path).imageCacheHits.fetch_add(1, std::memory_order_relaxed);
}

void recordImageCacheMiss(HwVideoBufferPath path) noexcept
{
    slot(path).imageCacheMisses.fetch_add(1, std::memory_order_relaxed);
}

quint64 takeImageCacheHits(HwVideoBufferPath path) noexcept
{
    return slot(path).imageCacheHits.exchange(0, std::memory_order_relaxed);
}

quint64 takeImageCacheMisses(HwVideoBufferPath path) noexcept
{
    return slot(path).imageCacheMisses.exchange(0, std::memory_order_relaxed);
}

#if defined(QGC_HAS_ANY_GPU_PATH)
QVideoFrameTexturesUPtr fail(HwVideoBufferPath path) noexcept
{
    recordMapFailure(path);
    return {};
}
#endif

void recordDelivered(HwVideoBufferPath path) noexcept
{
    slot(path).delivered.fetch_add(1, std::memory_order_relaxed);
}

quint64 peekDeliveredCount(HwVideoBufferPath path) noexcept
{
    return slot(path).delivered.load(std::memory_order_relaxed);
}

quint64 takeDeliveredCount(HwVideoBufferPath path) noexcept
{
    return slot(path).delivered.exchange(0, std::memory_order_relaxed);
}

void recordMapDuration(HwVideoBufferPath path, qint64 nsecs) noexcept
{
    if (nsecs < 0) {
        return;
    }
    // EWMA in microseconds (alpha = 1/8); seed on the first sample so the average tracks immediately.
    const quint64 sampleUs = static_cast<quint64>(nsecs) / 1000;
    auto& ewma = slot(path).mapDurationUsEwma;
    quint64 prev = ewma.load(std::memory_order_relaxed);
    quint64 next;
    do {
        next = (prev == 0) ? sampleUs : prev - (prev >> 3) + (sampleUs >> 3);
    } while (!ewma.compare_exchange_weak(prev, next, std::memory_order_relaxed));
}

quint64 peekMapDurationUsEwma(HwVideoBufferPath path) noexcept
{
    return slot(path).mapDurationUsEwma.load(std::memory_order_relaxed);
}

void recordFenceTimeout(HwVideoBufferPath path) noexcept
{
    slot(path).fenceTimeouts.fetch_add(1, std::memory_order_relaxed);
}

quint64 peekFenceTimeouts(HwVideoBufferPath path) noexcept
{
    return slot(path).fenceTimeouts.load(std::memory_order_relaxed);
}

quint64 takeFenceTimeouts(HwVideoBufferPath path) noexcept
{
    return slot(path).fenceTimeouts.exchange(0, std::memory_order_relaxed);
}

void recordMmapBarrierHit(HwVideoBufferPath path) noexcept
{
    slot(path).mmapBarrierHits.fetch_add(1, std::memory_order_relaxed);
}

quint64 peekMmapBarrierHits(HwVideoBufferPath path) noexcept
{
    return slot(path).mmapBarrierHits.load(std::memory_order_relaxed);
}

quint64 takeMmapBarrierHits(HwVideoBufferPath path) noexcept
{
    return slot(path).mmapBarrierHits.exchange(0, std::memory_order_relaxed);
}

void recordExplicitFenceWait(HwVideoBufferPath path) noexcept
{
    slot(path).explicitFenceWaits.fetch_add(1, std::memory_order_relaxed);
}

quint64 takeExplicitFenceWaits(HwVideoBufferPath path) noexcept
{
    return slot(path).explicitFenceWaits.exchange(0, std::memory_order_relaxed);
}

void recordFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept
{
    s_fallbackReasons[size_t(attemptedPath)][size_t(reason)].fetch_add(1, std::memory_order_relaxed);
}

quint64 peekFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept
{
    return s_fallbackReasons[size_t(attemptedPath)][size_t(reason)].load(std::memory_order_relaxed);
}

quint64 takeFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept
{
    return s_fallbackReasons[size_t(attemptedPath)][size_t(reason)].exchange(0, std::memory_order_relaxed);
}

void recordStreamDemotion(HwVideoBufferPath negotiated) noexcept
{
    slot(negotiated).streamDemotions.fetch_add(1, std::memory_order_relaxed);
}

quint64 takeStreamDemotions(HwVideoBufferPath negotiated) noexcept
{
    return slot(negotiated).streamDemotions.exchange(0, std::memory_order_relaxed);
}

}  // namespace GstHwPathTelemetry
