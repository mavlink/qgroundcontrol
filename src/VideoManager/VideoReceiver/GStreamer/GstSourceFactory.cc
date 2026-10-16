#include "GstSourceFactory.h"

#include <gst/gst.h>
#include <gst/rtsp/gstrtsptransport.h>
#include <utility>

#include <QtCore/QFile>
#include <QtCore/QUrl>

#include "GStreamerHelpers.h"
#include "GstScoped.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"

QGC_LOGGING_CATEGORY(GstSourceFactoryLog, "Video.GStreamer.GstSourceFactory")

namespace {

using namespace GStreamer;

constexpr guint64 kRtspTcpTimeoutUs = G_GUINT64_CONSTANT(5000000);
constexpr int kRtspRetry = 3;
constexpr int kUdpBufferSizeBytes = 8 * 1024 * 1024;

const char* elementFactoryName(GstElement* element)
{
    GstElementFactory* factory = gst_element_get_factory(element);
    return factory ? gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)) : nullptr;
}

bool isH26xRtpDepayloader(const char* factoryName)
{
    return (g_strcmp0(factoryName, "rtph264depay") == 0) || (g_strcmp0(factoryName, "rtph265depay") == 0);
}

void configureH26xParser(GstElement* element)
{
    const char* factoryName = elementFactoryName(element);
    if ((g_strcmp0(factoryName, "h264parse") == 0) || (g_strcmp0(factoryName, "h265parse") == 0)) {
        // Recording can begin long after RTSP setup. Repeat codec parameter sets at every IDR so
        // the late-opened recording branch can construct MP4/MOV codec_data without relying on
        // the camera to resend its startup-only VPS/SPS/PPS or SPS/PPS.
        g_object_set(element, "config-interval", -1, nullptr);
    }
}

// Joining a stream mid-GOP, slices before the next parameter sets can't be decoded. GStreamer 1.28's h264parse warns
// for every such slice and, when converting byte-stream to avc for the decoder, fails the pipeline with "No caps set"
// if it ever pushes an access unit before it has both SPS and PPS, e.g. an SPS whose PPS a jitter buffer dropped.
// The depayloader's DELTA_UNIT flag isn't enough either: a sliced encoder's IDR can be joined after its SPS/PPS.
struct NalSummary
{
    bool vps = false;
    bool sps = false;
    bool pps = false;
    bool slice = false;
};

NalSummary summarizeNals(GstBuffer* buffer, bool h265)
{
    NalSummary summary;
    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        return summary;
    }
    const guint8* const data = map.data;
    for (gsize i = 0; (i + 3) < map.size; ++i) {
        if ((data[i] != 0) || (data[i + 1] != 0) || (data[i + 2] != 1)) {
            continue;
        }
        const guint8 header = data[i + 3];
        if (h265) {
            const guint nalType = (header >> 1) & 0x3FU;
            summary.vps |= (nalType == 32U);
            summary.sps |= (nalType == 33U);
            summary.pps |= (nalType == 34U);
            summary.slice |= (nalType <= 31U);
        } else {
            const guint nalType = header & 0x1FU;
            summary.sps |= (nalType == 7U);
            summary.pps |= (nalType == 8U);
            summary.slice |= ((nalType >= 1U) && (nalType <= 5U));
        }
        i += 2;
    }
    gst_buffer_unmap(buffer, &map);
    return summary;
}

bool outputsByteStream(GstPad* pad)
{
    const GstCapsPtr caps = adoptCaps(gst_pad_get_current_caps(pad));
    const GstStructure* structure =
        (caps && !gst_caps_is_empty(caps.get())) ? gst_caps_get_structure(caps.get(), 0) : nullptr;
    return structure && (g_strcmp0(gst_structure_get_string(structure, "stream-format"), "byte-stream") == 0);
}

/// The latest parameter-set buffers seen while the gate is closed.
struct ParameterSetGate
{
    explicit ParameterSetGate(bool isH265)
        : h265(isH265)
    {}

    ~ParameterSetGate()
    {
        gst_clear_buffer(&vps);
        gst_clear_buffer(&sps);
        gst_clear_buffer(&pps);
    }

    ParameterSetGate(const ParameterSetGate&) = delete;
    ParameterSetGate& operator=(const ParameterSetGate&) = delete;

