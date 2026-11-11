#include "GstSourceFactory.h"

#include <algorithm>
#include <chrono>
#include <gst/gst.h>
#include <optional>
#include <utility>

#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>

#include "GStreamerHelpers.h"
#include "GstH26xParameterSetGate.h"
#include "GstRtpPacketProbe.h"
#include "GstRtspSource.h"
#include "GstScoped.h"
#include "QGCLoggingCategory.h"
#include "QGCNetworkHelper.h"
#include "VideoStreamUri.h"

QGC_LOGGING_CATEGORY(GstSourceFactoryLog, "Video.GStreamer.GstSourceFactory")

namespace {

using namespace GStreamer;

constexpr int kUdpBufferSizeBytes = 8 * 1024 * 1024;
// tcpclientsrc connects inside the state change, on the receiver's worker, and without a timeout waits as long as the
// OS does (minutes for a host that drops the SYN). The same time bounds a read, which is how long the receiver waits
// for a TCP stream that went silent anyway.
constexpr guint kTcpTimeoutSeconds = 3;
// The least latency an MPEG-TS stream is given. tsdemux pushes a video frame only once the next one starts (its PES
// packet needn't tell its length), so a frame is a frame interval late however steady the network is: this covers
// streams down to 10 fps.
constexpr guint kTsDemuxMinLatencyMs = 100;
// Of the RTP timestamps of video, H.264 (RFC 6184) and H.265 (RFC 7798) included.
constexpr int kRtpVideoClockRate = 90000;

/// @p data: non-null when the source is RTSP.
void configureAutopluggedElement([[maybe_unused]] GstBin* bin, [[maybe_unused]] GstBin* subBin, GstElement* element,
                                 gpointer data)
{
    SourceFactory::configureH26xParser(element);
    SourceFactory::configureH26xRtpDepayloader(element, data != nullptr);
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

bool isVideoRtp(const GstStructure* structure)
{
    return gst_structure_has_name(structure, "application/x-rtp") && SourceFactory::isVideoMedia(structure);
}

}  // namespace

namespace GStreamer::SourceFactory {

namespace {

using VideoStreamUri::Transport;

enum class SourceElement : uint8_t
{
    Rtsp,       ///< rtspsrc, which runs its own jitterbuffer
    TcpClient,  ///< tcpclientsrc
    Udp,        ///< udpsrc
};

/// The codec of RTP arriving on a plain UDP port, which the URI's scheme names as there is no SDP to tell.
struct RtpCodec
{
    const char* encodingName;  ///< In the caps udpsrc announces
    const char* depayloader;
    const char* parser;
    const char* byteStreamCaps;  ///< Between the two
};

constexpr RtpCodec kRtpH264{"H264", "rtph264depay", "h264parse",
                            "video/x-h264, stream-format=(string)byte-stream, alignment=(string)au"};
constexpr RtpCodec kRtpH265{"H265", "rtph265depay", "h265parse",
                            "video/x-h265, stream-format=(string)byte-stream, alignment=(string)au"};

// What create() builds for one Transport.
struct TransportSpec
{
    SourceElement element;
    const RtpCodec* rtpCodec;  ///< RTP on udpsrc, depayloaded and parsed for this codec; nullptr leaves it to parsebin
    bool demuxMpegTs;          ///< tsdemux ahead of the parser: Android's parsebin can't detect MPEG-TS
};

constexpr TransportSpec specFor(Transport transport)
{
    switch (transport) {
        case Transport::Rtsp:
            return {SourceElement::Rtsp, nullptr, false};
        case Transport::RtpUdpH264:
            return {SourceElement::Udp, &kRtpH264, false};
        case Transport::RtpUdpH265:
            return {SourceElement::Udp, &kRtpH265, false};
        case Transport::MpegTsUdp:
            return {SourceElement::Udp, nullptr, true};
        case Transport::MpegTsTcp:
            return {SourceElement::TcpClient, nullptr, true};
    }
    return {SourceElement::Udp, nullptr, false};
}

// Shared by the static and dynamic (pad-added) link paths so both apply the identical jitter-buffer
// policy. Retransmission stays off: these sources have no RTCP back-channel, so the jitterbuffer's
// retransmission requests would only reach the source element, which drops them.
GstElement* addJitterBuffer(GstElement* bin, guint latencyMs)
{
    GstElementPtr buffer = makeElement("rtpjitterbuffer");
    if (!buffer) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('rtpjitterbuffer') failed";
        return nullptr;
    }

