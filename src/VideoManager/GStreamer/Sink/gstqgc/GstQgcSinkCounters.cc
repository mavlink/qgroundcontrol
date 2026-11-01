#include "GstQgcSinkCounters.h"

#include <QtCore/QMutexLocker>

#include "GstQgcSinkStats.h"

namespace GstQgc {

quint64 SinkCounters::noteDelivered(HwVideoBufferPath path, bool superseded)
{
    {
        const QMutexLocker locker(&_deliveryMutex);
        (void) _pathFrames[static_cast<std::size_t>(path)].fetch_add(1, std::memory_order_relaxed);
        if (superseded) {
            (void) _superseded.fetch_add(1, std::memory_order_relaxed);
        }
    }
    return _delivered.fetch_add(1, std::memory_order_relaxed) + 1;
}

void SinkCounters::noteGpuFallback(bool firstOfEpoch)
{
    (void) _gpuFallbacks.fetch_add(1, std::memory_order_relaxed);
    if (firstOfEpoch) {
        (void) _streamDemotions.fetch_add(1, std::memory_order_relaxed);
    }
}

GstStructure* SinkCounters::frameStats() const
{
    GstStructure* const stats = gst_structure_new_empty("qgc-video-sink-stats");
    gst_structure_set(stats, kGpuFallbacksField, G_TYPE_UINT64,
                      static_cast<guint64>(_gpuFallbacks.load(std::memory_order_relaxed)), kStreamDemotionsField,
                      G_TYPE_UINT64, static_cast<guint64>(_streamDemotions.load(std::memory_order_relaxed)), nullptr);
    gst_structure_set(stats, kTiledDropsField, G_TYPE_UINT64,
                      static_cast<guint64>(_tiledDrops.load(std::memory_order_relaxed)), nullptr);
    {
        const QMutexLocker locker(&_deliveryMutex);
        for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
            gst_structure_set(stats, sinkFramesField(HwVideoBufferPath(i)), G_TYPE_UINT64,
                              static_cast<guint64>(_pathFrames[i].load(std::memory_order_relaxed)), nullptr);
        }
        gst_structure_set(stats, kSupersededFramesField, G_TYPE_UINT64,
                          static_cast<guint64>(_superseded.load(std::memory_order_relaxed)), nullptr);
    }
    return stats;
}

}  // namespace GstQgc
