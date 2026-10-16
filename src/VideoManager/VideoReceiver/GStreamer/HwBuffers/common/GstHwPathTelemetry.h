#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/qglobal.h>

#include "GstHwTypes.h"

/// Per-path telemetry counters (map failures, reuse hits, sync waits) keyed by `HwVideoBufferPath`; atomics,
/// thread-safe.
namespace GstHwPathTelemetry {

/// Specific cause a GPU path was rejected for a sample; surfaced as a per-(path,reason) breakdown.
enum class HwFallbackReason
{
    NoExt,
    ModifierRejected,
    EglBadMatch,
    ValidateFailed,
    UnknownMemType,
    MapFailed,
    VulkanNoSync,
    ImportUnsupported,
    _Count,
};

/// Per-path event counts.
enum class Counter
{
    MapFailures,   ///< mapTextures() returned an invalid bundle (GPU import failed).
    TextureReuse,  ///< Prior frame's QRhiTexture wrappers reused (decoder pool returned same native handle).
    GpuWaits,      ///< GPU-side waits on the producer's writes before sampling a frame.
    CpuWaits,      ///< CPU-blocking waits on the producer's writes before sampling a frame.
    ImageCacheHits,
    ImageCacheMisses,
    /// DMABuf wait on the producer's write fences (poll() on the dma-buf fds) timed out or failed; the frame was
    /// imported without a producer barrier.
    ProducerFenceTimeouts,
    _Count,
};

void record(HwVideoBufferPath path, Counter counter) noexcept;
quint64 peek(HwVideoBufferPath path, Counter counter) noexcept;
/// Reads and resets.
quint64 take(HwVideoBufferPath path, Counter counter) noexcept;

/// Per-path mapTextures() wall-time, fed into an EWMA; peek returns the smoothed value in microseconds.
void recordMapDuration(HwVideoBufferPath path, qint64 nsecs) noexcept;
quint64 peekMapDurationUsEwma(HwVideoBufferPath path) noexcept;

/// RAII timer: records mapTextures() wall time into the path's EWMA on scope exit.
class ScopedMapTimer
{
public:
    explicit ScopedMapTimer(HwVideoBufferPath path) noexcept : _path(path) { _timer.start(); }
    ~ScopedMapTimer() noexcept { recordMapDuration(_path, _timer.nsecsElapsed()); }
    ScopedMapTimer(const ScopedMapTimer&) = delete;
    ScopedMapTimer& operator=(const ScopedMapTimer&) = delete;

private:
    HwVideoBufferPath _path;
    QElapsedTimer _timer;
};

/// Per-(path,reason) fallback accounting; lets a bug report show *why* a path demoted to CPU.
void recordFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;
quint64 peekFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;
quint64 takeFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;

}  // namespace GstHwPathTelemetry
