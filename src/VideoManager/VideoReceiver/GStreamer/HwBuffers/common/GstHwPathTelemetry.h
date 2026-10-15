#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/qglobal.h>

#include "GstHwTypes.h"

/// Per-path telemetry counters (map failures, reuse hits, sync waits) keyed by `HwVideoBufferPath`; atomics,
/// thread-safe.
namespace GstHwPathTelemetry {

/// Specific cause a HW path was rejected for a sample; surfaced as a per-(path,reason) breakdown.
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

/// mapTextures() returned an invalid bundle (GPU import failed).
void recordMapFailure(HwVideoBufferPath path) noexcept;
quint64 takeMapFailureCount(HwVideoBufferPath path) noexcept;
quint64 peekMapFailureCount(HwVideoBufferPath path) noexcept;

/// Prior frame's QRhiTexture wrappers reused (decoder pool returned same native handle).
void recordTextureReuse(HwVideoBufferPath path) noexcept;
quint64 takeTextureReuseHits(HwVideoBufferPath path) noexcept;

/// Wait on the producer's writes before sampling a frame; split CPU-blocking vs GPU-side.
void recordSyncWait(HwVideoBufferPath path, bool gpuSide) noexcept;
/// Reads-and-resets CPU waits; writes GPU waits into @p gpuWaits.
quint64 takeSyncWaitCounts(HwVideoBufferPath path, quint64& gpuWaits) noexcept;

/// Native image/texture cache hit/miss accounting.
void recordImageCacheHit(HwVideoBufferPath path) noexcept;
void recordImageCacheMiss(HwVideoBufferPath path) noexcept;
quint64 takeImageCacheHits(HwVideoBufferPath path) noexcept;
quint64 takeImageCacheMisses(HwVideoBufferPath path) noexcept;

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

/// DMABuf wait on the producer's write fences (poll() on the dma-buf fds) timed out or failed; the frame was imported
/// without a producer barrier.
void recordProducerFenceTimeout(HwVideoBufferPath path) noexcept;
quint64 takeProducerFenceTimeouts(HwVideoBufferPath path) noexcept;

/// Per-(path,reason) fallback accounting; lets a bug report show *why* a path demoted to CPU.
void recordFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;
quint64 peekFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;
quint64 takeFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;

}  // namespace GstHwPathTelemetry
