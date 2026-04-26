#include "GstSourceFactory.h"

#include <QtCore/QLatin1String>
#include <QtCore/QLatin1StringView>
#include <QtCore/QStringLiteral>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <gst/gst.h>
#include <gst/rtsp/gstrtspurl.h>
#include <gst/rtsp/gstrtsptransport.h>

#include "QGCLoggingCategory.h"
#include "VideoSourceResolver.h"

QGC_LOGGING_CATEGORY(GstSourceFactoryLog, "VideoManager.GStreamer.SourceFactory")

namespace GstSourceFactory {

namespace {

gboolean isValidRtspUri(const gchar* uriStr)
{
    if (!uriStr || !gst_uri_is_valid(uriStr))
        return FALSE;

    GstRTSPUrl* url = nullptr;
    const GstRTSPResult result = gst_rtsp_url_parse(uriStr, &url);
    if (result != GST_RTSP_OK || !url) {
        if (url)
            gst_rtsp_url_free(url);
        return FALSE;
    }

    const gboolean hasHost = url->host && url->host[0] != '\0';
    gst_rtsp_url_free(url);
    return hasHost;
}

bool padCarriesVideo(GstPad* pad)
{
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps)
        caps = gst_pad_query_caps(pad, nullptr);
    if (!caps)
        return true;

    bool isVideo = false;
    for (guint i = 0; i < gst_caps_get_size(caps); ++i) {
        const GstStructure* structure = gst_caps_get_structure(caps, i);
        if (!structure)
            continue;

        const char* name = gst_structure_get_name(structure);
        if (name && g_str_has_prefix(name, "video/")) {
            isVideo = true;
            break;
        }
        if (name && g_str_equal(name, "application/x-rtp")) {
            const char* media = gst_structure_get_string(structure, "media");
            if (!media || g_str_equal(media, "video")) {
                isVideo = true;
                break;
            }
        }
    }