    const bool h265;
    GstBuffer* vps = nullptr;
    GstBuffer* sps = nullptr;
    GstBuffer* pps = nullptr;
};

/// Prepends @p prefix's memories to @p buffer, keeping @p buffer's timestamps and flags.
GstBuffer* prependMemories(GstBuffer* buffer, GstBuffer* prefix)
{
    buffer = gst_buffer_make_writable(buffer);
    for (guint i = gst_buffer_n_memory(prefix); i > 0; --i) {
        gst_buffer_prepend_memory(buffer, gst_buffer_get_memory(prefix, i - 1));
    }
    return buffer;
}

GstPadProbeReturn holdUntilParameterSets(GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer) {
        return GST_PAD_PROBE_OK;
    }
    // Length-prefixed avc/hvc1 output carries its parameter sets in the caps' codec_data.
    if (!outputsByteStream(pad)) {
        return GST_PAD_PROBE_REMOVE;
    }

    auto* gate = static_cast<ParameterSetGate*>(data);
    const NalSummary nals = summarizeNals(buffer, gate->h265);
    if (!nals.slice) {
        if (nals.vps) {
            gst_buffer_replace(&gate->vps, buffer);
        }
        if (nals.sps) {
            gst_buffer_replace(&gate->sps, buffer);
        }
        if (nals.pps) {
            gst_buffer_replace(&gate->pps, buffer);
        }
        return GST_PAD_PROBE_DROP;
    }

    const bool complete = (nals.sps || gate->sps) && (nals.pps || gate->pps) && (!gate->h265 || nals.vps || gate->vps);
    if (!complete) {
        return GST_PAD_PROBE_DROP;
    }

    // Prepend, in VPS/SPS/PPS order, the stored sets this buffer lacks; one buffer may hold several of them.
    GstBuffer* const stored[] = {(gate->h265 && !nals.vps) ? gate->vps : nullptr, nals.sps ? nullptr : gate->sps,
                                 nals.pps ? nullptr : gate->pps};
    for (int i = 2; i >= 0; --i) {
        const bool duplicate = (i > 0 && stored[i] == stored[i - 1]) || (i > 1 && stored[i] == stored[i - 2]);
        if (stored[i] && !duplicate) {
            buffer = prependMemories(buffer, stored[i]);
        }
    }
    GST_PAD_PROBE_INFO_DATA(info) = buffer;
    return GST_PAD_PROBE_REMOVE;
}

void configureH26xRtpDepayloader(GstElement* element)
{
    const char* factoryName = elementFactoryName(element);
    if (!isH26xRtpDepayloader(factoryName)) {
        return;
    }
    if (const GstPadPtr srcPad = adoptPad(gst_element_get_static_pad(element, "src"))) {
        GStreamer::SourceFactory::gateOnParameterSets(srcPad.get(), g_strcmp0(factoryName, "rtph265depay") == 0);
    }
}

void configureAutopluggedElement([[maybe_unused]] GstBin* bin, [[maybe_unused]] GstBin* subBin, GstElement* element,
                                 [[maybe_unused]] gpointer data)
{
    configureH26xParser(element);
    configureH26xRtpDepayloader(element);
}

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

    GstCapsPtr sinkCaps;
    GstStructure* structure = gst_caps_get_structure(srcCaps, 0);
    if (gst_structure_has_name(structure, "video/x-h265")) {
        const GstCapsPtr filter = adoptCaps(gst_caps_from_string("video/x-h265"));
        if (gst_caps_can_intersect(srcCaps, filter.get())) {
            sinkCaps = adoptCaps(gst_caps_from_string("video/x-h265,stream-format=hvc1"));
        }
    } else if (gst_structure_has_name(structure, "video/x-h264")) {
        const GstCapsPtr filter = adoptCaps(gst_caps_from_string("video/x-h264"));
        if (gst_caps_can_intersect(srcCaps, filter.get())) {
            sinkCaps = adoptCaps(gst_caps_from_string("video/x-h264,stream-format=avc"));
        }
    }

    if (sinkCaps) {
        gst_query_set_caps_result(query, sinkCaps.get());
        return TRUE;
    }

    return FALSE;
}
#endif