    // No "do-lost", as in rtspsrc's jitter buffers: nothing downstream acts on the GAP events it brings (the
    // depayloader tells loss from the DISCONT flag, and the lost packets are counted either way), while one that
    // reaches a parser ahead of the stream's parameter sets has it announce caps without them.
    g_object_set(buffer.get(), "latency", latencyMs, "drop-on-latency", TRUE, nullptr);
    reportDroppedPackets(buffer.get());
    return addToBin(bin, std::move(buffer), "rtpjitterbuffer");
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

GstElementPtr buildTcpSource(const QUrl& sourceUrl)
{
    const int port = sourceUrl.port();
    if (!VideoStreamUri::isValidPort(port)) {
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

    GstElementPtr source = makeElement("tcpclientsrc", "source");
    if (!source) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('tcpclientsrc') failed";
        return nullptr;
    }

    g_object_set(source.get(), "host", host.toUtf8().constData(), "port", static_cast<int>(port), "timeout",
                 kTcpTimeoutSeconds, nullptr);
    return source;
}

GstElementPtr buildUdpSource(const QUrl& sourceUrl, const char* rtpEncoding)
{
    // Port 0 binds a free port, which udpsrc then reports in its "port" property.
    const int port = sourceUrl.port();
    if ((port != 0) && !VideoStreamUri::isValidPort(port)) {
        qCWarning(GstSourceFactoryLog) << "Invalid UDP port" << port << "in"
                                       << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }

    GstElementPtr source = makeElement("udpsrc", "source");
    if (!source) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('udpsrc') failed";
        return nullptr;
    }

    // Normalize udp265:///mpegts:// to udp:// for udpsrc, preserving query params.
    QUrl udpUrl(sourceUrl);
    udpUrl.setScheme(QStringLiteral("udp"));
    udpUrl.setUserInfo(QString());
    // udpsrc doesn't report a URI without a host as the error it takes it for, and listens on its default port, 5004.
    if (udpUrl.host().isEmpty()) {
        udpUrl.setHost(QStringLiteral("0.0.0.0"));
    }
    const QByteArray udpUri = udpUrl.toEncoded();
    // udpsrc shares its port by default, and of two listeners on a unicast address only one gets a stream's packets,
    // the other nothing and no error: a port in use has to fail to bind. The members of a multicast group do share it.
    const gboolean sharePort = QHostAddress(udpUrl.host()).isMulticast() ? TRUE : FALSE;

    // Nothing reads the sender's address, which udpsrc otherwise allocates a meta for with every packet.
    g_object_set(source.get(), "uri", udpUri.constData(), "buffer-size", udpReceiveBufferSize(), "reuse", sharePort,
                 "retrieve-sender-address", FALSE, nullptr);

    if (rtpEncoding) {
        const GstCapsPtr caps = adoptCaps(gst_caps_new_simple("application/x-rtp", "media", G_TYPE_STRING, "video",
                                                              "clock-rate", G_TYPE_INT, kRtpVideoClockRate,
                                                              "encoding-name", G_TYPE_STRING, rtpEncoding, nullptr));
        g_object_set(source.get(), "caps", caps.get(), nullptr);
    }
    return source;
}

struct ParserStage
{
    GstElementPtr parser;
    GstElementPtr depay;  ///< Set only for TransportSpec::rtpCodec
};

// The parser (and, for RTP over UDP, the depayloader) for @p spec, configured but not yet in a bin.
ParserStage buildParserStage(const TransportSpec& spec, bool isRtsp)
{
    const char* const parserFactory = spec.rtpCodec ? spec.rtpCodec->parser : "parsebin";
    ParserStage stage{makeElement(parserFactory, "parser"), {}};
    if (!stage.parser) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make(" << parserFactory << ") failed";
        return {};
    }

    if (spec.rtpCodec) {
        // RTP over UDP has no SDP/RTSP control plane. Make depayloading and parser config explicit
        // so hardware decoders, especially Android MediaCodec, see repeated parameter sets at IDR
        // boundaries instead of depending on parsebin's defaults after startup or reconnect.
        stage.depay = makeElement(spec.rtpCodec->depayloader);
        if (!stage.depay) {
            qCWarning(GstSourceFactoryLog) << "gst_element_factory_make(" << spec.rtpCodec->depayloader << ") failed";
            return {};
        }
        configureH26xParser(stage.parser.get());
        configureH26xRtpDepayloader(stage.depay.get(), false);
        return stage;
    }

