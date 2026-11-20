#include "GstRtspSource.h"

#include <chrono>
#include <cstring>
#include <gst/rtsp/gstrtspmessage.h>
#include <gst/rtsp/gstrtsptransport.h>
#include <gst/rtsp/gstrtspurl.h>
#include <gst/sdp/gstsdpmessage.h>

#include <QtCore/QUrl>

#include "QGCNetworkHelper.h"

namespace GStreamer::SourceFactory {

namespace {

// How long rtspsrc waits to connect and for the answer to each request (20 s by default). It doesn't end a stream
// whose TCP-interleaved data stops: there rtspsrc only sends a keep-alive when the time is up, and it is the
// receiver's watchdog that notices the stall.
constexpr guint64 kRtspTcpTimeoutUs = G_GUINT64_CONSTANT(5000000);

// How long stopping waits for the TEARDOWN to be sent and answered, where rtspsrc's 100 ms cuts it short on a slow
// link and leaves the camera streaming until the session times out. A camera that is gone holds each stop up for this
// long, which has to stay well within the 2 s a receiver being destroyed gives its pipeline to stop.
constexpr guint64 kRtspTeardownTimeoutNs = GST_SECOND;

// Fixed rtx timing instead of the auto(-1) heuristic, which can stack retries and balloon playout
// latency: bounded recovery latency over a lossy RF link.
constexpr int kRtxDelayMs = 25;
// One retry after the first request: at most two requests for a packet.
constexpr int kRtxMaxRetries = 1;

/// @p data: non-null to set the retransmission timing.
void configureRtspJitterBuffer([[maybe_unused]] GstElement* manager, GstElement* jitterBuffer,
                               [[maybe_unused]] guint session, [[maybe_unused]] guint ssrc, gpointer data)
{
    reportDroppedPackets(jitterBuffer);
    if (data) {
        g_object_set(jitterBuffer, "rtx-delay", kRtxDelayMs, "rtx-max-retries", kRtxMaxRetries, nullptr);
    }
}

/// @p data: passed on to configureRtspJitterBuffer().
void connectRtspJitterBufferPolicy([[maybe_unused]] GstElement* rtspsrc, GstElement* manager, gpointer data)
{
    // rtspsrc picks rtpdec instead of rtpbin for RealMedia; only rtpbin exposes its jitterbuffers.
    if (g_signal_lookup("new-jitterbuffer", G_OBJECT_TYPE(manager)) == 0) {
        return;
    }
    (void) g_signal_connect(manager, "new-jitterbuffer", G_CALLBACK(configureRtspJitterBuffer), data);
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

// rtpbin sends a keyframe request (an RTCP PLI) only for a stream whose caps say the sender takes one, which they do
// when its SDP has "a=rtcp-fb: nack pli". Few cameras' SDP has it, also among those that answer a PLI, and the others
// ignore the packet. Without it the requests of the depayloader, the decoder and the recorder end in rtspsrc.
void advertiseKeyframeRequests([[maybe_unused]] GstElement* rtspsrc, GstSDPMessage* sdp, [[maybe_unused]] gpointer data)
{
    const guint mediaCount = gst_sdp_message_medias_len(sdp);
    for (guint i = 0; i < mediaCount; ++i) {
        // The message is rtspsrc's to change here, though its accessor is for reading.
        auto* const media = const_cast<GstSDPMedia*>(gst_sdp_message_get_media(sdp, i));
        if (g_strcmp0(gst_sdp_media_get_media(media), "video") != 0) {
            continue;
        }
        bool takesPli = false;
        for (guint n = 0; !takesPli; ++n) {
            const gchar* const feedback = gst_sdp_media_get_attribute_val_n(media, "rtcp-fb", n);
            if (!feedback) {
                break;
            }
            takesPli = std::strstr(feedback, "nack pli") != nullptr;
        }
        if (!takesPli) {
            (void) gst_sdp_media_add_attribute(media, "rtcp-fb", "* nack pli");
        }
    }
}

// A pipeline passes through PAUSED on its way to NULL, where rtspsrc sends a PAUSE before the TEARDOWN that ends the
// session anyway, and some live cameras answer a PAUSE with an error. The receiver never pauses a stream, so that is
// the only PAUSE there would be.
gboolean skipPauseRequest([[maybe_unused]] GstElement* rtspsrc, GstRTSPMessage* request, [[maybe_unused]] gpointer data)
{
    const bool pause =
        (request->type == GST_RTSP_MESSAGE_REQUEST) && (request->type_data.request.method == GST_RTSP_PAUSE);
    return pause ? FALSE : TRUE;
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
    // rtspsrc's own 512 KiB is more than a stock Linux lets a socket ask for, which udpsrc warns about, and less than
    // the factory's other UDP sockets get where the kernel allows more.
    g_object_set(source.get(), "udp-buffer-size", udpReceiveBufferSize(), nullptr);
    // Over TCP rtspsrc stamps only the first packet with its arrival time and derives the others from their RTP
    // timestamps, so the camera's clock drifts against ours until, with a short latency, every packet is late. With
    // this (GStreamer 1.24.10) each packet gets its arrival time, as over UDP, and the jitter buffer evens out the
    // drift.
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(source.get()), "tcp-timestamp")) {
        g_object_set(source.get(), "tcp-timestamp", TRUE, nullptr);
    }

    // rtspsrc forwards latency/drop-on-latency/do-retransmission to its rtpbin, but neither the rtx timing nor the
    // messages about dropped packets.
    (void) g_signal_connect(source.get(), "new-manager", G_CALLBACK(connectRtspJitterBufferPolicy),
                            doRetransmission ? GINT_TO_POINTER(TRUE) : nullptr);
    (void) g_signal_connect(source.get(), "select-stream", G_CALLBACK(selectVideoStream), nullptr);
    (void) g_signal_connect(source.get(), "on-sdp", G_CALLBACK(advertiseKeyframeRequests), nullptr);
    (void) g_signal_connect(source.get(), "before-send", G_CALLBACK(skipPauseRequest), nullptr);
    g_object_set(source.get(), "teardown-timeout", kRtspTeardownTimeoutNs, nullptr);

    const QString rtspUser = sourceUrl.userName(QUrl::FullyDecoded);
    const QString rtspPassword = sourceUrl.password(QUrl::FullyDecoded);
    if (!rtspUser.isEmpty()) {
        g_object_set(source.get(), "user-id", rtspUser.toUtf8().constData(), "user-pw",
                     rtspPassword.toUtf8().constData(), nullptr);
    }
    return source;
}

}  // namespace GStreamer::SourceFactory