    gst_clear_caps(&caps);
    return isVideo;
}

/// Ghost-pad a newly-added parsebin src pad onto the parent bin.
void wrapWithGhostPad(GstElement* element, GstPad* pad, gpointer /*data*/)
{
    gchar* name = gst_pad_get_name(pad);
    if (!name) {
        qCCritical(GstSourceFactoryLog) << "gst_pad_get_name() failed";
        return;
    }

    GstPad* ghostpad = gst_ghost_pad_new(name, pad);
    g_clear_pointer(&name, g_free);
    if (!ghostpad) {
        qCCritical(GstSourceFactoryLog) << "gst_ghost_pad_new() failed";
        return;
    }

    (void)gst_pad_set_active(ghostpad, TRUE);
    if (!gst_element_add_pad(GST_ELEMENT_PARENT(element), ghostpad)) {
        qCCritical(GstSourceFactoryLog) << "gst_element_add_pad() failed";
    }
}

/// Link a dynamically-added pad to the target element's sink pad.
void linkPadToSink(GstElement* element, GstPad* pad, gpointer data)
{
    if (!padCarriesVideo(pad))
        return;

    gchar* name = gst_pad_get_name(pad);
    if (!name) {
        qCCritical(GstSourceFactoryLog) << "gst_pad_get_name() failed";
        return;
    }

    if (!gst_element_link_pads(element, name, GST_ELEMENT(data), "sink")) {
        qCCritical(GstSourceFactoryLog) << "gst_element_link_pads() failed";
    }
    g_clear_pointer(&name, g_free);
}

/// Check if a source element has src pads and whether they carry RTP.
void probeSourcePads(GstElement* source, bool& hasPads, bool& isRtp)
{
    struct ProbeCtx
    {
        bool hasPads = false;
        bool isRtp = false;
    };

    ProbeCtx ctx;

    auto probeFn = [](GstElement* /*element*/, GstPad* pad, gpointer user_data) -> gboolean {
        auto* c = static_cast<ProbeCtx*>(user_data);
        c->hasPads = true;

        GstCaps* filter = gst_caps_from_string("application/x-rtp");
        if (filter) {
            GstCaps* caps = gst_pad_query_caps(pad, nullptr);
            if (caps) {
                if (!gst_caps_is_any(caps) && gst_caps_can_intersect(caps, filter))
                    c->isRtp = true;
                gst_clear_caps(&caps);
            }
            gst_clear_caps(&filter);
        }
        return TRUE;
    };

    gst_element_foreach_src_pad(source, probeFn, &ctx);
    hasPads = ctx.hasPads;
    isRtp = ctx.isRtp;
}

// GStreamer < 1.28 workaround: parsebin doesn't negotiate hvc1/avc stream-format.
#if !defined(QGC_GST_BUILD_VERSION_MAJOR) || (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR < 28)
gboolean filterParserCaps(GstElement* /*bin*/, GstPad* /*pad*/, GstElement* /*element*/, GstQuery* query,
                          gpointer /*data*/)
{
    if (GST_QUERY_TYPE(query) != GST_QUERY_CAPS)
        return FALSE;

    GstCaps* srcCaps;
    gst_query_parse_caps(query, &srcCaps);
    if (!srcCaps || gst_caps_is_any(srcCaps))
        return FALSE;

    GstCaps* sinkCaps = nullptr;
    GstStructure* structure = gst_caps_get_structure(srcCaps, 0);
    if (gst_structure_has_name(structure, "video/x-h265")) {
        GstCaps* filter = gst_caps_from_string("video/x-h265");
        if (gst_caps_can_intersect(srcCaps, filter))
            sinkCaps = gst_caps_from_string("video/x-h265,stream-format=hvc1");
        gst_clear_caps(&filter);
    } else if (gst_structure_has_name(structure, "video/x-h264")) {
        GstCaps* filter = gst_caps_from_string("video/x-h264");
        if (gst_caps_can_intersect(srcCaps, filter))
            sinkCaps = gst_caps_from_string("video/x-h264,stream-format=avc");
        gst_clear_caps(&filter);
    }

    if (sinkCaps) {
        gst_query_set_caps_result(query, sinkCaps);
        gst_clear_caps(&sinkCaps);
        return TRUE;
    }
    return FALSE;
}
#endif

void configureParsebin(GstElement* parser)
{
#if !defined(QGC_GST_BUILD_VERSION_MAJOR) || (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR < 28)
    (void)g_signal_connect(parser, "autoplug-query", G_CALLBACK(filterParserCaps), nullptr);
#endif
    (void)g_signal_connect(parser, "pad-added", G_CALLBACK(wrapWithGhostPad), nullptr);
}

/// Wrap a raw source element + optional tsdemux + parsebin into a source bin.
GstElement* buildSourceBin(GstElement* source, bool needsTsdemux, int bufferMs)
{
    GstElement* bin = gst_bin_new("sourcebin");
    GstElement* parser = gst_element_factory_make("parsebin", "parser");
    GstElement* tsdemux = nullptr;

    if (!bin || !parser) {
        qCCritical(GstSourceFactoryLog) << "Failed to create sourcebin or parsebin";
        gst_clear_object(&bin);
        gst_clear_object(&parser);
        return nullptr;
    }

    configureParsebin(parser);
    gst_bin_add_many(GST_BIN(bin), source, parser, nullptr);

    GstElement* linkFrom = source;

    if (needsTsdemux) {
        tsdemux = gst_element_factory_make("tsdemux", nullptr);
        if (!tsdemux) {
            qCCritical(GstSourceFactoryLog) << "Failed to create tsdemux";
            gst_clear_object(&bin);
            return nullptr;
        }
        gst_bin_add(GST_BIN(bin), tsdemux);
        if (!gst_element_link(source, tsdemux)) {
            qCCritical(GstSourceFactoryLog) << "Failed to link source → tsdemux";
            gst_clear_object(&bin);
            return nullptr;
        }
        linkFrom = tsdemux;
    }

    bool hasPads = false, isRtp = false;
    probeSourcePads(linkFrom, hasPads, isRtp);

    if (hasPads) {
        if (isRtp && bufferMs >= 0) {
            GstElement* jitterBuf = gst_element_factory_make("rtpjitterbuffer", nullptr);
            if (!jitterBuf) {
                qCCritical(GstSourceFactoryLog) << "Failed to create rtpjitterbuffer";
                gst_clear_object(&bin);
                return nullptr;
            }
            const int jitterLatency = (bufferMs > 0) ? bufferMs : 60;
            // do-retransmission: no-op on UDP without RTCP feedback, helpful when
            //   upstream supports RFC 4588; set TRUE unconditionally so the tuner
            //   path is consistent with rtspsrc's internal jitterbuffer.
            // add-reference-timestamp-meta: attaches GstReferenceTimestampMeta
            //   (1.18+) with the sender's absolute NTP time once RTCP SR arrives,
            //   letting downstream (VideoStreamStats) compute true one-way delay.
            g_object_set(jitterBuf,
                         "latency", static_cast<guint>(jitterLatency),
                         "do-retransmission", TRUE,
                         "add-reference-timestamp-meta", TRUE,
                         nullptr);
            gst_bin_add(GST_BIN(bin), jitterBuf);
            if (!gst_element_link_many(linkFrom, jitterBuf, parser, nullptr)) {
                qCCritical(GstSourceFactoryLog) << "Failed to link source → jitterbuffer → parser";
                gst_clear_object(&bin);
                return nullptr;
            }
        } else {
            if (!gst_element_link(linkFrom, parser)) {
                qCCritical(GstSourceFactoryLog) << "Failed to link source → parser";
                gst_clear_object(&bin);
                return nullptr;
            }
        }
    } else {
        (void)g_signal_connect(linkFrom, "pad-added", G_CALLBACK(linkPadToSink), parser);
    }

    return bin;
}

// ═══════════════════════════════════════════════════════════════════════════
// Per-scheme source element constructors
// ═══════════════════════════════════════════════════════════════════════════

GstElement* createRtspSource(const QString& uri)
{
    if (!isValidRtspUri(uri.toUtf8().constData())) {
        qCCritical(GstSourceFactoryLog) << "Invalid RTSP URI:" << uri;
        return nullptr;
    }

    GstElement* source = gst_element_factory_make("rtspsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "Failed to create rtspsrc";
        return nullptr;
    }

    constexpr guint kRtspTransports =
        GST_RTSP_LOWER_TRANS_UDP | GST_RTSP_LOWER_TRANS_UDP_MCAST | GST_RTSP_LOWER_TRANS_TCP;

    // do-retransmission=TRUE asks rtspsrc's internal rtpjitterbuffer to send
    // RTCP NACK requests for missing packets when the server advertises RFC 4588
    // retransmission. It's a no-op if the server doesn't support it, so safe to
    // enable unconditionally — recovers reorder/loss without waiting for IDR.
    g_object_set(source, "location", uri.toUtf8().constData(), "latency", 25, "protocols", kRtspTransports,
                 "do-retransmission", TRUE, nullptr);
    return source;
}

GstElement* createTcpMpegtsSource(const QString& host, quint16 port)
{
    GstElement* source = gst_element_factory_make("tcpclientsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "Failed to create tcpclientsrc";
        return nullptr;
    }
    g_object_set(source, "host", host.toUtf8().constData(), "port", port, nullptr);
    return source;
}

GstElement* createUdpSource(const QString& host, quint16 port, const char* rtpCaps = nullptr)
{
    GstElement* source = gst_element_factory_make("udpsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "Failed to create udpsrc";
        return nullptr;
    }

    const QString uri = QStringLiteral("udp://%1:%2").arg(host, QString::number(port));
    g_object_set(source, "uri", uri.toUtf8().constData(), nullptr);

    if (rtpCaps) {
        GstCaps* caps = gst_caps_from_string(rtpCaps);
        if (caps) {
            g_object_set(source, "caps", caps, nullptr);
            gst_caps_unref(caps);
        }
    }
    return source;
}

GstElement* createWhepSource(const QString& uri)
{
    GstElementFactory* factory = gst_element_factory_find("whepsrc");
    if (!factory) {
        qCWarning(GstSourceFactoryLog) << "whepsrc not found — requires GStreamer 1.24+";
        return nullptr;
    }
    gst_object_unref(factory);

    QString endpointUrl(uri);
    if (endpointUrl.startsWith(QLatin1String("wheps://"), Qt::CaseInsensitive)) {
        endpointUrl.replace(0, 7, QStringLiteral("https://"));
    } else {
        endpointUrl.replace(0, 6, QStringLiteral("http://"));
    }

    GstElement* source = gst_element_factory_make("whepsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "Failed to create whepsrc";
        return nullptr;
    }
    g_object_set(source, "whep-endpoint", endpointUrl.toUtf8().constData(), nullptr);
    qCDebug(GstSourceFactoryLog) << "Created WHEP source:" << endpointUrl;

    GstElement* bin = gst_bin_new("whep-source");
    GstElement* parser = gst_element_factory_make("parsebin", "parser");
    if (!bin || !parser) {
        qCCritical(GstSourceFactoryLog) << "Failed to create WHEP source bin";
        gst_clear_object(&bin);
        gst_clear_object(&parser);
        gst_clear_object(&source);
        return nullptr;
    }

    configureParsebin(parser);
    gst_bin_add_many(GST_BIN(bin), source, parser, nullptr);
    (void)g_signal_connect(source, "pad-added", G_CALLBACK(linkPadToSink), parser);
    return bin;
}

GstElement* createSrtSource(const QString& uri)
{
    GstElement* source = gst_element_factory_make("srtsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "Failed to create srtsrc — is gst-plugins-bad (srt) installed?";
        return nullptr;
    }

    const QUrl url(uri);
    const int latency = QUrlQuery(url).hasQueryItem(QStringLiteral("latency"))
                            ? QUrlQuery(url).queryItemValue(QStringLiteral("latency")).toInt()
                            : 125;

    g_object_set(source, "uri", uri.toUtf8().constData(), "latency", latency, "wait-for-connection", FALSE, nullptr);
    qCDebug(GstSourceFactoryLog) << "Created SRT source:" << uri << "latency:" << latency << "ms";
    return source;
}

/// Build a source element by parsing a raw `gst_parse_launch()` description.
/// When the parsed tree is itself a GstPipeline, its children are reparented
/// into a new bin with a ghost src pad on the last element, so the caller can
/// treat the result uniformly as a "source element" downstream.
GstElement* createFromPipelineDescription(const QString& description)
{
    if (description.isEmpty()) {
        qCCritical(GstSourceFactoryLog) << "Empty pipeline description";
        return nullptr;
    }

    qCDebug(GstSourceFactoryLog) << "Parsing custom pipeline:" << description;

    GError* error = nullptr;
    GstElement* parsed = gst_parse_bin_from_description(description.toUtf8().constData(), TRUE, &error);

    if (error) {
        qCCritical(GstSourceFactoryLog) << "Pipeline parse error:" << error->message;
        g_clear_error(&error);
        gst_clear_object(&parsed);
        return nullptr;
    }

    if (!parsed) {
        qCCritical(GstSourceFactoryLog) << "gst_parse_bin_from_description returned null";
        return nullptr;
    }

    return parsed;
}

// Prefix for gstreamer-pipeline: URIs
constexpr QLatin1StringView kPipelinePrefix("gstreamer-pipeline:");

}  // anonymous namespace

