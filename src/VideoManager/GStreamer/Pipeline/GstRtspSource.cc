#include "GstRtspSource.h"

#include <chrono>
#include <gst/rtsp/gstrtsptransport.h>
#include <gst/rtsp/gstrtspurl.h>

#include <QtCore/QUrl>

#include "QGCNetworkHelper.h"

namespace GStreamer::SourceFactory {

namespace {

// rtspsrc gives up on a TCP connection (RTSP control, or interleaved data) that stays silent this long.
constexpr guint64 kRtspTcpTimeoutUs = G_GUINT64_CONSTANT(5000000);

// Fixed rtx timing instead of the auto(-1) heuristic, which can stack retries and balloon playout
// latency: bounded recovery latency over a lossy RF link.
constexpr int kRtxDelayMs = 25;
constexpr int kRtxMaxRetries = 1;

void configureRtspJitterBuffer([[maybe_unused]] GstElement* manager, GstElement* jitterBuffer,
                               [[maybe_unused]] guint session, [[maybe_unused]] guint ssrc,
                               [[maybe_unused]] gpointer data)
{
    g_object_set(jitterBuffer, "rtx-delay", kRtxDelayMs, "rtx-max-retries", kRtxMaxRetries, nullptr);
}

void connectRtspJitterBufferPolicy([[maybe_unused]] GstElement* rtspsrc, GstElement* manager,
                                   [[maybe_unused]] gpointer data)
{
    // rtspsrc picks rtpdec instead of rtpbin for RealMedia; only rtpbin exposes its jitterbuffers.
    if (g_signal_lookup("new-jitterbuffer", G_OBJECT_TYPE(manager)) == 0) {
        return;
    }
    (void) g_signal_connect(manager, "new-jitterbuffer", G_CALLBACK(configureRtspJitterBuffer), nullptr);
}

// Runs before SETUP, so skipped SDP streams cost no bandwidth and can't claim the video parser. The caps are
// still named application/x-unknown here (rtspsrc renames them to the transport's media type only after
// SETUP), so only the SDP media type decides.
gboolean selectVideoStream([[maybe_unused]] GstElement* rtspsrc, guint num, GstCaps* caps,
                           [[maybe_unused]] gpointer data)
{
    if (caps && (gst_caps_get_size(caps) > 0) && isVideoMedia(gst_caps_get_structure(caps, 0))) {
        return TRUE;
    }

    qCDebug(GstSourceFactoryLog) << "Skipping non-video RTSP stream" << num;
    return FALSE;
}

}  // namespace

bool isValidRtspUri(const gchar* uri_str)
{
    if (!uri_str) {
        return false;
    }

    if (!gst_uri_is_valid(uri_str)) {
        return false;
    }

    GstRTSPUrl* url = nullptr;
    const GstRTSPResult res = gst_rtsp_url_parse(uri_str, &url);
    if ((res != GST_RTSP_OK) || !url) {
        if (url) {
            gst_rtsp_url_free(url);
        }
        return false;
    }

    const bool hasHost = (url->host && url->host[0] != '\0');
    gst_rtsp_url_free(url);
    return hasHost;
}

bool isVideoMedia(const GstStructure* structure)
{
    return g_strcmp0(gst_structure_get_string(structure, "media"), "video") == 0;
}

GstElementPtr buildRtspSource(const QString& uri, const QUrl& sourceUrl, const Config& config, guint latencyMs)
{
    if (!isValidRtspUri(uri.toUtf8().constData())) {
        qCWarning(GstSourceFactoryLog) << "Invalid RTSP URI:" << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }

    GstElementPtr source = makeElement("rtspsrc", "source");
    if (!source) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('rtspsrc') failed";
        return nullptr;
    }

    QUrl cleanUrl(sourceUrl);
    cleanUrl.setUserInfo(QString());
    const QByteArray cleanLocation = cleanUrl.toEncoded();

    // Unicast UDP, then TCP-interleaved: rtspsrc retries over TCP when no UDP data arrives within "timeout". The
    // default would also try UDP multicast, which a GCS link never uses.
    constexpr GstRTSPLowerTrans kRtspProtocols =
        static_cast<GstRTSPLowerTrans>(GST_RTSP_LOWER_TRANS_UDP | GST_RTSP_LOWER_TRANS_TCP);
    constexpr guint64 kRtspUdpTimeoutUs = std::chrono::microseconds(kRtspUdpTimeout).count();

    // rtspsrc always owns an internal jitterbuffer, so None maps to zero playout latency and no retransmission.
    const guint rtspLatencyMs = (config.jitterBuffer == JitterBuffer::None) ? 0u : latencyMs;
    const gboolean doRetransmission =
        ((config.jitterBuffer != JitterBuffer::None) && config.doRetransmission) ? TRUE : FALSE;
    g_object_set(source.get(), "location", cleanLocation.constData(), "latency", rtspLatencyMs, "do-rtcp", TRUE,
                 "do-retransmission", doRetransmission, "tcp-timeout", kRtspTcpTimeoutUs, "udp-reconnect", TRUE,
                 "drop-on-latency", TRUE, "timeout", kRtspUdpTimeoutUs, "protocols", kRtspProtocols, nullptr);

    // rtspsrc forwards latency/drop-on-latency/do-retransmission to its rtpbin, but not the rtx timing.
    if (doRetransmission) {
        (void) g_signal_connect(source.get(), "new-manager", G_CALLBACK(connectRtspJitterBufferPolicy), nullptr);
    }
    (void) g_signal_connect(source.get(), "select-stream", G_CALLBACK(selectVideoStream), nullptr);

    const QString rtspUser = sourceUrl.userName(QUrl::FullyDecoded);
    const QString rtspPassword = sourceUrl.password(QUrl::FullyDecoded);
    if (!rtspUser.isEmpty()) {
        g_object_set(source.get(), "user-id", rtspUser.toUtf8().constData(), "user-pw",
                     rtspPassword.toUtf8().constData(), nullptr);
    }
    return source;
}

}  // namespace GStreamer::SourceFactory
