#include "GstQgcSinkStats.h"

#include <array>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMutexLocker>

#include "GstQgcFrameMap.h"
#include "GstQgcSinkBin.h"
#include "gstqgcqvideosink.h"

Q_DECLARE_LOGGING_CATEGORY(GstQgcQVideoSinkLog)

namespace GstQgc {

const char* sinkFramesField(HwVideoBufferPath path)
{
    static const std::array<QByteArray, GstHw::kPathCount> s_fields = [] {
        std::array<QByteArray, GstHw::kPathCount> fields;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            fields[i] = QByteArray("frames-") + QByteArray(GstHw::toString(HwVideoBufferPath(i))).toLower();
        }
        return fields;
    }();
    const auto index = static_cast<std::size_t>(path);
    return (index < s_fields.size()) ? s_fields[index].constData() : nullptr;
}

GStreamer::GstStructurePtr frameStats(GstElement* sinkOrBin)
{
    GStreamer::GstElementPtr inner;
    if (sinkOrBin && GST_IS_BIN(sinkOrBin)) {
        inner = GStreamer::adoptElement(GstQgc::sinkBinVideoSink(sinkOrBin));
        sinkOrBin = inner.get();
    }
    if (!sinkOrBin || !GST_IS_QGC_Q_VIDEO_SINK(sinkOrBin)) {
        return {};
    }
    GstStructure* stats = nullptr;
    g_object_get(sinkOrBin, "frame-stats", &stats, nullptr);
    return GStreamer::adoptStructure(stats);
}

PathStats formatSinkStats(const GstStructure* stats, const GstStructure* since)
{
    PathStats out;
    if (!stats) {
        return out;
    }
    for (std::size_t i = 0; i < GstHw::kPathCount; ++i) {
        const auto path = HwVideoBufferPath(i);
        if (const quint64 frames = frameStatsGrowth(stats, since, sinkFramesField(path)); frames > 0) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(GstHw::toString(path))).arg(frames);
            if (path != HwVideoBufferPath::None) {
                out.totalDelivered += frames;
            }
        }
    }
    for (const char* const field : {kGpuFallbacksField, kStreamDemotionsField, kTiledDropsField}) {
        if (const quint64 count = frameStatsGrowth(stats, since, field); count > 0) {
            out.line += QStringLiteral(" %1:%2").arg(QLatin1String(field)).arg(count);
        }
    }
    return out;
}

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

void PathSession::startEpoch()
{
    _pathCache = HwResolvedPathCache{};
    _demotionRecorded = false;
    _tiledDropLogged = false;
}

void PathSession::stop()
{
    startEpoch();
    _consecutiveMapFailures = 0;
}

PathSession::Verdict PathSession::onMapFailed(const MappedFrame& mapped, SinkCounters& counters)
{
    const quint64 failures = ++_consecutiveMapFailures;
    if (mapped.noCpuLayout) {
        // Until upstream renegotiates (the sink asked for it when the DMABuf import stopped), or the run escalates.
        counters.noteTiledDrop();
        if (!std::exchange(_tiledDropLogged, true)) {
            qCWarning(GstQgcQVideoSinkLog) << "tiled DMA_DRM frame not imported and has no CPU layout — dropped";
        }
    } else if ((failures & 0x3F) == 1) {
        qCWarning(GstQgcQVideoSinkLog) << "mapping failed, consecutive=" << failures;
    }
    if (failures >= kMaxConsecutiveMapFailures) {
        qCWarning(GstQgcQVideoSinkLog) << failures << "consecutive map failures — erroring out";
        return Verdict::Error;
    }
    return Verdict::Drop;
}

void PathSession::onDelivered(const MappedFrame& mapped, SinkCounters& counters)
{
    if (mapped.demoted) {
        counters.noteGpuFallback(!std::exchange(_demotionRecorded, true));
    }
}

}  // namespace GstQgc