GstElement* createFromUri(const QString& uri, int bufferMs)
{
    return createFromSource(VideoSourceResolver::describeUri(uri), bufferMs);
}

GstElement* createFromSource(const VideoSourceResolver::SourceDescriptor& sourceDescriptor, int bufferMs)
{
    const QString& uri = sourceDescriptor.uri;
    if (uri.isEmpty()) {
        qCCritical(GstSourceFactoryLog) << "Empty URI";
        return nullptr;
    }

    using T = VideoSourceResolver::Transport;
    const T transport = sourceDescriptor.transport;

    if (transport == T::Pipeline) {
        QString desc = uri.mid(kPipelinePrefix.size());
        return createFromPipelineDescription(desc);
    }
    if (transport == T::WHEP)
        return createWhepSource(uri);
    const QUrl url(uri);
    GstElement* source = nullptr;
    bool isMPEGTS = false;

    switch (transport) {
        case T::RTSP:
            source = createRtspSource(uri);
            break;
        case T::TCP:
            source = createTcpMpegtsSource(url.host(), static_cast<quint16>(url.port()));
            isMPEGTS = true;
            break;
        case T::UDP_H265:
            source = createUdpSource(url.host(), static_cast<quint16>(url.port()),
                "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265");
            break;
        case T::UDP_H264:
            source = createUdpSource(url.host(), static_cast<quint16>(url.port()),
                "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264");
            break;
        case T::MPEGTS:
            source = createUdpSource(url.host(), static_cast<quint16>(url.port()));
            isMPEGTS = true;
            break;
        case T::SRT:
            source = createSrtSource(uri);
            isMPEGTS = true;
            break;
        case T::Pipeline:
        case T::WHEP:
        case T::HLS:
        case T::DASH:
        case T::Unknown:
            qCWarning(GstSourceFactoryLog) << "Unrecognized URI scheme in" << uri;
            return nullptr;
    }

    if (!source)
        return nullptr;

    GstElement* bin = buildSourceBin(source, isMPEGTS, bufferMs);
    if (!bin) {
        gst_object_unref(source);
        return nullptr;
    }
    return bin;
}

}  // namespace GstSourceFactory
