#pragma once

#include <gst/video/gstvideosink.h>

#include <QtCore/QSize>
#include <QtCore/QString>

#include "GstScoped.h"
#include "HwBuffers/common/GstHwTypes.h"

G_BEGIN_DECLS

#define GST_TYPE_QGC_Q_VIDEO_SINK (gst_qgc_q_video_sink_get_type())

/// \brief GstVideoSink that pushes decoded frames into a Qt QVideoSink; sole sink in qgcvideosinkbin.
/// Streaming-thread vfuncs MUST NOT touch QObject members directly (thread-affinity asserts) — hand
/// off via QMetaObject::invokeMethod / bus messages.
G_DECLARE_FINAL_TYPE(GstQgcQVideoSink, gst_qgc_q_video_sink, GST, QGC_Q_VIDEO_SINK, GstVideoSink)

G_END_DECLS

namespace GstQgc {

/// Name of the element message qgcqvideosink posts from set_caps with the negotiated raw caps, which are always fixed.
inline constexpr char kCapsInfoMessage[] = "qgc-caps-info";

struct CapsInfo
{
    QString format;  ///< GStreamer format name, e.g. "NV12".
    QSize size;
};

/// The negotiated format and size a kCapsInfoMessage carries.
CapsInfo parseCapsInfo(GstMessage* message);

/// guint64 "frame-stats" field holding the frames @p path delivered: "frames-" plus the lower-cased
/// GstHw::toString(path).
const char* sinkFramesField(HwVideoBufferPath path) noexcept;

/// The "frame-stats" of @p sinkOrBin, a qgcqvideosink or the qgcvideosinkbin around one; null for other elements.
GStreamer::GstStructurePtr frameStats(GstElement* sinkOrBin);

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
