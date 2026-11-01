#pragma once

#include <array>
#include <atomic>
#include <gst/gst.h>

#include <QtCore/QMutex>

#include "HwBuffers/common/GstHwTypes.h"

namespace GstQgc {

/// \brief qgcqvideosink's frame accounting: what its frames-input, frames-dropped and frames-delivered properties
/// report, and the "frame-stats" structure GstQgcSinkStats.h reads. Thread-safe.
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

}  // namespace GstQgc
