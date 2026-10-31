#pragma once

#include <gst/gst.h>

#include <QtCore/QString>

#include "GstScoped.h"
#include "HwBuffers/common/GstHwTypes.h"

namespace GstQgc {

/// guint64 "frame-stats" field holding the frames @p path delivered: "frames-" plus the lower-cased
/// GstHw::toString(path).
const char* sinkFramesField(HwVideoBufferPath path) noexcept;

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

/// The "frame-stats" of @p sinkOrBin, a qgcqvideosink or the qgcvideosinkbin around one; null for other elements.
GStreamer::GstStructurePtr frameStats(GstElement* sinkOrBin);

/// The guint64 @p field of a "frame-stats" structure; 0 without the structure or the field.
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
PathStats formatSinkStats(const GstStructure* stats, const GstStructure* since = nullptr) noexcept;

}  // namespace GstQgc