void wrapWithGhostPad(GstElement* element, GstPad* pad, [[maybe_unused]] gpointer data)
{
    const GCharPtr name = adoptGChar(gst_pad_get_name(pad));
    if (!name) {
        qCWarning(GstSourceFactoryLog) << "gst_pad_get_name() failed";
        return;
    }

    GstPadPtr ghostpad = adoptPad(gst_ghost_pad_new(name.get(), pad));
    if (!ghostpad) {
        qCWarning(GstSourceFactoryLog) << "gst_ghost_pad_new() failed";
        return;
    }

    (void) gst_pad_set_active(ghostpad.get(), TRUE);

    // gst_object_get_parent takes a ref (GST_ELEMENT_PARENT does not) — guards against a
    // concurrent bin teardown finalizing the parent before gst_element_add_pad.
    const GstObjectPtr parent = adoptObject(gst_object_get_parent(GST_OBJECT(element)));
    if (!parent) {
        qCWarning(GstSourceFactoryLog) << "Element has no parent; bin already torn down?";
        return;
    }

    if (!gst_element_add_pad(GST_ELEMENT(parent.get()), ghostpad.release())) {
        qCWarning(GstSourceFactoryLog) << "gst_element_add_pad() failed";
    }
}

gboolean padProbe([[maybe_unused]] GstElement* element, GstPad* pad, gpointer user_data)
{
    int* probeRes = static_cast<int*>(user_data);
    *probeRes |= 1;

    const GstCapsPtr filter = adoptCaps(gst_caps_from_string("application/x-rtp"));
    const GstCapsPtr caps = adoptCaps(gst_pad_query_caps(pad, nullptr));
    if (filter && caps && !gst_caps_is_any(caps.get()) && gst_caps_can_intersect(caps.get(), filter.get())) {
        *probeRes |= 2;
    }

    return TRUE;
}

bool addStaticGhostPad(GstElement* element)
{
    const GstPadPtr srcPad = adoptPad(gst_element_get_static_pad(element, "src"));
    if (!srcPad) {
        qCWarning(GstSourceFactoryLog) << "gst_element_get_static_pad('src') failed";
        return false;
    }

    wrapWithGhostPad(element, srcPad.get(), nullptr);
    return true;
}

bool validPort(int port)
{
    return port > 0 && port <= 65535;
}

// SDP-derived caps carry the stream's media type; audio and metadata streams must never reach the video parser.
bool isVideoMedia(const GstStructure* structure)
{
    return g_strcmp0(gst_structure_get_string(structure, "media"), "video") == 0;
}

bool isVideoRtp(const GstStructure* structure)
{
    return gst_structure_has_name(structure, "application/x-rtp") && isVideoMedia(structure);
}

}  // namespace

namespace GStreamer::SourceFactory {

namespace {

// Fixed rtx timing instead of the auto(-1) heuristic, which can stack retries and balloon playout
// latency: bounded recovery latency over a lossy RF link.
constexpr int kRtxDelayMs = 25;
constexpr int kRtxMaxRetries = 1;

// Shared by the static and dynamic (pad-added) link paths so both apply the identical jitter-buffer
// policy. Retransmission stays off: these sources have no RTCP back-channel, so the jitterbuffer's
// retransmission requests would only reach the source element, which drops them.
GstElement* addJitterBuffer(GstElement* bin, guint latencyMs)
{
    GstElementPtr buffer = adoptElement(gst_element_factory_make("rtpjitterbuffer", nullptr));
    if (!buffer) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('rtpjitterbuffer') failed";
        return nullptr;
    }