    // parsebin creates the depayloader and codec parser only after it sees the stream caps. Configure them
    // as soon as they are autoplugged, before data flows.
    (void) g_signal_connect(stage.parser.get(), "deep-element-added", G_CALLBACK(configureAutopluggedElement),
                            isRtsp ? GINT_TO_POINTER(TRUE) : nullptr);

    // Older Linux/system GStreamer misnegotiates parser->decoder caps; force avc/hvc1 there only.
    // Bundled 1.28+ SDKs do not need it, and the forced caps break byte-stream HW decoders.
#if defined(QGC_GST_ENABLE_LEGACY_PARSEBIN_CAPS_FILTER)
    (void) g_signal_connect(stage.parser.get(), "autoplug-query", G_CALLBACK(filterParserCaps), nullptr);
#endif
    return stage;
}

// Android can't determine MPEG2-TS via parsebin, so MPEG-TS sources get an explicit tsdemux. Returns the bin-owned
// demuxer linked after @p upstream, or nullptr (logged).
GstElement* addTsDemux(GstElement* bin, GstElement* upstream, guint latencyMs)
{
    GstElementPtr tsdemux = makeElement("tsdemux");
    if (!tsdemux) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make('tsdemux') failed";
        return nullptr;
    }

    // tsdemux adds its "latency" to a live stream's: with its default, 700 ms, every frame of MPEG-TS over UDP is
    // shown that much later.
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(tsdemux.get()), "latency")) {
        g_object_set(tsdemux.get(), "latency", static_cast<gint>(std::max(latencyMs, kTsDemuxMinLatencyMs)), nullptr);
    }

    GstElement* const demux = addToBin(bin, std::move(tsdemux), "tsdemux");
    if (!demux) {
        return nullptr;
    }

    if (!gst_element_link(upstream, demux)) {
        qCWarning(GstSourceFactoryLog) << "gst_element_link(source, tsdemux) failed";
        return nullptr;
    }
    return demux;
}

// Wires upstream → (optional rtpjitterbuffer) → binParser once upstream's video pad appears: rtspsrc and tsdemux
// have no source pad before they stream.
void linkSourceToParserOnPadAdded(GstElement* upstream, GstElement* binParser, const Config& config, guint latencyMs,
                                  bool isRtsp)
{
    // linkPad applies the jitter-buffer policy itself when an application/x-rtp pad appears on a
    // non-RTSP source. Context lifetime is tied to binParser (freed on its finalize).
    auto* ctx = new DynamicLinkContext{binParser, latencyMs, !isRtsp && (config.jitterBuffer != JitterBuffer::None)};
    g_object_set_data_full(G_OBJECT(binParser), "qgc-dynamic-link-ctx", ctx,
                           [](gpointer p) { delete static_cast<DynamicLinkContext*>(p); });
    (void) g_signal_connect(upstream, "pad-added", G_CALLBACK(linkPad), ctx);

    // pad-added fires on the streaming thread after create() returns the NULL-state bin, so it
    // can't race construction; wrapWithGhostPad ref-checks the parent against teardown.
    (void) g_signal_connect(binParser, "pad-added", G_CALLBACK(wrapWithGhostPad), nullptr);
}

bool linkUdpRtpToDepayAndParser(GstElement* bin, GstElement* source, GstElement* depay, GstElement* parser,
                                const RtpCodec& codec, const Config& config, guint latencyMs)
{
    GstElement* upstream = source;
    if (config.jitterBuffer != JitterBuffer::None) {
        GstElement* const buffer = addJitterBuffer(bin, latencyMs);
        if (!buffer) {
            return false;
        }
        if (!gst_element_link(source, buffer)) {
            qCWarning(GstSourceFactoryLog) << "gst_element_link(source, rtpjitterbuffer) failed";
            return false;
        }
        upstream = buffer;
    } else {
        // A jitter buffer counts the packets it is given, lost ones included. Without one the bin counts them itself,
        // where they arrive, so the stream's statistics tell packet loss in this mode too.
        const GstPadPtr sourcePad = adoptPad(gst_element_get_static_pad(source, "src"));
        if (!sourcePad || !countRtpPackets(bin, sourcePad.get(), kRtpVideoClockRate)) {
            qCWarning(GstSourceFactoryLog) << "Unable to count the RTP packets: no packet loss statistics";
        }
    }

    if (!gst_element_link(upstream, depay)) {
        qCWarning(GstSourceFactoryLog) << "gst_element_link(upstream, depay) failed";
        return false;
    }

    // Left to negotiate, the depayloader may pick the length-prefixed format (avc, hvc1), whose caps it only sets once
    // in-band parameter sets arrive, so its segment can go out first (GStreamer 1.28 warns) and the parameter-set
    // gate, which handles byte-stream, stands down. Byte-stream caps are set as soon as the RTP caps arrive.
    const GstCapsPtr byteStream = adoptCaps(gst_caps_from_string(codec.byteStreamCaps));
    if (!byteStream || !gst_element_link_filtered(depay, parser, byteStream.get())) {
        qCWarning(GstSourceFactoryLog) << "gst_element_link_filtered(depay, parser, byte-stream) failed";
        return false;
    }

    return true;
}

}  // namespace

