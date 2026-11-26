#include "GstSourceFactory.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <gst/gst.h>
#include <mutex>
#include <optional>
#include <utility>

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>

#include "GStreamerHelpers.h"
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

/// Exposes @p pad of @p element on the bin that holds the element. False when the bin didn't get the pad.
bool addGhostPad(GstElement* element, GstPad* pad)
{
    const GCharPtr name = adoptGChar(gst_pad_get_name(pad));
    GstPadPtr ghostpad = adoptPad(gst_ghost_pad_new(name.get(), pad));
    (void) gst_pad_set_active(ghostpad.get(), TRUE);

    // gst_object_get_parent takes a ref (GST_ELEMENT_PARENT does not) — guards against a
    // concurrent bin teardown finalizing the parent before gst_element_add_pad.
    const GstObjectPtr parent = adoptObject(gst_object_get_parent(GST_OBJECT(element)));
    if (!parent) {
        qCWarning(GstSourceFactoryLog) << "Element has no parent; bin already torn down?";
        return false;
    }

    if (!gst_element_add_pad(GST_ELEMENT(parent.get()), ghostpad.release())) {
        qCWarning(GstSourceFactoryLog) << "gst_element_add_pad() failed";
        return false;
    }
    return true;
}

void wrapWithGhostPad(GstElement* element, GstPad* pad, [[maybe_unused]] gpointer data)
{
    (void) addGhostPad(element, pad);
}

bool addStaticGhostPad(GstElement* element)
{
    const GstPadPtr srcPad = adoptPad(gst_element_get_static_pad(element, "src"));
    return addGhostPad(element, srcPad.get());
}

bool isVideoRtp(const GstStructure* structure)
{
    return gst_structure_has_name(structure, "application/x-rtp") && SourceFactory::isVideoMedia(structure);
}

}  // namespace

