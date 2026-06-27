#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/qglobal.h>

#include "GstHwVideoBufferFactory.h"  // HwVideoBufferPath enum

#if defined(QGC_HAS_ANY_GPU_PATH)
#include <private/qhwvideobuffer_p.h>
#endif

/// Per-path telemetry counters (map failures, reuse hits, sync waits) keyed by `HwVideoBufferPath`; atomics,
/// thread-safe.
namespace GstHwPathTelemetry {

/// Specific cause a HW path was rejected for a sample; surfaced as a per-(path,reason) breakdown.
enum class HwFallbackReason
{
    None,
    NoExt,
    ModifierRejected,
    EglBadMatch,
    FenceTimeout,
    ValidateFailed,
    UnknownMemType,
    NullSample,
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

/// GL fence sync wait; split CPU-blocking vs GPU-side.
void recordSyncWait(HwVideoBufferPath path, bool gpuSide) noexcept;
/// Reads-and-resets CPU waits; writes GPU waits into @p gpuWaits.
quint64 takeSyncWaitCounts(HwVideoBufferPath path, quint64& gpuWaits) noexcept;

/// Native image/texture cache hit/miss accounting.
void recordImageCacheHit(HwVideoBufferPath path) noexcept;
void recordImageCacheMiss(HwVideoBufferPath path) noexcept;
quint64 takeImageCacheHits(HwVideoBufferPath path) noexcept;
quint64 takeImageCacheMisses(HwVideoBufferPath path) noexcept;

#if defined(QGC_HAS_ANY_GPU_PATH)
/// Single-shot helper for the common "increment-and-return-empty" pattern.
QVideoFrameTexturesUPtr fail(HwVideoBufferPath path) noexcept;
#endif

/// Frames successfully delivered via this path.
void recordDelivered(HwVideoBufferPath path) noexcept;
quint64 peekDeliveredCount(HwVideoBufferPath path) noexcept;
quint64 takeDeliveredCount(HwVideoBufferPath path) noexcept;

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

/// DMABuf EGL fence wait timed out (GPU stall) and fell through to the mmap barrier.
void recordFenceTimeout(HwVideoBufferPath path) noexcept;
quint64 peekFenceTimeouts(HwVideoBufferPath path) noexcept;
quint64 takeFenceTimeouts(HwVideoBufferPath path) noexcept;

/// DMABuf mmap CPU-side completion barrier taken (no usable fence ext).
void recordMmapBarrierHit(HwVideoBufferPath path) noexcept;
quint64 peekMmapBarrierHits(HwVideoBufferPath path) noexcept;
quint64 takeMmapBarrierHits(HwVideoBufferPath path) noexcept;

/// DMABuf imported the producer's dma-buf/native fence and did a GPU-side wait (skipped the mmap barrier).
void recordExplicitFenceWait(HwVideoBufferPath path) noexcept;
quint64 takeExplicitFenceWaits(HwVideoBufferPath path) noexcept;

/// Per-(path,reason) fallback accounting; lets a bug report show *why* a path demoted to CPU.
void recordFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;
quint64 peekFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;
quint64 takeFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;

/// One-shot-per-epoch event: a stream that negotiated a HW path demoted to CPU. Distinct from per-frame CPU counts.
void recordStreamDemotion(HwVideoBufferPath negotiated) noexcept;
quint64 takeStreamDemotions(HwVideoBufferPath negotiated) noexcept;

}  // namespace GstHwPathTelemetry