int udpReceiveBufferSize()
{
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
    return udpBufferSize;
}

void reportDroppedPackets(GstElement* jitterBuffer)
{
    // GStreamer 1.18. A message for every packet, rather than the default of one every 200 ms that tells of those
    // dropped since the last: the rest of a burst would only be told with the next one, which may never come.
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(jitterBuffer), "post-drop-messages")) {
        g_object_set(jitterBuffer, "post-drop-messages", TRUE, "drop-messages-interval", 0u, nullptr);
    }
}

GstElement* create(const QString& uri, const Config& config)
{
    if (uri.isEmpty()) {
        qCWarning(GstSourceFactoryLog) << "Failed because URI is not specified";
        return nullptr;
    }

    const guint latencyMs = (config.latencyMs < 0) ? 0u : static_cast<guint>(config.latencyMs);

    const QUrl sourceUrl(uri);
    const std::optional<Transport> transport = VideoStreamUri::transportFor(sourceUrl);
    if (!transport) {
        qCWarning(GstSourceFactoryLog) << "Unsupported URI scheme:" << sourceUrl.scheme() << "in"
                                       << QGCNetworkHelper::redactedUrlForLogging(sourceUrl);
        return nullptr;
    }

    const TransportSpec spec = specFor(*transport);
    const bool isRtsp = (spec.element == SourceElement::Rtsp);

    GstElementPtr source;
    switch (spec.element) {
        case SourceElement::Rtsp:
            source = buildRtspSource(uri, sourceUrl, config, latencyMs);
            break;
        case SourceElement::TcpClient:
            source = buildTcpSource(sourceUrl);
            break;
        case SourceElement::Udp:
            source = buildUdpSource(sourceUrl, spec.rtpCodec ? spec.rtpCodec->encodingName : nullptr);
            break;
    }
    if (!source) {
        return nullptr;
    }

    GstElementPtr bin = adoptElement(gst_bin_new("sourcebin"));
    if (!bin) {
        qCWarning(GstSourceFactoryLog) << "gst_bin_new('sourcebin') failed";
        return nullptr;
    }

    ParserStage parserStage = buildParserStage(spec, isRtsp);
    if (!parserStage.parser) {
        return nullptr;
    }

    // From here on the bin owns every element; the borrowed pointers stay valid while bin is alive.
    GstElement* upstream = addToBin(bin.get(), std::move(source), "source");
    if (!upstream) {
        return nullptr;
    }

    GstElement* depay = nullptr;
    if (parserStage.depay) {
        depay = addToBin(bin.get(), std::move(parserStage.depay), spec.rtpCodec->depayloader);
        if (!depay) {
            return nullptr;
        }
    }

    GstElement* const binParser = addToBin(bin.get(), std::move(parserStage.parser), "parser");
    if (!binParser) {
        return nullptr;
    }

    if (spec.demuxMpegTs) {
        upstream = addTsDemux(bin.get(), upstream, latencyMs);
        if (!upstream) {
            return nullptr;
        }
    }

    if (spec.rtpCodec) {
        if (!linkUdpRtpToDepayAndParser(bin.get(), upstream, depay, binParser, *spec.rtpCodec, config, latencyMs)) {
            return nullptr;
        }
        if (!addStaticGhostPad(binParser)) {
            return nullptr;
        }
    } else {
        linkSourceToParserOnPadAdded(upstream, binParser, config, latencyMs, isRtsp);
    }

    return bin.release();
}

std::optional<std::chrono::milliseconds> connectGracePeriod(const QString& uri)
{
    const std::optional<Transport> transport = VideoStreamUri::transportFor(QUrl(uri));
    if (transport && VideoStreamUri::listens(*transport)) {
        return std::nullopt;
    }
    return (transport == Transport::Rtsp) ? kRtspUdpTimeout : std::chrono::milliseconds{0};
}

}  // namespace GStreamer::SourceFactory