    g_object_set(buffer.get(), "latency", latencyMs, "do-lost", TRUE, "drop-on-latency", TRUE, nullptr);
    return addToBin(bin, std::move(buffer), "rtpjitterbuffer");
}

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

// Heap-owned context for the dynamic (pad-added) link path; freed when @binParser is finalized.
struct DynamicLinkContext
{
    GstElement* binParser;
    guint latencyMs;
    bool insertJitterBuffer;  // false for RTSP (rtspsrc has its own internal jitterbuffer) and JitterBuffer::None
};

// Inserts a running rtpjitterbuffer linked to the parser; returns it (bin-owned) or nullptr to link directly.
GstElement* insertDynamicJitterBuffer(const DynamicLinkContext* ctx)
{
    const GstObjectPtr bin = adoptObject(gst_object_get_parent(GST_OBJECT(ctx->binParser)));
    if (!bin) {
        return nullptr;
    }

    GstElement* buffer = addJitterBuffer(GST_ELEMENT(bin.get()), ctx->latencyMs);
    if (!buffer) {
        qCWarning(GstSourceFactoryLog) << "dynamic rtpjitterbuffer create/add failed; skipping jitter buffer";
        return nullptr;
    }

    if (!gst_element_sync_state_with_parent(buffer) || !gst_element_link(buffer, ctx->binParser)) {
        qCWarning(GstSourceFactoryLog) << "dynamic rtpjitterbuffer link/sync failed; linking pad straight to parser";
        disposeChild(buffer);
        return nullptr;
    }

    return buffer;
}

void linkPad(GstElement* element, GstPad* pad, gpointer data)
{
    const auto* ctx = static_cast<const DynamicLinkContext*>(data);

    // tsdemux/rtspsrc fire pad-added for audio/data pads too; only link video src pads so a
    // non-video pad can't claim the parser ahead of the real video pad.
    if (GST_PAD_DIRECTION(pad) != GST_PAD_SRC) {
        return;
    }

    bool isVideo = false;
    bool isRtp = false;
    GstCapsPtr caps = adoptCaps(gst_pad_get_current_caps(pad));
    if (!caps) {
        caps = adoptCaps(gst_pad_query_caps(pad, nullptr));
    }
    if (caps) {
        const guint n = gst_caps_get_size(caps.get());
        for (guint i = 0; i < n; ++i) {
            const GstStructure* st = gst_caps_get_structure(caps.get(), i);
            if (isVideoRtp(st)) {
                // The depayloader/parsebin downstream classifies the video payload.
                isRtp = true;
                isVideo = true;
            } else if (g_str_has_prefix(gst_structure_get_name(st), "video/")) {
                isVideo = true;
            }
        }
    }
    if (!isVideo) {
        return;
    }

    // Link only the first matching video pad. A second pad-added (duplicate RTP pad, or a later
    // tsdemux elementary stream) must not stack a second jitterbuffer or re-link an occupied sink.
    const GstPadPtr parserSink = adoptPad(gst_element_get_static_pad(ctx->binParser, "sink"));
    if (parserSink && gst_pad_is_linked(parserSink.get())) {
        return;
    }

    // Insert a jitterbuffer ahead of the parser for RTP pads on non-RTSP dynamic sources, matching the
    // static path's policy. RTSP is excluded: rtspsrc owns an internal jitterbuffer (no double-buffering).
    GstElement* dynamicBuffer = nullptr;
    if (isRtp && ctx->insertJitterBuffer) {
        dynamicBuffer = insertDynamicJitterBuffer(ctx);
    }
    GstElement* const downstream = dynamicBuffer ? dynamicBuffer : ctx->binParser;

    const GCharPtr name = adoptGChar(gst_pad_get_name(pad));
    if (!name || !gst_element_link_pads(element, name.get(), downstream, "sink")) {
        qCWarning(GstSourceFactoryLog) << "gst_element_link_pads() failed for" << (name ? name.get() : "(unnamed pad)");
        if (dynamicBuffer) {
            disposeChild(dynamicBuffer);
        }
    }
}

// Each build* helper makes and configures the source element for one scheme family, logs the
// specific failure, and returns the owning element (or nullptr). create() owns it from here on.

GstElementPtr buildRtspSource(const QString& uri, const QUrl& sourceUrl, const Config& config, guint latencyMs)
{
    if (!GStreamer::isValidRtspUri(uri.toUtf8().constData())) {
        qCWarning(GstSourceFactoryLog) << "Invalid RTSP URI:" << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }

    GstElementPtr source = adoptElement(gst_element_factory_make("rtspsrc", "source"));
    if (!source) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('rtspsrc') failed";
        return nullptr;
    }

    QUrl cleanUrl(sourceUrl);
    cleanUrl.setUserInfo(QString());
    const QByteArray cleanLocation = cleanUrl.toEncoded();

    // protocols mask enables TCP-interleaved fallback when UDP is blocked; without it
    // firewalled networks hang until tcp-timeout instead of negotiating TCP.
    constexpr GstRTSPLowerTrans kRtspProtocols =
        static_cast<GstRTSPLowerTrans>(GST_RTSP_LOWER_TRANS_UDP | GST_RTSP_LOWER_TRANS_TCP);

