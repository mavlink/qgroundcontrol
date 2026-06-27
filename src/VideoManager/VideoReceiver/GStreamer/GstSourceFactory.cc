#include "GstSourceFactory.h"

#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <gst/gst.h>
#include <gst/rtsp/gstrtsptransport.h>

#include "GStreamerHelpers.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(GstSourceFactoryLog, "Video.GStreamer.GstSourceFactory")

namespace {

constexpr guint64 kRtspTcpTimeoutUs = G_GUINT64_CONSTANT(5000000);
constexpr int kRtspRetry = 3;
constexpr int kUdpBufferSizeBytes = 8 * 1024 * 1024;

// Older Linux/system GStreamer needs an autoplug-query caps filter to keep parsebin on byte-stream output.
#if defined(QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER)
gboolean filterParserCaps([[maybe_unused]] GstElement* bin, [[maybe_unused]] GstPad* pad,
                          [[maybe_unused]] GstElement* element, GstQuery* query, [[maybe_unused]] gpointer data)
{
    if (GST_QUERY_TYPE(query) != GST_QUERY_CAPS) {
        return FALSE;
    }

    GstCaps* srcCaps;
    gst_query_parse_caps(query, &srcCaps);
    if (!srcCaps || gst_caps_is_any(srcCaps)) {
        return FALSE;
    }

    GstCaps* sinkCaps = nullptr;
    GstCaps* filter = nullptr;
    GstStructure* structure = gst_caps_get_structure(srcCaps, 0);
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

void wrapWithGhostPad(GstElement* element, GstPad* pad, [[maybe_unused]] gpointer data)
{
    gchar* name = gst_pad_get_name(pad);
    if (!name) {
        qCCritical(GstSourceFactoryLog) << "gst_pad_get_name() failed";
        return;
    }

    GstPad* ghostpad = gst_ghost_pad_new(name, pad);
    if (!ghostpad) {
        qCCritical(GstSourceFactoryLog) << "gst_ghost_pad_new() failed";
        g_clear_pointer(&name, g_free);
        return;
    }

    g_clear_pointer(&name, g_free);

    (void) gst_pad_set_active(ghostpad, TRUE);

    // gst_object_get_parent takes a ref (GST_ELEMENT_PARENT does not) — guards against a
    // concurrent bin teardown finalizing the parent before gst_element_add_pad.
    GstObject* parent = gst_object_get_parent(GST_OBJECT(element));
    if (parent) {
        if (!gst_element_add_pad(GST_ELEMENT(parent), ghostpad)) {
            qCCritical(GstSourceFactoryLog) << "gst_element_add_pad() failed";
            // add_pad only sinks the ghost pad's floating ref on success; release it on failure.
            gst_object_unref(ghostpad);
        }
        gst_object_unref(parent);
    } else {
        qCWarning(GstSourceFactoryLog) << "wrapWithGhostPad: element has no parent — bin already torn down?";
        gst_object_unref(ghostpad);
    }
}

gboolean padProbe([[maybe_unused]] GstElement* element, GstPad* pad, gpointer user_data)
{
    int* probeRes = static_cast<int*>(user_data);
    *probeRes |= 1;

    GstCaps* filter = gst_caps_from_string("application/x-rtp");
    if (filter) {
        GstCaps* caps = gst_pad_query_caps(pad, nullptr);
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

bool addStaticGhostPad(GstElement* element)
{
    GstPad* srcPad = gst_element_get_static_pad(element, "src");
    if (!srcPad) {
        qCCritical(GstSourceFactoryLog) << "gst_element_get_static_pad('src') failed";
        return false;
    }

    wrapWithGhostPad(element, srcPad, nullptr);
    gst_object_unref(srcPad);
    return true;
}

bool validPort(int port)
{
    return port > 0 && port <= 65535;
}

}  // namespace

namespace GStreamer::SourceFactory {

namespace {

// Shared by the static and dynamic (pad-added) link paths so both apply the identical jitter-buffer
// policy. rtx-delay/rtx-max-retries are fixed (not auto) so RF-link recovery is predictable.
void configureJitterBuffer(GstElement* buffer, const Config& config, guint latencyMs)
{
    g_object_set(buffer, "latency", latencyMs, "do-lost", TRUE, "do-retransmission",
                 config.doRetransmission ? TRUE : FALSE, "drop-on-latency",
                 config.jitterBuffer == JitterBuffer::DropOnLatency ? TRUE : FALSE, nullptr);
    if (config.doRetransmission) {
        // Fixed 25 ms rtx-delay + single retry: bounded recovery latency over a lossy RF link,
        // instead of the auto(-1) heuristic that can stack retries and balloon playout latency.
        g_object_set(buffer, "rtx-delay", 25, "rtx-max-retries", 1, nullptr);
    }
}

// Heap-owned context for the dynamic (pad-added) link path; freed when @binParser is finalized.
struct DynamicLinkContext
{
    GstElement* binParser;
    Config config;
    guint latencyMs;
    bool allowJitterBuffer;  // false for RTSP (rtspsrc has its own internal jitterbuffer)
};

void linkPad(GstElement* element, GstPad* pad, gpointer data)
{
    const auto* ctx = static_cast<const DynamicLinkContext*>(data);

    // tsdemux fires pad-added for audio/data pads too; only link video src pads so non-video
    // pads don't trigger a spurious CRITICAL cascade on the expected link failure.
    if (GST_PAD_DIRECTION(pad) != GST_PAD_SRC) {
        return;
    }

    bool isVideo = false;
    bool isRtp = false;
    GstCaps* caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }
    if (caps) {
        const guint n = gst_caps_get_size(caps);
        for (guint i = 0; i < n; ++i) {
            const GstStructure* st = gst_caps_get_structure(caps, i);
            const gchar* sname = gst_structure_get_name(st);
            if (!sname) {
                continue;
            }
            if (g_str_has_prefix(sname, "video/")) {
                isVideo = true;
            } else if (g_str_equal(sname, "application/x-rtp")) {
                isRtp = true;
                // RTP carries video; the depayloader/parsebin downstream classifies the payload.
                isVideo = true;
            }
        }
        gst_clear_caps(&caps);
    }
    if (!isVideo) {
        return;
    }

    // Link only the first matching video pad. A second pad-added (duplicate RTP pad, or a later
    // tsdemux elementary stream) must not stack a second jitterbuffer or re-link an occupied sink.
    if (GstPad* parserSink = gst_element_get_static_pad(ctx->binParser, "sink")) {
        const gboolean alreadyLinked = gst_pad_is_linked(parserSink);
        gst_object_unref(parserSink);
        if (alreadyLinked) {
            return;
        }
    }

    // Insert a jitterbuffer ahead of the parser for RTP pads on non-RTSP dynamic sources, matching the
    // static path's policy. RTSP is excluded: rtspsrc owns an internal jitterbuffer (no double-buffering).
    GstElement* downstream = ctx->binParser;
    GstElement* dynamicBuffer = nullptr;
    const auto removeDynamicBuffer = [&dynamicBuffer] {
        if (!dynamicBuffer) {
            return;
        }

        GstObject* parent = gst_object_get_parent(GST_OBJECT(dynamicBuffer));
        if (parent) {
            gst_bin_remove(GST_BIN(parent), dynamicBuffer);
            gst_object_unref(parent);
        }
        dynamicBuffer = nullptr;
    };

    if (isRtp && ctx->allowJitterBuffer && (ctx->config.jitterBuffer != JitterBuffer::None)) {
        GstElement* bin = GST_ELEMENT(gst_object_get_parent(GST_OBJECT(ctx->binParser)));
        if (bin) {
            GstElement* buffer = gst_element_factory_make("rtpjitterbuffer", nullptr);
            if (buffer && gst_bin_add(GST_BIN(bin), buffer)) {
                configureJitterBuffer(buffer, ctx->config, ctx->latencyMs);
                if (gst_element_sync_state_with_parent(buffer) && gst_element_link(buffer, ctx->binParser)) {
                    downstream = buffer;
                    dynamicBuffer = buffer;
                } else {
                    qCWarning(GstSourceFactoryLog)
                        << "dynamic rtpjitterbuffer link/sync failed; linking pad straight to parser";
                    gst_bin_remove(GST_BIN(bin), buffer);
                    buffer = nullptr;
                }
            } else {
                qCWarning(GstSourceFactoryLog) << "dynamic rtpjitterbuffer create/add failed; skipping jitter buffer";
                if (buffer) {
                    gst_object_unref(buffer);
                }
            }
            gst_object_unref(bin);
        }
    }

    gchar* name = gst_pad_get_name(pad);
    if (!name) {
        qCWarning(GstSourceFactoryLog) << "gst_pad_get_name() failed";
        removeDynamicBuffer();
        return;
    }

    if (!gst_element_link_pads(element, name, downstream, "sink")) {
        qCWarning(GstSourceFactoryLog) << "gst_element_link_pads() failed for" << name;
        removeDynamicBuffer();
    }

    g_clear_pointer(&name, g_free);
}

// Each build* helper makes and configures the source element for one scheme family, logs the
// specific failure, and returns the owning element (or nullptr). create() owns it from here on.

GstElement* buildRtspSource(const QString& uri, const QUrl& sourceUrl, const Config& config, guint latencyMs)
{
    if (!GStreamer::isValidRtspUri(uri.toUtf8().constData())) {
        qCCritical(GstSourceFactoryLog) << "Invalid RTSP URI:" << sourceUrl.toDisplayString(QUrl::RemoveUserInfo);
        return nullptr;
    }

    GstElement* source = gst_element_factory_make("rtspsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('rtspsrc') failed";
        return nullptr;
    }

    QUrl cleanUrl(sourceUrl);
    cleanUrl.setUserInfo(QString());
    const QByteArray cleanLocation = cleanUrl.toEncoded();

    // protocols mask enables TCP-interleaved fallback when UDP is blocked; without it
    // firewalled networks hang until tcp-timeout instead of negotiating TCP.
    constexpr GstRTSPLowerTrans kRtspProtocols =
        static_cast<GstRTSPLowerTrans>(GST_RTSP_LOWER_TRANS_UDP | GST_RTSP_LOWER_TRANS_TCP);

    // do-retransmission forwards to rtspsrc's internal rtpjitterbuffer (added 1.6);
    // drop-on-latency=TRUE unless jitterBuffer==Buffered (opt out of bounded playout).
    const gboolean dropOnLatency = (config.jitterBuffer == JitterBuffer::Buffered) ? FALSE : TRUE;
    g_object_set(source, "location", cleanLocation.constData(), "latency", latencyMs, "do-rtcp", TRUE,
                 "do-retransmission", config.doRetransmission ? TRUE : FALSE, "tcp-timeout", kRtspTcpTimeoutUs,
                 "udp-reconnect", TRUE, "drop-on-latency", dropOnLatency, "retry", kRtspRetry, "protocols",
                 kRtspProtocols, nullptr);

    const QString rtspUser = sourceUrl.userName(QUrl::FullyDecoded);
    const QString rtspPassword = sourceUrl.password(QUrl::FullyDecoded);
    if (!rtspUser.isEmpty()) {
        g_object_set(source, "user-id", rtspUser.toUtf8().constData(), "user-pw",
                     rtspPassword.toUtf8().constData(), nullptr);
    }
    return source;
}

GstElement* buildTcpSource(const QUrl& sourceUrl)
{
    const int port = sourceUrl.port();
    if (!validPort(port)) {
        qCCritical(GstSourceFactoryLog) << "Invalid TCP port" << port << "in" << sourceUrl.toDisplayString(QUrl::RemoveUserInfo);
        return nullptr;
    }
    const QString host = sourceUrl.host();
    if (host.isEmpty()) {
        qCCritical(GstSourceFactoryLog) << "Missing host in TCP URI" << sourceUrl.toDisplayString(QUrl::RemoveUserInfo);
        return nullptr;
    }

    GstElement* source = gst_element_factory_make("tcpclientsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('tcpclientsrc') failed";
        return nullptr;
    }

    g_object_set(source, "host", host.toUtf8().constData(), "port", static_cast<int>(port), nullptr);
    return source;
}

GstElement* buildUdpSource(const QUrl& sourceUrl, bool isUdpH264, bool isUdpH265)
{
    const int port = sourceUrl.port();
    if (!validPort(port)) {
        qCCritical(GstSourceFactoryLog) << "Invalid UDP port" << port << "in" << sourceUrl.toDisplayString(QUrl::RemoveUserInfo);
        return nullptr;
    }

    GstElement* source = gst_element_factory_make("udpsrc", "source");
    if (!source) {
        qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('udpsrc') failed";
        return nullptr;
    }

    // Normalize udp265:///mpegts:// to udp:// for udpsrc, preserving query params.
    QUrl udpUrl(sourceUrl);
    udpUrl.setScheme(QStringLiteral("udp"));
    udpUrl.setUserInfo(QString());
    const QByteArray udpUri = udpUrl.toEncoded();

    // SO_RCVBUF above net.core.rmem_max needs CAP_NET_ADMIN; without it gstudpsrc fails with EPERM
    // (scary warning) then clamps. Request the kernel-permitted max instead (Linux/Android only).
    int udpBufferSize = kUdpBufferSizeBytes;
#ifdef Q_OS_LINUX
    // rmem_max is a boot-time sysctl; parse once and cache (0 = unreadable, leave request unclamped).
    static const int s_rmemMax = [] {
        QFile rmemMaxFile(QStringLiteral("/proc/sys/net/core/rmem_max"));
        if (rmemMaxFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            bool ok = false;
            const int sysMax = rmemMaxFile.readAll().trimmed().toInt(&ok);
            if (ok && (sysMax > 0)) {
                return sysMax;
            }
        }
        return 0;
    }();
    if ((s_rmemMax > 0) && (s_rmemMax < udpBufferSize)) {
        qCDebug(GstSourceFactoryLog) << "Clamping UDP buffer-size from" << udpBufferSize << "to net.core.rmem_max"
                                     << s_rmemMax;
        udpBufferSize = s_rmemMax;
    }
#endif
    g_object_set(source, "uri", udpUri.constData(), "buffer-size", udpBufferSize, nullptr);

    GstCaps* caps = nullptr;
    if (isUdpH264) {
        caps = gst_caps_from_string(
            "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264");
    } else if (isUdpH265) {
        caps = gst_caps_from_string(
            "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265");
    }

    if ((isUdpH264 || isUdpH265) && !caps) {
        qCCritical(GstSourceFactoryLog) << "gst_caps_from_string() failed";
        gst_object_unref(source);
        return nullptr;
    }

    if (caps) {
        g_object_set(source, "caps", caps, nullptr);
        gst_clear_caps(&caps);
    }
    return source;
}

// Wire upstream → (optional rtpjitterbuffer) → binParser, topology chosen by RTP probe (MPEG-TS
// links via pad-added). Created elements join @p bin; returns false (logged) on failure.
bool linkSourceToParser(GstElement* bin, GstElement* upstream, GstElement* binParser, const Config& config,
                        guint latencyMs, bool isMpegTs, bool isRtsp)
{
    // MPEG-TS exposes elementary streams via tsdemux dynamic src pads (none at NULL state) and
    // isn't raw RTP, so skip the RTP probe and link via pad-added when the video pad appears.
    int probeRes = 0;
    if (!isMpegTs) {
        (void) gst_element_foreach_src_pad(upstream, padProbe, &probeRes);
    }

    if (probeRes & 1) {
        if ((probeRes & 2) && config.jitterBuffer != JitterBuffer::None) {
            GstElement* buffer = gst_element_factory_make("rtpjitterbuffer", nullptr);
            if (!buffer) {
                qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('rtpjitterbuffer') failed";
                return false;
            }

            configureJitterBuffer(buffer, config, latencyMs);

            if (!gst_bin_add(GST_BIN(bin), buffer)) {
                qCCritical(GstSourceFactoryLog) << "gst_bin_add(rtpjitterbuffer) failed";
                gst_object_unref(buffer);
                return false;
            }
            // bin owns buffer now; a link failure below is cleaned up via the caller's bin teardown.
            if (!gst_element_link_many(upstream, buffer, binParser, nullptr)) {
                qCCritical(GstSourceFactoryLog) << "gst_element_link_many(source, rtpjitterbuffer, parser) failed";
                return false;
            }
        } else {
            if (!gst_element_link(upstream, binParser)) {
                qCCritical(GstSourceFactoryLog) << "gst_element_link(source, parser) failed";
                return false;
            }
        }
    } else {
        // linkPad applies the jitter-buffer policy itself when an application/x-rtp pad appears on a
        // non-RTSP source. Context lifetime is tied to binParser (freed on its finalize).
        auto* ctx = new DynamicLinkContext{binParser, config, latencyMs, !isRtsp};
        g_object_set_data_full(G_OBJECT(binParser), "qgc-dynamic-link-ctx", ctx,
                               [](gpointer p) { delete static_cast<DynamicLinkContext*>(p); });
        (void) g_signal_connect(upstream, "pad-added", G_CALLBACK(linkPad), ctx);
    }

    // pad-added fires on the streaming thread after create() returns the NULL-state bin, so it
    // can't race construction; wrapWithGhostPad ref-checks the parent against teardown.
    (void) g_signal_connect(binParser, "pad-added", G_CALLBACK(wrapWithGhostPad), nullptr);
    return true;
}

bool linkUdpRtpToDepayAndParser(GstElement* bin, GstElement* source, GstElement* depay, GstElement* parser,
                                const Config& config, guint latencyMs)
{
    if (config.jitterBuffer == JitterBuffer::None) {
        if (!gst_element_link_many(source, depay, parser, nullptr)) {
            qCCritical(GstSourceFactoryLog) << "gst_element_link_many(source, depay, parser) failed";
            return false;
        }
        return true;
    }

    GstElement* buffer = gst_element_factory_make("rtpjitterbuffer", nullptr);
    if (!buffer) {
        qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('rtpjitterbuffer') failed";
        return false;
    }

    configureJitterBuffer(buffer, config, latencyMs);

    if (!gst_bin_add(GST_BIN(bin), buffer)) {
        qCCritical(GstSourceFactoryLog) << "gst_bin_add(rtpjitterbuffer) failed";
        gst_object_unref(buffer);
        return false;
    }

    if (!gst_element_link_many(source, buffer, depay, parser, nullptr)) {
        qCCritical(GstSourceFactoryLog) << "gst_element_link_many(source, rtpjitterbuffer, depay, parser) failed";
        return false;
    }

    return true;
}

}  // namespace

GstElement* create(const QString& uri, const Config& config)
{
    if (uri.isEmpty()) {
        qCCritical(GstSourceFactoryLog) << "Failed because URI is not specified";
        return nullptr;
    }

    const guint latencyMs = (config.latencyMs < 0) ? 0u : static_cast<guint>(config.latencyMs);

    const QUrl sourceUrl(uri);
    const QString scheme = sourceUrl.scheme().toLower();

    const bool isRtsp = scheme.startsWith(QLatin1String("rtsp"));
    const bool isUdpH264 = (scheme == QLatin1String("udp"));
    const bool isUdpH265 = (scheme == QLatin1String("udp265"));
    const bool isUdpMPEGTS = (scheme == QLatin1String("mpegts"));
    const bool isTcpMPEGTS = (scheme == QLatin1String("tcp"));

    if (!isRtsp && !isUdpH264 && !isUdpH265 && !isUdpMPEGTS && !isTcpMPEGTS) {
        qCWarning(GstSourceFactoryLog) << "Unsupported URI scheme:" << scheme << "in" << sourceUrl.toDisplayString(QUrl::RemoveUserInfo);
        return nullptr;
    }

    // Owning locals until gst_bin_add*, then nulled (non-owning alias used downstream) so the
    // unconditional gst_clear_object cleanup at the bottom stays safe.
    GstElement* source = nullptr;
    GstElement* parser = nullptr;
    GstElement* rtpDepay = nullptr;
    GstElement* tsdemux = nullptr;
    GstElement* bin = nullptr;
    GstElement* srcbin = nullptr;

    do {
        if (isRtsp) {
            source = buildRtspSource(uri, sourceUrl, config, latencyMs);
        } else if (isTcpMPEGTS) {
            source = buildTcpSource(sourceUrl);
        } else {  // isUdpH264 || isUdpH265 || isUdpMPEGTS
            source = buildUdpSource(sourceUrl, isUdpH264, isUdpH265);
        }
        if (!source) {
            break;
        }

        bin = gst_bin_new("sourcebin");
        if (!bin) {
            qCCritical(GstSourceFactoryLog) << "gst_bin_new('sourcebin') failed";
            break;
        }

        parser = gst_element_factory_make(isUdpH265 ? "h265parse" : "parsebin", "parser");
        if (!parser) {
            qCCritical(GstSourceFactoryLog) << "gst_element_factory_make("
                                            << (isUdpH265 ? "'h265parse'" : "'parsebin'") << ") failed";
            break;
        }

        if (isUdpH265) {
            // UDP H.265 has no SDP/RTSP control plane. Make depayloading and parser config explicit
            // so hardware decoders, especially Android MediaCodec, see repeated VPS/SPS/PPS at IDR
            // boundaries instead of depending on parsebin's defaults after startup or reconnect.
            rtpDepay = gst_element_factory_make("rtph265depay", nullptr);
            if (!rtpDepay) {
                qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('rtph265depay') failed";
                break;
            }
            g_object_set(parser, "config-interval", -1, nullptr);
        }

        // Older Linux/system GStreamer misnegotiates parser->decoder caps; force avc/hvc1 there only.
        // Bundled 1.28+ SDKs do not need it, and the forced caps break byte-stream HW decoders.
#if defined(QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER)
        if (!isUdpH265) {
            (void) g_signal_connect(parser, "autoplug-query", G_CALLBACK(filterParserCaps), nullptr);
        }
#endif

        // Add individually so ownership is unambiguous on failure: gst_bin_add sinks the ref only
        // on success, so the local stays owning (and gets unref'd by the cleanup block) when it fails.
        if (!gst_bin_add(GST_BIN(bin), source)) {
            qCCritical(GstSourceFactoryLog) << "gst_bin_add(source) failed";
            break;
        }
        GstElement* upstream = source;
        source = nullptr;

        if (rtpDepay && !gst_bin_add(GST_BIN(bin), rtpDepay)) {
            qCCritical(GstSourceFactoryLog) << "gst_bin_add(rtph265depay) failed";
            break;
        }
        GstElement* depay = rtpDepay;
        rtpDepay = nullptr;

        if (!gst_bin_add(GST_BIN(bin), parser)) {
            qCCritical(GstSourceFactoryLog) << "gst_bin_add(parser) failed";
            break;
        }
        GstElement* binParser = parser;
        parser = nullptr;

        // Android can't determine MPEG2-TS via parsebin, so create tsdemux explicitly.
        if (isTcpMPEGTS || isUdpMPEGTS) {
            tsdemux = gst_element_factory_make("tsdemux", nullptr);
            if (!tsdemux) {
                qCCritical(GstSourceFactoryLog) << "gst_element_factory_make('tsdemux') failed";
                break;
            }

            if (!gst_bin_add(GST_BIN(bin), tsdemux)) {
                qCCritical(GstSourceFactoryLog) << "gst_bin_add(tsdemux) failed";
                break;
            }
            GstElement* demux = tsdemux;
            tsdemux = nullptr;

            if (!gst_element_link(upstream, demux)) {
                qCCritical(GstSourceFactoryLog) << "gst_element_link(source, tsdemux) failed";
                break;
            }

            upstream = demux;
        }

        if (isUdpH265) {
            if (!linkUdpRtpToDepayAndParser(bin, upstream, depay, binParser, config, latencyMs)) {
                break;
            }
            if (!addStaticGhostPad(binParser)) {
                break;
            }
        } else {
            if (!linkSourceToParser(bin, upstream, binParser, config, latencyMs, isTcpMPEGTS || isUdpMPEGTS, isRtsp)) {
                break;
            }
        }

        srcbin = bin;
        bin = nullptr;
    } while (0);

    gst_clear_object(&bin);
    gst_clear_object(&parser);
    gst_clear_object(&rtpDepay);
    gst_clear_object(&tsdemux);
    gst_clear_object(&source);

    return srcbin;
}

}  // namespace GStreamer::SourceFactory
