#pragma once

#include <array>
#include <atomic>
#include <gst/gst.h>

#include <QtCore/QElapsedTimer>
#include <QtCore/qglobal.h>

#include "GstHwTypes.h"

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

/// DMABuf wait on the producer's write fences (poll() on the dma-buf fds) timed out or failed; the frame was imported
/// without a producer barrier.
void recordProducerFenceTimeout(HwVideoBufferPath path) noexcept;
quint64 takeProducerFenceTimeouts(HwVideoBufferPath path) noexcept;

/// DMABuf waited on the CPU for the producer's write fences (poll() on the dma-buf fds) because no GPU-side wait was
/// available.
void recordCpuBarrierWait(HwVideoBufferPath path) noexcept;
quint64 takeCpuBarrierWaits(HwVideoBufferPath path) noexcept;

/// DMABuf imported the producer's dma-buf/native fence and did a GPU-side wait (skipped the CPU barrier).
void recordExplicitFenceWait(HwVideoBufferPath path) noexcept;
quint64 takeExplicitFenceWaits(HwVideoBufferPath path) noexcept;

/// Per-(path,reason) fallback accounting; lets a bug report show *why* a path demoted to CPU.
void recordFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;
quint64 peekFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;
quint64 takeFallbackReason(HwVideoBufferPath attemptedPath, HwFallbackReason reason) noexcept;

/// One-shot-per-epoch event: a stream that negotiated a HW path demoted to CPU. Distinct from per-frame CPU counts.
void recordStreamDemotion(HwVideoBufferPath negotiated) noexcept;
quint64 takeStreamDemotions(HwVideoBufferPath negotiated) noexcept;

/// Frame accounting for one sink. The process-wide counters above mix every stream; these let each receiver report
/// only its own frames. qgcqvideosink exposes them through its "frame-stats" property.
struct SinkCounters
{
    std::array<std::atomic<quint64>, GstHw::kPathCount> delivered{};  ///< per path; None counts CPU frames
    std::atomic<quint64> gpuFallbacks{0};     ///< frames a GPU path was tried for but delivered through the CPU
    std::atomic<quint64> streamDemotions{0};  ///< caps epochs that started on a GPU path and fell back to the CPU
};

/// guint64 stats field holding the frames @p path delivered: "frames-" plus the lower-cased GstHw::toString(path).
const char* sinkFramesField(HwVideoBufferPath path) noexcept;

/// Adds @p counters to @p stats: sinkFramesField() for every path, plus "gpu-fallbacks" and "stream-demotions".
void appendSinkCounters(GstStructure* stats, const SinkCounters& counters) noexcept;

}  // namespace GstHwPathTelemetry