    // rtspsrc always owns an internal jitterbuffer, so None maps to zero playout latency and no retransmission.
    const guint rtspLatencyMs = (config.jitterBuffer == JitterBuffer::None) ? 0u : latencyMs;
    const gboolean doRetransmission =
        ((config.jitterBuffer != JitterBuffer::None) && config.doRetransmission) ? TRUE : FALSE;
    g_object_set(source.get(), "location", cleanLocation.constData(), "latency", rtspLatencyMs, "do-rtcp", TRUE,
                 "do-retransmission", doRetransmission, "tcp-timeout", kRtspTcpTimeoutUs, "udp-reconnect", TRUE,
                 "drop-on-latency", TRUE, "retry", kRtspRetry, "protocols", kRtspProtocols, nullptr);

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

GstElementPtr buildTcpSource(const QUrl& sourceUrl)
{
    const int port = sourceUrl.port();
    if (!validPort(port)) {
        qCWarning(GstSourceFactoryLog) << "Invalid TCP port" << port << "in"
                                       << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }
    const QString host = sourceUrl.host();
    if (host.isEmpty()) {
        qCWarning(GstSourceFactoryLog) << "Missing host in TCP URI"
                                       << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }

    GstElementPtr source = adoptElement(gst_element_factory_make("tcpclientsrc", "source"));
    if (!source) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('tcpclientsrc') failed";
        return nullptr;
    }

    g_object_set(source.get(), "host", host.toUtf8().constData(), "port", static_cast<int>(port), nullptr);
    return source;
}

GstElementPtr buildUdpSource(const QUrl& sourceUrl, bool isUdpH264, bool isUdpH265)
{
    const int port = sourceUrl.port();
    if (!validPort(port)) {
        qCWarning(GstSourceFactoryLog) << "Invalid UDP port" << port << "in"
                                       << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }

    GstElementPtr source = adoptElement(gst_element_factory_make("udpsrc", "source"));
    if (!source) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('udpsrc') failed";
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
    g_object_set(source.get(), "uri", udpUri.constData(), "buffer-size", udpBufferSize, nullptr);

    if (isUdpH264 || isUdpH265) {
        const GstCapsPtr caps = adoptCaps(gst_caps_from_string(
            isUdpH264 ? "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264"
                      : "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H265"));
        if (!caps) {
            qCCritical(GstSourceFactoryLog) << "gst_caps_from_string() failed";
            return nullptr;
        }
        g_object_set(source.get(), "caps", caps.get(), nullptr);
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
            GstElement* buffer = addJitterBuffer(bin, latencyMs);
            if (!buffer) {
                return false;
            }
            // bin owns buffer now; a link failure below is cleaned up via the caller's bin teardown.
            if (!gst_element_link_many(upstream, buffer, binParser, nullptr)) {
                qCWarning(GstSourceFactoryLog) << "gst_element_link_many(source, rtpjitterbuffer, parser) failed";
                return false;
            }
        } else {
            if (!gst_element_link(upstream, binParser)) {
                qCWarning(GstSourceFactoryLog) << "gst_element_link(source, parser) failed";
                return false;
            }
        }
    } else {
        // linkPad applies the jitter-buffer policy itself when an application/x-rtp pad appears on a
        // non-RTSP source. Context lifetime is tied to binParser (freed on its finalize).
        auto* ctx =
            new DynamicLinkContext{binParser, latencyMs, !isRtsp && (config.jitterBuffer != JitterBuffer::None)};
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
            qCWarning(GstSourceFactoryLog) << "gst_element_link_many(source, depay, parser) failed";
            return false;
        }
        return true;
    }

    GstElement* buffer = addJitterBuffer(bin, latencyMs);
    if (!buffer) {
        return false;
    }

    if (!gst_element_link_many(source, buffer, depay, parser, nullptr)) {
        qCWarning(GstSourceFactoryLog) << "gst_element_link_many(source, rtpjitterbuffer, depay, parser) failed";
        return false;
    }

    return true;
}

}  // namespace

