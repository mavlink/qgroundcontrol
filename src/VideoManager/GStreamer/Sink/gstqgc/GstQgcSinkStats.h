#pragma once

#include <array>
#include <atomic>

#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include "HwBuffers/HwBuffers.h"

struct MappedFrame;

namespace GstQgc {

/// One qgcqvideosink's frame accounting (sinkStats()), or how far it grew between two of them (operator-()).
struct SinkStats
{
    /// Buffers show_frame saw, dropped ones included.
    quint64 input = 0;
    /// Buffers show_frame dropped: the sink was inactive or had no QVideoSink, or the frame didn't map.
    quint64 dropped = 0;
    /// Frames handed to the QVideoSink, superseded ones included.
    quint64 delivered = 0;
    /// The frames each path delivered, by HwVideoBufferPath (None: the CPU).
    std::array<quint64, GstHw::kPathCount> pathFrames{};
    /// Frames a GPU path was tried for but the CPU delivered.
    quint64 gpuFallbacks = 0;
    /// Caps epochs that started on a GPU path and fell back to the CPU.
    quint64 streamDemotions = 0;
    /// Frames dropped because no GPU path imported them and the CPU can't read their tiled DMA_DRM layout.
    quint64 tiledDrops = 0;
    /// Delivered frames a newer one replaced before the QVideoSink took them, which were never shown.
    quint64 superseded = 0;

    quint64 frames(HwVideoBufferPath path) const { return pathFrames[static_cast<std::size_t>(path)]; }

    /// The delivered frames the QVideoSink took: those of every path less the superseded ones.
    quint64 displayed() const;

    /// How far each count grew since @p since, an earlier value of the same sink.
    SinkStats operator-(const SinkStats& since) const;
};

/// Formatted per-path frame counts + the total delivered through GPU paths.
struct PathStats
{
    QString line;
    quint64 totalDelivered = 0;
};

/// @p stats formatted: the per-path counts, then the GPU fallbacks, stream demotions and tiled drops, each where it
/// isn't 0.
PathStats formatSinkStats(const SinkStats& stats);

/// \brief qgcqvideosink's frame accounting, counted on its streaming thread and read as a SinkStats. Thread-safe.
class SinkCounters
{
public:
    void noteInput() { (void) _input.fetch_add(1, std::memory_order_relaxed); }

    void noteDropped() { (void) _dropped.fetch_add(1, std::memory_order_relaxed); }

    /// A frame reached the QVideoSink through @p path (None for the CPU), replacing one it hadn't taken yet when
    /// @p superseded. Returns the frames delivered so far.
    quint64 noteDelivered(HwVideoBufferPath path, bool superseded);
    /// A GPU path was tried for a frame the CPU delivered; @p firstOfEpoch: the caps epoch's first, a stream demotion.
    void noteGpuFallback(bool firstOfEpoch);

    /// A frame in a tiled DMA_DRM layout no GPU path imported and the CPU can't read.
    void noteTiledDrop() { (void) _tiledDrops.fetch_add(1, std::memory_order_relaxed); }

    quint64 input() const { return _input.load(std::memory_order_relaxed); }

    quint64 dropped() const { return _dropped.load(std::memory_order_relaxed); }

    quint64 delivered() const { return _delivered.load(std::memory_order_relaxed); }

    quint64 pathFrames(HwVideoBufferPath path) const
    {
        return _pathFrames[static_cast<std::size_t>(path)].load(std::memory_order_relaxed);
    }

    SinkStats snapshot() const;

private:
    std::atomic<quint64> _input{0};
    std::atomic<quint64> _dropped{0};
    // Per element, so receivers that each have a sink don't see a shared total.
    std::atomic<quint64> _delivered{0};
    std::array<std::atomic<quint64>, GstHw::kPathCount> _pathFrames{};
    std::atomic<quint64> _gpuFallbacks{0};
    std::atomic<quint64> _streamDemotions{0};
    std::atomic<quint64> _tiledDrops{0};
    std::atomic<quint64> _superseded{0};
    // Held while a delivery and the frame it superseded are counted, and while snapshot() reads them: its readers
    // count the frames displayed as those delivered less those superseded, which a snapshot seeing one count without
    // the other would make fall.
    mutable QMutex _deliveryMutex;
};

/// \brief What qgcqvideosink keeps about the path its frames take through one caps epoch, from set_caps to the next
/// set_caps or stop: the GPU path it resolved, and what is once per epoch, a stream demotion and the tiled-drop
/// warning. Also decides what becomes of a frame that fails to map: dropped, or after a sustained run of failures, an
/// error that ends the stream.
///
/// Threading: the sink's streaming thread; GstBaseSink serializes set_caps, show_frame and stop.
class PathSession
{
public:
    /// A run of map failures this long is a broken import, not a hiccup: the stream fails so it is set up anew.
    static constexpr quint64 kMaxConsecutiveMapFailures = 120;

    enum class Verdict
    {
        Drop,   ///< Drop the frame and go on.
        Error,  ///< Fail the stream.
    };

    /// New caps: the path is resolved anew, and the once-per-epoch bookkeeping starts over.
    void startEpoch();
    /// The sink stopped: as startEpoch(), and a run of map failures ends.
    void stop();

    /// For mapSampleToFrame(), which caches the path it resolves in the epoch.
    HwResolvedPathCache* pathCache() { return &_pathCache; }

    /// @p mapped, which has no frame, failed to map. Counts and logs it, and returns Error once the run of failures
    /// reaches kMaxConsecutiveMapFailures.
    Verdict onMapFailed(const MappedFrame& mapped, SinkCounters& counters);

    /// The frame mapped: a run of failures ends.
    void onMapped() { _consecutiveMapFailures = 0; }

    quint64 consecutiveMapFailures() const { return _consecutiveMapFailures; }

    /// @p mapped reached the QVideoSink. A frame a GPU path was tried for but the CPU delivered counts as a GPU
    /// fallback, the epoch's first also as a stream demotion.
    void onDelivered(const MappedFrame& mapped, SinkCounters& counters);

private:
    HwResolvedPathCache _pathCache = {};
    bool _demotionRecorded = false;
    bool _tiledDropLogged = false;
    quint64 _consecutiveMapFailures = 0;
};

}  // namespace GstQgc
