#include "GstQgcSinkStats.h"

#include <array>

#include <QtCore/QByteArray>

#include "gstqgcqvideosink.h"
#include "gstqgcvideosinkbin.h"

namespace GstQgc {

const char* sinkFramesField(HwVideoBufferPath path) noexcept
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
    if (sinkOrBin && GST_IS_QGC_VIDEO_SINK_BIN(sinkOrBin)) {
        inner = GStreamer::adoptElement(gst_qgc_video_sink_bin_get_qvideosink(GST_QGC_VIDEO_SINK_BIN(sinkOrBin)));
        sinkOrBin = inner.get();
    }
    if (!sinkOrBin || !GST_IS_QGC_Q_VIDEO_SINK(sinkOrBin)) {
        return {};
    }
    GstStructure* stats = nullptr;
    g_object_get(sinkOrBin, "frame-stats", &stats, nullptr);
    return GStreamer::adoptStructure(stats);
}

PathStats formatSinkStats(const GstStructure* stats, const GstStructure* since) noexcept
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

}  // namespace GstQgc