GstElement* create(const QString& uri, const Config& config)
{
    if (uri.isEmpty()) {
        qCWarning(GstSourceFactoryLog) << "Failed because URI is not specified";
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
        qCWarning(GstSourceFactoryLog) << "Unsupported URI scheme:" << scheme << "in"
                                      << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }

    GstElementPtr source;
    if (isRtsp) {
        source = buildRtspSource(uri, sourceUrl, config, latencyMs);
    } else if (isTcpMPEGTS) {
        source = buildTcpSource(sourceUrl);
    } else {  // isUdpH264 || isUdpH265 || isUdpMPEGTS
        source = buildUdpSource(sourceUrl, isUdpH264, isUdpH265);
    }
    if (!source) {
        return nullptr;
    }

    GstElementPtr bin = adoptElement(gst_bin_new("sourcebin"));
    if (!bin) {
        qCWarning(GstSourceFactoryLog) << "gst_bin_new('sourcebin') failed";
        return nullptr;
    }

    const char* const parserFactory = isUdpH265 ? "h265parse" : "parsebin";
    GstElementPtr parser = adoptElement(gst_element_factory_make(parserFactory, "parser"));
    if (!parser) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make(" << parserFactory << ") failed";
        return nullptr;
    }

    GstElementPtr rtpDepay;
    if (isUdpH265) {
        // UDP H.265 has no SDP/RTSP control plane. Make depayloading and parser config explicit
        // so hardware decoders, especially Android MediaCodec, see repeated VPS/SPS/PPS at IDR
        // boundaries instead of depending on parsebin's defaults after startup or reconnect.
        rtpDepay = adoptElement(gst_element_factory_make("rtph265depay", nullptr));
        if (!rtpDepay) {
            qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('rtph265depay') failed";
            return nullptr;
        }
        configureH26xParser(parser.get());
        configureH26xRtpDepayloader(rtpDepay.get());
    } else {
        // parsebin creates the depayloader and codec parser only after it sees the stream caps. Configure them
        // as soon as they are autoplugged, before data flows.
        (void) g_signal_connect(parser.get(), "deep-element-added", G_CALLBACK(configureAutopluggedElement), nullptr);
    }

    // Older Linux/system GStreamer misnegotiates parser->decoder caps; force avc/hvc1 there only.
    // Bundled 1.28+ SDKs do not need it, and the forced caps break byte-stream HW decoders.
#if defined(QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER)
    if (!isUdpH265) {
        (void) g_signal_connect(parser.get(), "autoplug-query", G_CALLBACK(filterParserCaps), nullptr);
    }
#endif

    // From here on the bin owns every element; the borrowed pointers stay valid while bin is alive.
    GstElement* upstream = addToBin(bin.get(), std::move(source), "source");
    if (!upstream) {
        return nullptr;
    }

    GstElement* depay = nullptr;
    if (rtpDepay) {
        depay = addToBin(bin.get(), std::move(rtpDepay), "rtph265depay");
        if (!depay) {
            return nullptr;
        }
    }

    GstElement* const binParser = addToBin(bin.get(), std::move(parser), "parser");
    if (!binParser) {
        return nullptr;
    }

    // Android can't determine MPEG2-TS via parsebin, so create tsdemux explicitly.
    if (isTcpMPEGTS || isUdpMPEGTS) {
        GstElementPtr tsdemux = adoptElement(gst_element_factory_make("tsdemux", nullptr));
        if (!tsdemux) {
            qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('tsdemux') failed";
            return nullptr;
        }

        GstElement* const demux = addToBin(bin.get(), std::move(tsdemux), "tsdemux");
        if (!demux) {
            return nullptr;
        }

        if (!gst_element_link(upstream, demux)) {
            qCWarning(GstSourceFactoryLog) << "gst_element_link(source, tsdemux) failed";
            return nullptr;
        }

        upstream = demux;
    }

    if (isUdpH265) {
        if (!linkUdpRtpToDepayAndParser(bin.get(), upstream, depay, binParser, config, latencyMs)) {
            return nullptr;
        }
        if (!addStaticGhostPad(binParser)) {
            return nullptr;
        }
    } else {
        if (!linkSourceToParser(bin.get(), upstream, binParser, config, latencyMs, isTcpMPEGTS || isUdpMPEGTS,
                                isRtsp)) {
            return nullptr;
        }
    }

    return bin.release();
}

void gateOnParameterSets(GstPad* depayloaderSrc, bool h265)
{
    (void) gst_pad_add_probe(depayloaderSrc, GST_PAD_PROBE_TYPE_BUFFER, holdUntilParameterSets,
                             new ParameterSetGate(h265),
                             [](gpointer gate) { delete static_cast<ParameterSetGate*>(gate); });
}

}  // namespace GStreamer::SourceFactory
