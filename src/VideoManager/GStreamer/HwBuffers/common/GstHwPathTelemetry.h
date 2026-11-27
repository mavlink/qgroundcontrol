#pragma once

#include <array>

#include <QtCore/QElapsedTimer>
#include <QtCore/qglobal.h>

#include "GstHwTypes.h"

/// Per-path telemetry counters (map failures, sync waits) keyed by `HwVideoBufferPath`; atomics,
/// thread-safe. Counters only grow: readers diff against their own Snapshot, so several receivers never consume each
/// other's counts.
namespace GstHwPathTelemetry {

/// Specific cause a GPU path was rejected for a sample; surfaced as a per-(path,reason) breakdown.
enum class HwFallbackReason
{
    NoExt,
    EglBadMatch,
    ValidateFailed,
    UnknownMemType,
    MapFailed,
    VulkanNoSync,
    ImportUnsupported,
    FdDupFailed,  ///< A frame's dma-buf fd could not be duplicated for the import: no fault of the driver's.
    Count,
};

/// Per-path event counts.
enum class Counter
{
    MapFailures,  ///< mapTextures() returned no textures (GPU import failed).
    GpuWaits,     ///< GPU-side waits on the producer's writes before sampling a frame.
    CpuWaits,     ///< CPU-blocking waits on the producer's writes before sampling a frame.
    /// DMABuf wait on the producer's write fences (poll() on the dma-buf fds) timed out or failed; the frame was
    /// imported without a producer barrier.
    ProducerFenceTimeouts,
    Count,
};

void record(HwVideoBufferPath path, Counter counter) noexcept;

/// Per-path mapTextures() wall-time, fed into an EWMA; peek returns the smoothed value in microseconds.
void recordMapDuration(HwVideoBufferPath path, qint64 nsecs) noexcept;
quint64 peekMapDurationUsEwma(HwVideoBufferPath path) noexcept;

/// RAII timer: records mapTextures() wall time into the path's EWMA on scope exit.
class ScopedMapTimer
{
public:
    explicit ScopedMapTimer(HwVideoBufferPath path) noexcept
        : _path(path)
    {
        _timer.start();
    }

    ~ScopedMapTimer() noexcept { recordMapDuration(_path, _timer.nsecsElapsed()); }

    ScopedMapTimer(const ScopedMapTimer&) = delete;
    ScopedMapTimer& operator=(const ScopedMapTimer&) = delete;

private:
    HwVideoBufferPath _path;
    QElapsedTimer _timer;
};

/// Per-(path,reason) fallback accounting; lets a bug report show *why* a path demoted to CPU.
void recordFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;

/// Every counter and fallback reason at one instant; a reader's baseline for counts since then.
struct Snapshot
{
    std::array<std::array<quint64, std::size_t(Counter::Count)>, GstHw::kPathCount> counters{};
    std::array<std::array<quint64, std::size_t(HwFallbackReason::Count)>, GstHw::kPathCount> fallbackReasons{};

    quint64 count(HwVideoBufferPath path, Counter counter) const noexcept
    {
        return counters[std::size_t(path)][std::size_t(counter)];
    }

    quint64 count(HwVideoBufferPath path, HwFallbackReason reason) const noexcept
    {
        return fallbackReasons[std::size_t(path)][std::size_t(reason)];
    }
};

Snapshot snapshot() noexcept;

}  // namespace GstHwPathTelemetry