namespace GStreamer::SourceFactory {

namespace {

using VideoStreamUri::Transport;

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

// Retransmission stays off: these sources have no RTCP back-channel, so the jitterbuffer's
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

// Heap-owned context for the dynamic (pad-added) link path; freed with the pad-added handler it is given to.
struct DynamicLinkContext
{
    /// What the source's video pad is linked into: parsebin, or the depayloader that took its place (plugRtspCodec()).
    GstElement* binParser;
    bool isRtsp;
    /// The codec whose depayloader and parser took parsebin's place.
    const RtpCodec* rtspCodec;
    /// rtspsrc adds the pads of each of a session's streams on a thread of their own.
    std::mutex linking;
};

void plugRtspCodec(DynamicLinkContext* ctx, const QByteArray& encoding);

// rtspsrc names the pad of each RTP stream it receives "recv_rtp_src_<stream>_<SSRC>_<payload type>", the stream
// being the one of the session, in the SDP's order. nullopt for any other pad.
std::optional<guint> rtspStreamOfPad(GstPad* pad)
{
    const GCharPtr name = adoptGChar(gst_pad_get_name(pad));
    guint stream = 0;
    guint ssrc = 0;
    guint payloadType = 0;
    if (!name || (std::sscanf(name.get(), "recv_rtp_src_%u_%u_%u", &stream, &ssrc, &payloadType) != 3)) {
        return std::nullopt;
    }
    return stream;
}

// The "encoding-name" of the RTP that @p pad carries, or can carry while it has no caps; empty when they don't tell.
QByteArray rtpEncodingOfPad(GstPad* pad)
{
    GstCapsPtr caps = adoptCaps(gst_pad_get_current_caps(pad));
    if (!caps) {
        caps = adoptCaps(gst_pad_query_caps(pad, nullptr));
    }
    if (!caps || gst_caps_is_empty(caps.get()) || gst_caps_is_any(caps.get())) {
        return {};
    }
    return QByteArray(gst_structure_get_string(gst_caps_get_structure(caps.get(), 0), "encoding-name"));
}

// rtspsrc adds a pad for every SSRC and payload type it receives in a stream of the session, and keeps the earlier
// ones. A camera that starts its encoder over, as for another resolution, goes on under a new SSRC: its pad carries
// the stream from then on, and @p linkedPad no more. A pad of another encoding is the stream's retransmissions or
// error correction.
bool continuesRtspStream(GstPad* pad, GstPad* linkedPad)
{
    const std::optional<guint> stream = rtspStreamOfPad(pad);
    const QByteArray encoding = rtpEncodingOfPad(pad);
    return stream && (rtspStreamOfPad(linkedPad) == stream) && !encoding.isEmpty() &&
           (rtpEncodingOfPad(linkedPad) == encoding);
}

// The source's "pad-added", on the streaming thread that brings the pad's data. A state change of the bin waits for
// that thread with the bin's state lock held, so what is taken out of the bin here goes through disposeChildAsync().
void linkPad(GstElement* element, GstPad* pad, gpointer data)
{
    auto* const ctx = static_cast<DynamicLinkContext*>(data);

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

    const std::lock_guard lock(ctx->linking);

    // Link only the first matching video pad. A second pad-added (another video stream of an RTSP session, or a
    // later tsdemux elementary stream) must not re-link an occupied sink. But a pad that the linked RTSP stream goes
    // on on takes its place: left unlinked, its data would be dropped without an error, as rtspsrc reports the flow
    // of the pad that is linked, until the stream watchdog ends the session.
    const GstPadPtr parserSink = adoptPad(gst_element_get_static_pad(ctx->binParser, "sink"));
    if (parserSink && gst_pad_is_linked(parserSink.get())) {
        const GstPadPtr linkedPad = adoptPad(gst_pad_get_peer(parserSink.get()));
        if (!linkedPad || !continuesRtspStream(pad, linkedPad.get()) ||
            !gst_pad_unlink(linkedPad.get(), parserSink.get())) {
            return;
        }
        qCDebug(GstSourceFactoryLog) << "The RTSP stream goes on on a new pad:" << GST_PAD_NAME(pad);
    } else if (isRtp && ctx->isRtsp) {
        plugRtspCodec(ctx, rtpEncodingOfPad(pad));
    }

    const GCharPtr name = adoptGChar(gst_pad_get_name(pad));
    if (!name || !gst_element_link_pads(element, name.get(), ctx->binParser, "sink")) {
        qCWarning(GstSourceFactoryLog) << "gst_element_link_pads() failed for" << (name ? name.get() : "(unnamed pad)");
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
    GstElementPtr depay;  ///< Set only with an RtpCodec
};

// The parser and the depayloader of @p rtpCodec, or parsebin without one, configured but not yet in a bin.
ParserStage buildParserStage(const RtpCodec* rtpCodec, bool isRtsp)
{
    const char* const parserFactory = rtpCodec ? rtpCodec->parser : "parsebin";
    ParserStage stage{makeElement(parserFactory, "parser"), {}};
    if (!stage.parser) {
        qCWarning(GstSourceFactoryLog) << "gst_element_factory_make(" << parserFactory << ") failed";
        return {};
    }

    if (rtpCodec) {
        // RTP over UDP has no SDP/RTSP control plane. Make depayloading and parser config explicit
        // so hardware decoders, especially Android MediaCodec, see repeated parameter sets at IDR
        // boundaries instead of depending on parsebin's defaults after startup or reconnect.
        stage.depay = makeElement(rtpCodec->depayloader);
        if (!stage.depay) {
            qCWarning(GstSourceFactoryLog) << "gst_element_factory_make(" << rtpCodec->depayloader << ") failed";
            return {};
        }
        configureH26xParser(stage.parser.get());
        configureH26xRtpDepayloader(stage.depay.get(), isRtsp);
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
    g_object_set(tsdemux.get(), "latency", static_cast<gint>(std::max(latencyMs, kTsDemuxMinLatencyMs)), nullptr);

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

// GStreamer defect, seen in GStreamer 1.28.8: decodebin3 picks the decoder when the stream starts, from the caps on
// the stream object that came with it, and stores the parsed caps on that object only then, on another thread.
// tsdemux's object has the caps of the unparsed stream, which no decoder takes, and parsebin leaves an upstream
// element's object alone: on a busy machine decodebin3 read those caps, and the stream ended for want of a decoder.
// So they are stored as they leave the parser, which a decodebin3 that waits for the parsed caps would not need.
GstPadProbeReturn storeParsedCapsOnStream(GstPad* pad, GstPadProbeInfo* info, [[maybe_unused]] gpointer data)
{
    GstEvent* const event = GST_PAD_PROBE_INFO_EVENT(info);
    if (GST_EVENT_TYPE(event) != GST_EVENT_CAPS) {
        return GST_PAD_PROBE_OK;
    }

    GstCaps* caps = nullptr;
    gst_event_parse_caps(event, &caps);
    const GstEventPtr streamStart = adoptEvent(gst_pad_get_sticky_event(pad, GST_EVENT_STREAM_START, 0));
    GstStream* rawStream = nullptr;
    if (streamStart) {
        gst_event_parse_stream(streamStart.get(), &rawStream);
    }
    const GstObjectPtr stream = adoptObject(GST_OBJECT_CAST(rawStream));
    if (stream && caps && gst_caps_is_fixed(caps)) {
        gst_stream_set_caps(GST_STREAM_CAST(stream.get()), caps);
    }
    return GST_PAD_PROBE_OK;
}

void storeParsedCapsOnPad([[maybe_unused]] GstElement* parser, GstPad* pad, [[maybe_unused]] gpointer data)
{
    (void) gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, storeParsedCapsOnStream, nullptr, nullptr);
}

// Wires upstream → binParser once upstream's video pad appears: rtspsrc and tsdemux have no source pad before they
// stream. Neither gets a jitter buffer here: rtspsrc runs its own, and tsdemux's pads carry no RTP.
void linkSourceToParserOnPadAdded(GstElement* upstream, GstElement* binParser, bool isRtsp)
{
    // The context lives as long as the handler: binParser may be replaced (plugRtspCodec()).
    auto* ctx = new DynamicLinkContext{
        .binParser = binParser,
        .isRtsp = isRtsp,
        .rtspCodec = nullptr,
        .linking = {},
    };
    (void) g_signal_connect_data(
        upstream, "pad-added", G_CALLBACK(linkPad), ctx,
        [](gpointer context, GClosure*) { delete static_cast<DynamicLinkContext*>(context); }, GConnectFlags(0));

    // pad-added fires on the streaming thread after create() returns the NULL-state bin, so it
    // can't race construction; wrapWithGhostPad ref-checks the parent against teardown.
    (void) g_signal_connect(binParser, "pad-added", G_CALLBACK(wrapWithGhostPad), nullptr);
}

bool linkDepayToParser(GstElement* depay, GstElement* parser, const RtpCodec& codec)
{
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

bool linkUdpRtpToDepayAndParser(GstElement* bin, GstElement* source, GstElement* depay, GstElement* parser,
                                const RtpCodec& codec, const Config& config, guint latencyMs)
{
    // The bin counts the packets itself, where they arrive, with a jitter buffer too: the jitter buffer's own count
    // takes the sequence numbers a sender skips when it starts over for packets lost.
    const GstPadPtr sourcePad = adoptPad(gst_element_get_static_pad(source, "src"));
    if (!sourcePad || !countRtpPackets(bin, sourcePad.get(), kRtpVideoClockRate)) {
        qCWarning(GstSourceFactoryLog) << "Unable to count the RTP packets where they arrive";
    }

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
    }

    if (!gst_element_link(upstream, depay)) {
        qCWarning(GstSourceFactoryLog) << "gst_element_link(upstream, depay) failed";
        return false;
    }

    return linkDepayToParser(depay, parser, codec);
}

// The SDP names the encoding of an RTSP stream, as the scheme does for RTP over UDP: an H.264 or H.265 stream gets the
// same depayloader and parser, linked the same way, in place of @p ctx's parsebin, which stays for any other
// @p encoding. parsebin links a parser only after the depayloader has chosen its output, a buffer for each NAL unit,
// and the parser then can't tell that a frame is complete until the next one starts: a frame interval of latency.
void plugRtspCodec(DynamicLinkContext* ctx, const QByteArray& encoding)
{
    const RtpCodec* const codec =
        (encoding == kRtpH264.encodingName) ? &kRtpH264 : ((encoding == kRtpH265.encodingName) ? &kRtpH265 : nullptr);
    const GstObjectPtr bin = adoptObject(gst_object_get_parent(GST_OBJECT(ctx->binParser)));
    if (!codec || ctx->rtspCodec || !bin) {
        return;
    }

    ParserStage stage = buildParserStage(codec, true);
    if (!stage.parser) {
        return;
    }
    // The bin has an element of the stage's name while parsebin is in it.
    (void) gst_object_set_name(GST_OBJECT(stage.parser.get()), nullptr);
    GstElement* const depay = addToBin(GST_ELEMENT(bin.get()), std::move(stage.depay), codec->depayloader);
    GstElement* const parser = addToBin(GST_ELEMENT(bin.get()), std::move(stage.parser), codec->parser);
    if (!depay || !parser || !linkDepayToParser(depay, parser, *codec) || !gst_element_sync_state_with_parent(parser) ||
        !gst_element_sync_state_with_parent(depay) || !addStaticGhostPad(parser)) {
        qCWarning(GstSourceFactoryLog) << "Unable to plug" << codec->depayloader << "and" << codec->parser
                                       << "for the RTSP stream; leaving it to parsebin";
        if (depay) {
            disposeChildAsync(depay);
        }
        if (parser) {
            disposeChildAsync(parser);
        }
        return;
    }

    disposeChildAsync(ctx->binParser);
    ctx->binParser = depay;
    ctx->rtspCodec = codec;
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
    // A message for every packet, rather than the default of one every 200 ms that tells of those dropped since the
    // last: the rest of a burst would only be told with the next one, which may never come.
    g_object_set(jitterBuffer, "post-drop-messages", TRUE, "drop-messages-interval", 0u, nullptr);
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

    const bool isRtsp = (*transport == Transport::Rtsp);
    // RTP on udpsrc is depayloaded and parsed for the codec its scheme names; the others leave it to parsebin.
    const RtpCodec* const rtpCodec = (*transport == Transport::RtpUdpH264)
                                         ? &kRtpH264
                                         : ((*transport == Transport::RtpUdpH265) ? &kRtpH265 : nullptr);
    // tsdemux ahead of the parser: Android's parsebin can't detect MPEG-TS.
    const bool demuxMpegTs = (*transport == Transport::MpegTsUdp) || (*transport == Transport::MpegTsTcp);

    GstElementPtr source;
    switch (*transport) {
        case Transport::Rtsp:
            source = buildRtspSource(uri, sourceUrl, config, latencyMs);
            break;
        case Transport::MpegTsTcp:
            source = buildTcpSource(sourceUrl);
            break;
        case Transport::RtpUdpH264:
        case Transport::RtpUdpH265:
        case Transport::MpegTsUdp:
            source = buildUdpSource(sourceUrl, rtpCodec ? rtpCodec->encodingName : nullptr);
            break;
    }
    if (!source) {
        return nullptr;
    }

    GstElementPtr bin = adoptElement(gst_bin_new("sourcebin"));

    ParserStage parserStage = buildParserStage(rtpCodec, isRtsp);
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
        depay = addToBin(bin.get(), std::move(parserStage.depay), rtpCodec->depayloader);
        if (!depay) {
            return nullptr;
        }
    }

    GstElement* const binParser = addToBin(bin.get(), std::move(parserStage.parser), "parser");
    if (!binParser) {
        return nullptr;
    }

    if (demuxMpegTs) {
        upstream = addTsDemux(bin.get(), upstream, latencyMs);
        if (!upstream) {
            return nullptr;
        }
        // Ahead of the handler that hands the pad on.
        (void) g_signal_connect(binParser, "pad-added", G_CALLBACK(storeParsedCapsOnPad), nullptr);
    }

    if (rtpCodec) {
        if (!linkUdpRtpToDepayAndParser(bin.get(), upstream, depay, binParser, *rtpCodec, config, latencyMs)) {
            return nullptr;
        }
        if (!addStaticGhostPad(binParser)) {
            return nullptr;
        }
    } else {
        linkSourceToParserOnPadAdded(upstream, binParser, isRtsp);
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
