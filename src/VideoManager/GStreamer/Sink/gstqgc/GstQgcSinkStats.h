#pragma once

#include <array>
#include <atomic>
#include <gst/gst.h>

#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QtTypes>

#include "GstScoped.h"
#include "HwBuffers/common/GstHwPathRegistry.h"
#include "HwBuffers/common/GstHwTypes.h"

struct MappedFrame;

namespace GstQgc {

/// guint64 "frame-stats" field holding the frames @p path delivered: "frames-" plus the lower-cased
/// GstHw::toString(path).
const char* sinkFramesField(HwVideoBufferPath path);

/// guint64 "frame-stats" field holding the delivered frames a newer frame replaced before the QVideoSink took them,
/// which were never shown.
inline constexpr const char* kSupersededFramesField = "superseded-frames";

/// guint64 "frame-stats" field holding the frames a GPU path was tried for but the CPU delivered.
inline constexpr const char* kGpuFallbacksField = "gpu-fallbacks";

/// guint64 "frame-stats" field holding the caps epochs that started on a GPU path and fell back to the CPU.
inline constexpr const char* kStreamDemotionsField = "stream-demotions";

/// guint64 "frame-stats" field holding the frames dropped because no GPU path imported them and the CPU can't read
/// their tiled DMA_DRM layout.
inline constexpr const char* kTiledDropsField = "tiled-drops";

/// The "frame-stats" of @p sinkOrBin, a qgcqvideosink or the sink bin around one; null for other elements.
GStreamer::GstStructurePtr frameStats(GstElement* sinkOrBin);

/// The guint64 @p field of @p stats, a "frame-stats" or any other structure; 0 without the structure or the field.
inline quint64 frameStatsField(const GstStructure* stats, const char* field) noexcept
{
    guint64 value = 0;
    if (stats) {
        (void) gst_structure_get_uint64(stats, field, &value);
    }
    return value;
}

/// How far the guint64 @p field grew from @p since to @p stats, two "frame-stats" values of the same sink.
inline quint64 frameStatsGrowth(const GstStructure* stats, const GstStructure* since, const char* field) noexcept
{
    const quint64 now = frameStatsField(stats, field);
    const quint64 before = frameStatsField(since, field);
    return (now > before) ? (now - before) : 0;
}

/// Formatted per-path frame counts + the total delivered through GPU paths.
struct PathStats
{
    QString line;
    quint64 totalDelivered = 0;
};

/// One qgcqvideosink's frame accounting, from its "frame-stats" structure: the per-path counts it has delivered, plus
/// its GPU fallbacks and demotions. With @p since (an earlier "frame-stats" value of the same sink), only what changed
/// since.
PathStats formatSinkStats(const GstStructure* stats, const GstStructure* since = nullptr);

/// \brief qgcqvideosink's frame accounting: what its frames-input, frames-dropped and frames-delivered properties
/// report, and the "frame-stats" structure the functions above read. Thread-safe.
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

    /// The "frame-stats" property value (transfer-full).
    GstStructure* frameStats() const;

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
    // Held while a delivery and the frame it superseded are counted, and while frameStats() reads them: its readers
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
