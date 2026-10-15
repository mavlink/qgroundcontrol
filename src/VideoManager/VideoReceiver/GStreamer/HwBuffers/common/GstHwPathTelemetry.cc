#include "GstHwPathTelemetry.h"

#include <array>
#include <atomic>

namespace GstHwPathTelemetry {

namespace {

constexpr size_t kCounterCount = size_t(Counter::_Count);
std::array<std::array<std::atomic<quint64>, kCounterCount>, GstHw::kPathCount> s_counters{};

std::array<std::atomic<quint64>, GstHw::kPathCount> s_mapDurationUsEwma{};

constexpr size_t kReasonCount = size_t(HwFallbackReason::_Count);
std::array<std::array<std::atomic<quint64>, kReasonCount>, GstHw::kPathCount> s_fallbackReasons{};

}  // namespace

void record(HwVideoBufferPath path, Counter counter) noexcept
{
    s_counters[size_t(path)][size_t(counter)].fetch_add(1, std::memory_order_relaxed);
}

quint64 peek(HwVideoBufferPath path, Counter counter) noexcept
{
    return s_counters[size_t(path)][size_t(counter)].load(std::memory_order_relaxed);
}

quint64 take(HwVideoBufferPath path, Counter counter) noexcept
{
    return s_counters[size_t(path)][size_t(counter)].exchange(0, std::memory_order_relaxed);
}

void recordMapDuration(HwVideoBufferPath path, qint64 nsecs) noexcept
{
    if (nsecs < 0) {
        return;
    }
    // EWMA in microseconds (alpha = 1/8); seed on the first sample so the average tracks immediately.
    const quint64 sampleUs = static_cast<quint64>(nsecs) / 1000;
    auto& ewma = s_mapDurationUsEwma[size_t(path)];
    quint64 prev = ewma.load(std::memory_order_relaxed);
    quint64 next;
    do {
        next = (prev == 0) ? sampleUs : prev - (prev >> 3) + (sampleUs >> 3);
    } while (!ewma.compare_exchange_weak(prev, next, std::memory_order_relaxed));
}

quint64 peekMapDurationUsEwma(HwVideoBufferPath path) noexcept
{
    return s_mapDurationUsEwma[size_t(path)].load(std::memory_order_relaxed);
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

}  // namespace GstHwPathTelemetry
