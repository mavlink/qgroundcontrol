#include "GstSourceFactory.h"

#include "GStreamerHelpers.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QUrl>

#include <gst/gst.h>
#include <gst/rtsp/gstrtsptransport.h>

QGC_LOGGING_CATEGORY(GstSourceFactoryLog, "Video.GStreamer.GstSourceFactory")

namespace {

constexpr int kRtspLatencyMs = 25;
constexpr guint64 kRtspTcpTimeoutUs = G_GUINT64_CONSTANT(5000000);
constexpr int kRtspRetry = 3;
constexpr int kUdpBufferSizeBytes = 8 * 1024 * 1024;

#if !defined(QGC_GST_BUILD_VERSION_MAJOR) || (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR < 28)
gboolean filterParserCaps([[maybe_unused]] GstElement *bin, [[maybe_unused]] GstPad *pad,
                          [[maybe_unused]] GstElement *element, GstQuery *query,
                          [[maybe_unused]] gpointer data)
{
    if (GST_QUERY_TYPE(query) != GST_QUERY_CAPS) {
        return FALSE;
    }

    GstCaps *srcCaps;
    gst_query_parse_caps(query, &srcCaps);
    if (!srcCaps || gst_caps_is_any(srcCaps)) {
        return FALSE;
    }

    GstCaps *sinkCaps = nullptr;
    GstCaps *filter = nullptr;
    GstStructure *structure = gst_caps_get_structure(srcCaps, 0);
    if (gst_structure_has_name(structure, "video/x-h265")) {
        filter = gst_caps_from_string("video/x-h265");
        if (gst_caps_can_intersect(srcCaps, filter)) {
            sinkCaps = gst_caps_from_string("video/x-h265,stream-format=hvc1");
        }
        gst_clear_caps(&filter);
    } else if (gst_structure_has_name(structure, "video/x-h264")) {
        filter = gst_caps_from_string("video/x-h264");
        if (gst_caps_can_intersect(srcCaps, filter)) {
            sinkCaps = gst_caps_from_string("video/x-h264,stream-format=avc");
        }
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

void wrapWithGhostPad(GstElement *element, GstPad *pad, [[maybe_unused]] gpointer data)
{
    gchar *name = gst_pad_get_name(pad);
    if (!name) {
        qCCritical(GstSourceFactoryLog) << "gst_pad_get_name() failed";
        return;
    }

    GstPad *ghostpad = gst_ghost_pad_new(name, pad);
    if (!ghostpad) {
        qCCritical(GstSourceFactoryLog) << "gst_ghost_pad_new() failed";
        g_clear_pointer(&name, g_free);
        return;
    }

    g_clear_pointer(&name, g_free);

    (void) gst_pad_set_active(ghostpad, TRUE);

    // gst_object_get_parent takes a ref; the macro form (GST_ELEMENT_PARENT) does not, so a
    // concurrent bin teardown could finalize the parent between read and gst_element_add_pad.
    GstObject *parent = gst_object_get_parent(GST_OBJECT(element));
    if (parent) {
        if (!gst_element_add_pad(GST_ELEMENT(parent), ghostpad)) {
            qCCritical(GstSourceFactoryLog) << "gst_element_add_pad() failed";
        }
        gst_object_unref(parent);
    } else {
        qCWarning(GstSourceFactoryLog) << "wrapWithGhostPad: element has no parent — bin already torn down?";
        gst_object_unref(ghostpad);
    }
}

void linkPad(GstElement *element, GstPad *pad, gpointer data)
{
    gchar *name = gst_pad_get_name(pad);
    if (!name) {
        qCCritical(GstSourceFactoryLog) << "gst_pad_get_name() failed";
        return;
    }

    if (!gst_element_link_pads(element, name, GST_ELEMENT(data), "sink")) {
        qCCritical(GstSourceFactoryLog) << "gst_element_link_pads() failed";
    }

    g_clear_pointer(&name, g_free);
}

gboolean padProbe([[maybe_unused]] GstElement *element, GstPad *pad, gpointer user_data)
{
    int *probeRes = static_cast<int*>(user_data);
    *probeRes |= 1;

    GstCaps *filter = gst_caps_from_string("application/x-rtp");
    if (filter) {
        GstCaps *caps = gst_pad_query_caps(pad, nullptr);
        if (caps) {
            if (!gst_caps_is_any(caps) && gst_caps_can_intersect(caps, filter)) {
                *probeRes |= 2;
            }

            gst_clear_caps(&caps);
        }

        gst_clear_caps(&filter);
    }

    return TRUE;
}

bool validPort(int port)
{
    return port > 0 && port <= 65535;
}

} // namespace

namespace GStreamer::SourceFactory {

GstElement *create(const QString &uri, JitterBuffer jitterBuffer)
{
    if (uri.isEmpty()) {
        qCCritical(GstSourceFactoryLog) << "Failed because URI is not specified";
        return nullptr;
    }

    const QUrl sourceUrl(uri);
    const QString scheme = sourceUrl.scheme().toLower();

    const bool isRtsp = scheme.startsWith(QLatin1String("rtsp"));
    const bool isUdpH264 = (scheme == QLatin1String("udp"));
    const bool isUdpH265 = (scheme == QLatin1String("udp265"));
    const bool isUdpMPEGTS = (scheme == QLatin1String("mpegts"));
    const bool isTcpMPEGTS = (scheme == QLatin1String("tcp"));

    if (!isRtsp && !isUdpH264 && !isUdpH265 && !isUdpMPEGTS && !isTcpMPEGTS) {
        qCWarning(GstSourceFactoryLog) << "Unsupported URI scheme:" << scheme << "in" << uri;
        return nullptr;
    }

    // We hold these until they are added to `bin`; after each gst_bin_add* the
    // local is nulled and a non-owning alias is used downstream. This keeps the
    // unconditional cleanup at the bottom safe — `gst_clear_object(&bin)` will
    // finalize anything bin owns, and the cleanup pointers below stay null.
    GstElement *source = nullptr;
    GstElement *parser = nullptr;
    GstElement *tsdemux = nullptr;
    GstElement *buffer = nullptr;
    GstElement *bin = nullptr;
    GstElement *srcbin = nullptr;

    do {
        if (isRtsp) {
            if (!GStreamer::isValidRtspUri(uri.toUtf8().constData())) {
                qCCritical(GstSourceFactoryLog) << "Invalid RTSP URI:" << uri;
                break;
            }

            source = gst_element_factory_make("rtspsrc", "source");
            if (!source) {
                qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('rtspsrc') failed";
                break;
            }

            QUrl cleanUrl(sourceUrl);
            cleanUrl.setUserInfo(QString());
            const QByteArray cleanLocation = cleanUrl.toEncoded();

            // protocols mask enables TCP-interleaved fallback when UDP is blocked. rtspsrc tries
            // each in order on SETUP; without this, firewalled networks see the connection hang
            // until tcp-timeout instead of negotiating TCP.
            constexpr GstRTSPLowerTrans kRtspProtocols =
                static_cast<GstRTSPLowerTrans>(GST_RTSP_LOWER_TRANS_UDP | GST_RTSP_LOWER_TRANS_TCP);

            g_object_set(source,
                         "location", cleanLocation.constData(),
                         "latency", kRtspLatencyMs,
                         "do-rtcp", TRUE,
                         "tcp-timeout", kRtspTcpTimeoutUs,
                         "udp-reconnect", TRUE,
                         "drop-on-latency", TRUE,
                         "retry", kRtspRetry,
                         "protocols", kRtspProtocols,
                         nullptr);

            const QString rtspUser = sourceUrl.userName(QUrl::FullyDecoded);
            const QString rtspPassword = sourceUrl.password(QUrl::FullyDecoded);
            if (!rtspUser.isEmpty()) {
                g_object_set(source,
                             "user-id", rtspUser.toUtf8().constData(),
                             "user-pw", rtspPassword.toUtf8().constData(),
                             nullptr);
            }
        } else if (isTcpMPEGTS) {
            const int port = sourceUrl.port();
            if (!validPort(port)) {
                qCCritical(GstSourceFactoryLog) << "Invalid TCP port" << port << "in" << uri;
                break;
            }
            const QString host = sourceUrl.host();
            if (host.isEmpty()) {
                qCCritical(GstSourceFactoryLog) << "Missing host in TCP URI" << uri;
                break;
            }

            source = gst_element_factory_make("tcpclientsrc", "source");
            if (!source) {
                qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('tcpclientsrc') failed";
                break;
            }

            g_object_set(source,
                         "host", host.toUtf8().constData(),
                         "port", static_cast<int>(port),
                         nullptr);
        } else if (isUdpH264 || isUdpH265 || isUdpMPEGTS) {
            const int port = sourceUrl.port();
            if (!validPort(port)) {
                qCCritical(GstSourceFactoryLog) << "Invalid UDP port" << port << "in" << uri;
                break;
            }

            source = gst_element_factory_make("udpsrc", "source");
            if (!source) {
                qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('udpsrc') failed";
                break;
            }

            // Normalize udp265:// / mpegts:// to udp:// for udpsrc, preserving query
            // params (multicast-iface, etc.) and dropping any user-info udpsrc ignores.
            QUrl udpUrl(sourceUrl);
            udpUrl.setScheme(QStringLiteral("udp"));
            udpUrl.setUserInfo(QString());
            const QByteArray udpUri = udpUrl.toEncoded();

            g_object_set(source,
                         "uri", udpUri.constData(),
                         "buffer-size", kUdpBufferSizeBytes,
                         nullptr);

            GstCaps *caps = nullptr;
            if (isUdpH264) {
                caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264");
            } else if (isUdpH265) {
                caps = gst_caps_from_string("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265");
            }

            if ((isUdpH264 || isUdpH265) && !caps) {
                qCCritical(GstSourceFactoryLog) << "gst_caps_from_string() failed";
                break;
            }

            if (caps) {
                g_object_set(source,
                             "caps", caps,
                             nullptr);
                gst_clear_caps(&caps);
            }
        }

        bin = gst_bin_new("sourcebin");
        if (!bin) {
            qCCritical(GstSourceFactoryLog) << "gst_bin_new('sourcebin') failed";
            break;
        }

        parser = gst_element_factory_make("parsebin", "parser");
        if (!parser) {
            qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('parsebin') failed";
            break;
        }

        // GStreamer <1.28 misnegotiates parser→decoder caps; force avc/hvc1. 1.28+ fixes it
        // and the forced caps break HW decoders that need byte-stream (Qualcomm AMC, D3D12).
#if !defined(QGC_GST_BUILD_VERSION_MAJOR) || (QGC_GST_BUILD_VERSION_MAJOR == 1 && QGC_GST_BUILD_VERSION_MINOR < 28)
        (void) g_signal_connect(parser, "autoplug-query", G_CALLBACK(filterParserCaps), nullptr);
#endif

        gst_bin_add_many(GST_BIN(bin), source, parser, nullptr);
        GstElement *upstream = source;
        GstElement *binParser = parser;
        source = nullptr;
        parser = nullptr;

        // FIXME: AV: Android does not determine MPEG2-TS via parsebin - have to explicitly state which demux to use
        // FIXME: AV: tsdemux handling is a bit ugly - let's try to find elegant solution for that later
        if (isTcpMPEGTS || isUdpMPEGTS) {
            tsdemux = gst_element_factory_make("tsdemux", nullptr);
            if (!tsdemux) {
                qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('tsdemux') failed";
                break;
            }

            (void) gst_bin_add(GST_BIN(bin), tsdemux);
            GstElement *demux = tsdemux;
            tsdemux = nullptr;

            if (!gst_element_link(upstream, demux)) {
                qCCritical(GstSourceFactoryLog) << "gst_element_link(source, tsdemux) failed";
                break;
            }

            upstream = demux;
        }

        int probeRes = 0;
        (void) gst_element_foreach_src_pad(upstream, padProbe, &probeRes);

        if (probeRes & 1) {
            if ((probeRes & 2) && jitterBuffer != JitterBuffer::None) {
                buffer = gst_element_factory_make("rtpjitterbuffer", nullptr);
                if (!buffer) {
                    qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('rtpjitterbuffer') failed";
                    break;
                }

                g_object_set(buffer,
                             "do-lost", TRUE,
                             "drop-on-latency", jitterBuffer == JitterBuffer::DropOnLatency ? TRUE : FALSE,
                             nullptr);

                (void) gst_bin_add(GST_BIN(bin), buffer);
                GstElement *jitter = buffer;
                buffer = nullptr;

                if (!gst_element_link_many(upstream, jitter, binParser, nullptr)) {
                    qCCritical(GstSourceFactoryLog) << "gst_element_link_many(source, rtpjitterbuffer, parser) failed";
                    break;
                }
            } else {
                if (!gst_element_link(upstream, binParser)) {
                    qCCritical(GstSourceFactoryLog) << "gst_element_link(source, parser) failed";
                    break;
                }
            }
        } else {
            (void) g_signal_connect(upstream, "pad-added", G_CALLBACK(linkPad), binParser);
        }

        (void) g_signal_connect(binParser, "pad-added", G_CALLBACK(wrapWithGhostPad), nullptr);

        srcbin = bin;
        bin = nullptr;
    } while(0);

    gst_clear_object(&bin);
    gst_clear_object(&parser);
    gst_clear_object(&tsdemux);
    gst_clear_object(&buffer);
    gst_clear_object(&source);

    return srcbin;
}

} // namespace GStreamer::SourceFactory
