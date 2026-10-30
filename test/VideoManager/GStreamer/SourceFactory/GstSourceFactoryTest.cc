#include "GstSourceFactoryTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"

namespace {

/// What a bare test sink pad received: each buffer's bytes and PTS.
struct ReceivedBuffers
{
    QList<QByteArray> data;
    QList<GstClockTime> pts;
};

GstFlowReturn recordBuffer(GstPad* pad, [[maybe_unused]] GstObject* parent, GstBuffer* buffer)
{
    auto* const received = static_cast<ReceivedBuffers*>(gst_pad_get_element_private(pad));
    GstMapInfo map;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        received->data.append(QByteArray(reinterpret_cast<const char*>(map.data), static_cast<qsizetype>(map.size)));
        gst_buffer_unmap(buffer, &map);
    }
    received->pts.append(GST_BUFFER_PTS(buffer));
    gst_buffer_unref(buffer);
    return GST_FLOW_OK;
}

/// A src pad standing in for a depayloader's, linked to a sink pad recording into @p received, with @p caps sent.
struct GatedPads
{
    GatedPads(ReceivedBuffers* received, const char* caps)
        : src(GStreamer::adoptPad(GST_PAD(gst_object_ref_sink(gst_pad_new("src", GST_PAD_SRC)))))
        , sink(GStreamer::adoptPad(GST_PAD(gst_object_ref_sink(gst_pad_new("sink", GST_PAD_SINK)))))
    {
        gst_pad_set_element_private(sink.get(), received);
        gst_pad_set_chain_function(sink.get(), recordBuffer);
        gst_pad_set_event_function(sink.get(), [](GstPad*, GstObject*, GstEvent* event) -> gboolean {
            gst_event_unref(event);
            return TRUE;
        });
        (void) gst_pad_set_active(src.get(), TRUE);
        (void) gst_pad_set_active(sink.get(), TRUE);
        linked = gst_pad_link(src.get(), sink.get()) == GST_PAD_LINK_OK;
        (void) gst_pad_push_event(src.get(), gst_event_new_stream_start("parameter-set-gate"));
        const GStreamer::GstCapsPtr streamCaps = GStreamer::adoptCaps(gst_caps_from_string(caps));
        (void) gst_pad_push_event(src.get(), gst_event_new_caps(streamCaps.get()));
        GstSegment segment;
        gst_segment_init(&segment, GST_FORMAT_TIME);
        (void) gst_pad_push_event(src.get(), gst_event_new_segment(&segment));
    }

    ~GatedPads()
    {
        (void) gst_pad_set_active(src.get(), FALSE);
        (void) gst_pad_set_active(sink.get(), FALSE);
    }

    GatedPads(const GatedPads&) = delete;
    GatedPads& operator=(const GatedPads&) = delete;

    GstFlowReturn push(const QByteArray& bytes, GstClockTime pts) const
    {
        GstBuffer* const buffer = gst_buffer_new_memdup(bytes.constData(), static_cast<gsize>(bytes.size()));
        GST_BUFFER_PTS(buffer) = pts;
        return gst_pad_push(src.get(), buffer);
    }

    GStreamer::GstPadPtr src;
    GStreamer::GstPadPtr sink;
    bool linked = false;
};

GstStaticPadTemplate sH264RtpPadTemplate = GST_STATIC_PAD_TEMPLATE(
    "video_rtp", GST_PAD_SRC, GST_PAD_SOMETIMES,
    GST_STATIC_CAPS("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264"));

/// An mpegts:// source bin, whose tsdemux the dynamic-pad tests hand RTP pads to.
struct MpegTsSource
{
    explicit MpegTsSource(const GStreamer::SourceFactory::Config& config)
        : bin(GStreamer::SourceFactory::create(QStringLiteral("mpegts://127.0.0.1:5600"), config))
        , demux(GstTest::findByFactory(bin.get(), "tsdemux"))
        , parserSink(GStreamer::adoptPad(parserSinkPad(bin.get())))
    {}

    /// Built, and no pad reaches the parser yet.
    bool isValid() const { return demux && parserSink && !gst_pad_is_linked(parserSink.get()); }

    GstTest::TestPipeline bin;
    GStreamer::GstElementPtr demux;
    GStreamer::GstPadPtr parserSink;

private:
    static GstPad* parserSinkPad(GstElement* bin)
    {
        const GStreamer::GstElementPtr parser = GstTest::findByFactory(bin, "parsebin");
        return parser ? gst_element_get_static_pad(parser.get(), "sink") : nullptr;
    }
};

}  // namespace

void GstSourceFactoryTest::_testSourceFactoryUdpRtpJitterBuffer()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "rtpjitterbuffer"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    config.latencyMs = 80;
    config.doRetransmission = true;

    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr jb = GstTest::findByFactory(bin.get(), "rtpjitterbuffer");
    QVERIFY2(jb, "static UDP-RTP path must insert an rtpjitterbuffer at NULL state");

    guint latency = 0;
    gboolean doLost = FALSE, doRtx = TRUE, dropOnLatency = FALSE;
    g_object_get(jb.get(), "latency", &latency, "do-lost", &doLost, "do-retransmission", &doRtx, "drop-on-latency",
                 &dropOnLatency, nullptr);
    QCOMPARE(latency, 80u);
    QCOMPARE(doLost, TRUE);
    QVERIFY2(!doRtx, "plain UDP has no RTCP back-channel, so retransmission requests could never reach the sender");
    QCOMPARE(dropOnLatency, TRUE);
}

void GstSourceFactoryTest::_testSourceFactoryJitterBufferNone()
{
    if (!GstTest::firstMissingFactory({"udpsrc"}).isEmpty()) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::None;

    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);

    QVERIFY2(!GstTest::findByFactory(bin.get(), "rtpjitterbuffer"),
             "JitterBuffer::None must link the source straight to the parser");
}

void GstSourceFactoryTest::_testSourceFactoryNoRetransmission()
{
    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }

    // RTSP is the only source with a back-channel for retransmission requests, so it's the one the option reaches.
    GStreamer::SourceFactory::Config config;
    config.doRetransmission = false;

    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr source = GstTest::findByFactory(bin.get(), "rtspsrc");
    QVERIFY(source);

    gboolean doRtx = TRUE;
    g_object_get(source.get(), "do-retransmission", &doRtx, nullptr);
    QCOMPARE(doRtx, FALSE);
}

void GstSourceFactoryTest::_testSourceFactoryRtspJitterBufferPolicy_data()
{
    QTest::addColumn<int>("jitterBufferPolicy");
    QTest::addColumn<guint>("expectedLatencyMs");
    QTest::addColumn<bool>("expectedRetransmission");
    QTest::addColumn<bool>("expectedDropOnLatency");
    QTest::addColumn<int>("expectedRtxDelay");
    QTest::addColumn<int>("expectedRtxMaxRetries");

    using JitterBuffer = GStreamer::SourceFactory::JitterBuffer;
    // -1 is rtpjitterbuffer's "auto" default, i.e. the factory left the internal jitterbuffer untouched.
    QTest::newRow("low-latency") << static_cast<int>(JitterBuffer::None) << 0u << false << true << -1 << -1;
    QTest::newRow("drop-on-latency") << static_cast<int>(JitterBuffer::DropOnLatency) << 80u << true << true << 25 << 1;
}

void GstSourceFactoryTest::_testSourceFactoryRtspJitterBufferPolicy()
{
    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }

    QFETCH(int, jitterBufferPolicy);
    QFETCH(guint, expectedLatencyMs);
    QFETCH(bool, expectedRetransmission);
    QFETCH(bool, expectedDropOnLatency);
    QFETCH(int, expectedRtxDelay);
    QFETCH(int, expectedRtxMaxRetries);

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = static_cast<GStreamer::SourceFactory::JitterBuffer>(jitterBufferPolicy);
    config.latencyMs = 80;
    config.doRetransmission = true;

    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr source = GstTest::findByFactory(bin.get(), "rtspsrc");
    QVERIFY(source);

    guint latencyMs = 0;
    gboolean doRetransmission = FALSE;
    gboolean dropOnLatency = FALSE;
    g_object_get(source.get(), "latency", &latencyMs, "do-retransmission", &doRetransmission, "drop-on-latency",
                 &dropOnLatency, nullptr);
    QCOMPARE(latencyMs, expectedLatencyMs);
    QCOMPARE(doRetransmission, expectedRetransmission ? TRUE : FALSE);
    QCOMPARE(dropOnLatency, expectedDropOnLatency ? TRUE : FALSE);

    QVERIFY2(!GstTest::findByFactory(bin.get(), "rtpjitterbuffer"),
             "rtspsrc owns its internal jitterbuffer; the factory must not add a second one");

    // rtspsrc only creates its rtpbin during SETUP; drive the same signals it emits so the
    // per-stream internal jitterbuffer policy can be checked without a server.
    GStreamer::GstElementPtr manager = GStreamer::adoptElement(gst_element_factory_make("rtpbin", nullptr));
    GStreamer::GstElementPtr internalBuffer =
        GStreamer::adoptElement(gst_element_factory_make("rtpjitterbuffer", nullptr));
    QVERIFY(manager);
    QVERIFY(internalBuffer);
    g_signal_emit_by_name(source.get(), "new-manager", manager.get());
    g_signal_emit_by_name(manager.get(), "new-jitterbuffer", internalBuffer.get(), 0u, 0u);

    gint rtxDelay = 0;
    gint rtxMaxRetries = 0;
    g_object_get(internalBuffer.get(), "rtx-delay", &rtxDelay, "rtx-max-retries", &rtxMaxRetries, nullptr);
    QCOMPARE(rtxDelay, expectedRtxDelay);
    QCOMPARE(rtxMaxRetries, expectedRtxMaxRetries);
}

void GstSourceFactoryTest::_testSourceFactoryRtspSelectsOnlyVideoStreams_data()
{
    QTest::addColumn<QString>("capsName");
    QTest::addColumn<QString>("media");
    QTest::addColumn<bool>("expectedSelected");

    // Before the first SETUP rtspsrc passes SDP-derived caps that are still named application/x-unknown.
    const QString sdpCaps = QStringLiteral("application/x-unknown");
    QTest::newRow("video") << sdpCaps << QStringLiteral("video") << true;
    QTest::newRow("audio") << sdpCaps << QStringLiteral("audio") << false;
    QTest::newRow("metadata") << sdpCaps << QStringLiteral("application") << false;
    // On reconnect the stream caps already carry the transport media type.
    QTest::newRow("reconnect-video") << QStringLiteral("application/x-rtp") << QStringLiteral("video") << true;
}

void GstSourceFactoryTest::_testSourceFactoryRtspSelectsOnlyVideoStreams()
{
    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }

    QFETCH(QString, capsName);
    QFETCH(QString, media);
    QFETCH(bool, expectedSelected);

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr source = GstTest::findByFactory(bin.get(), "rtspsrc");
    QVERIFY(source);

    const QByteArray capsNameUtf8 = capsName.toUtf8();
    const QByteArray mediaUtf8 = media.toUtf8();
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_new_simple(
        capsNameUtf8.constData(), "media", G_TYPE_STRING, mediaUtf8.constData(), "payload", G_TYPE_INT, 96,
        "clock-rate", G_TYPE_INT, 90000, "encoding-name", G_TYPE_STRING, "H264", nullptr));
    QVERIFY(caps);

    gboolean selected = expectedSelected ? FALSE : TRUE;
    g_signal_emit_by_name(source.get(), "select-stream", 0u, caps.get(), &selected);
    QCOMPARE(selected, expectedSelected ? TRUE : FALSE);
}

void GstSourceFactoryTest::_testSourceFactoryRejectsBadUri()
{
    ignoreLogMessage(
        "Video.GStreamer.GstSourceFactory", QtWarningMsg,
        QRegularExpression(QStringLiteral("URI is not specified|Unsupported URI scheme|Invalid UDP port")));

    GStreamer::SourceFactory::Config config;
    QVERIFY(!GstTest::createSource(QString(), config));
    QVERIFY(!GstTest::createSource(QStringLiteral("ftp://127.0.0.1/x"), config));
    QVERIFY(!GstTest::createSource(QStringLiteral("udp://127.0.0.1"), config));
    QVERIFY(!GstTest::createSource(QStringLiteral("udp://127.0.0.1:99999"), config));
}

void GstSourceFactoryTest::_testSourceFactoryTcpMpegTs()
{
    const QString missing = GstTest::firstMissingFactory({"tcpclientsrc", "tsdemux"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("tcp://192.168.1.50:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr src = GstTest::findByFactory(bin.get(), "tcpclientsrc");
    QVERIFY2(src, "tcp:// must build a tcpclientsrc");
    gchar* rawHost = nullptr;
    gint port = 0;
    g_object_get(src.get(), "host", &rawHost, "port", &port, nullptr);
    const GStreamer::GCharPtr host = GStreamer::adoptGChar(rawHost);
    QCOMPARE(QString::fromUtf8(host.get()), QStringLiteral("192.168.1.50"));
    QCOMPARE(port, 5600);

    QVERIFY2(GstTest::findByFactory(bin.get(), "tsdemux"), "MPEG-TS over TCP must insert tsdemux explicitly");
    QVERIFY2(!GstTest::findByFactory(bin.get(), "rtpjitterbuffer"), "raw MPEG-TS is not RTP; no jitterbuffer");
}

void GstSourceFactoryTest::_testSourceFactoryRejectsBadTcpUri()
{
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid TCP port|Missing host in TCP URI")));

    GStreamer::SourceFactory::Config config;
    QVERIFY2(!GstTest::createSource(QStringLiteral("tcp://192.168.1.50"), config),
             "tcp:// without a port must be rejected");
    QVERIFY2(!GstTest::createSource(QStringLiteral("tcp://:5600"), config), "tcp:// without a host must be rejected");
    QVERIFY2(!GstTest::createSource(QStringLiteral("tcp://192.168.1.50:0"), config),
             "tcp:// with port 0 must be rejected");
}

void GstSourceFactoryTest::_testSourceFactoryUdp265()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "rtpjitterbuffer", "rtph265depay", "h265parse"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("udp265://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr src = GstTest::findByFactory(bin.get(), "udpsrc");
    QVERIFY2(src, "udp265:// must build a udpsrc");
    GstCaps* rawCaps = nullptr;
    g_object_get(src.get(), "caps", &rawCaps, nullptr);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(rawCaps);
    QVERIFY2(caps, "udp265:// must set H265 RTP caps on udpsrc");
    QCOMPARE(QString::fromUtf8(gst_structure_get_string(gst_caps_get_structure(caps.get(), 0), "encoding-name")),
             QStringLiteral("H265"));

    QVERIFY2(GstTest::findByFactory(bin.get(), "rtpjitterbuffer"), "udp265 RTP path must insert a jitterbuffer");
    QVERIFY2(!GstTest::findByFactory(bin.get(), "tsdemux"), "udp265 is RTP, not MPEG-TS; no tsdemux");
    const GStreamer::GstElementPtr depay = GstTest::findByFactory(bin.get(), "rtph265depay");
    QVERIFY2(depay, "udp265:// must depayload H265 RTP explicitly");

    // hvc1 would leave the depayloader's caps unset until in-band parameter sets arrive
    const GStreamer::GstPadPtr depaySrc = GStreamer::adoptPad(gst_element_get_static_pad(depay.get(), "src"));
    QVERIFY(depaySrc);
    const GStreamer::GstCapsPtr allowed = GStreamer::adoptCaps(gst_pad_get_allowed_caps(depaySrc.get()));
    QVERIFY2(allowed && !gst_caps_is_empty(allowed.get()), "rtph265depay must link to the parser");
    for (guint i = 0; i < gst_caps_get_size(allowed.get()); ++i) {
        QCOMPARE(QString::fromUtf8(gst_structure_get_string(gst_caps_get_structure(allowed.get(), i), "stream-format")),
                 QStringLiteral("byte-stream"));
    }

    const GStreamer::GstElementPtr parser = GstTest::findByFactory(bin.get(), "h265parse");
    QVERIFY2(parser, "udp265:// must parse H265 explicitly");
    gint configInterval = 0;
    g_object_get(parser.get(), "config-interval", &configInterval, nullptr);
    QCOMPARE(configInterval, -1);
}

void GstSourceFactoryTest::_testSourceFactoryUdpH264Caps()
{
    if (!GstTest::firstMissingFactory({"udpsrc"}).isEmpty()) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr src = GstTest::findByFactory(bin.get(), "udpsrc");
    QVERIFY(src);
    GstCaps* rawCaps = nullptr;
    g_object_get(src.get(), "caps", &rawCaps, nullptr);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(rawCaps);
    QVERIFY2(caps, "udp:// must set H264 RTP caps");
    const GstStructure* st = gst_caps_get_structure(caps.get(), 0);
    QCOMPARE(QString::fromUtf8(gst_structure_get_string(st, "media")), QStringLiteral("video"));
    QCOMPARE(QString::fromUtf8(gst_structure_get_string(st, "encoding-name")), QStringLiteral("H264"));
}

void GstSourceFactoryTest::_testParameterSetGateHoldsUntilCompleteSet()
{
    const QByteArray aud = QByteArray::fromHex("0000000109f0");
    const QByteArray sps = QByteArray::fromHex("0000000167640014acd9");
    const QByteArray pps = QByteArray::fromHex("0000000168ebe3cb");
    const QByteArray idr = QByteArray::fromHex("0000000165888400");
    const QByteArray delta = QByteArray::fromHex("00000001419a2400");

    ReceivedBuffers received;
    const GatedPads pads(&received, "video/x-h264, stream-format=(string)byte-stream, alignment=(string)nal");
    QVERIFY(pads.linked);
    GStreamer::SourceFactory::gateOnParameterSets(pads.src.get(), false);

    // A late join whose first keyframe lost its PPS (and IDR): nothing may pass on an SPS alone.
    for (const QByteArray& nal : {aud, sps, delta, delta, aud, sps}) {
        QCOMPARE(pads.push(nal, GST_SECOND), GST_FLOW_OK);
    }
    QVERIFY(received.data.isEmpty());

    QCOMPARE(pads.push(pps, 2 * GST_SECOND), GST_FLOW_OK);
    QVERIFY(received.data.isEmpty());
    QCOMPARE(pads.push(idr, 3 * GST_SECOND), GST_FLOW_OK);
    QCOMPARE(pads.push(delta, 4 * GST_SECOND), GST_FLOW_OK);

    QCOMPARE(received.data.size(), 2);
    QCOMPARE(received.data.at(0), sps + pps + idr);
    QCOMPARE(received.pts.at(0), 3 * GST_SECOND);
    QCOMPARE(received.data.at(1), delta);
}

void GstSourceFactoryTest::_testParameterSetGatePassesLengthPrefixedOutput()
{
    ReceivedBuffers received;
    const GatedPads pads(&received, "video/x-h264, stream-format=(string)avc, alignment=(string)au");
    QVERIFY(pads.linked);
    GStreamer::SourceFactory::gateOnParameterSets(pads.src.get(), false);

    // avc carries its parameter sets in codec_data, so its first slice passes as is.
    const QByteArray lengthPrefixedSlice = QByteArray::fromHex("000000044188a5f0");
    QCOMPARE(pads.push(lengthPrefixedSlice, 0), GST_FLOW_OK);
    QCOMPARE(received.data, QList<QByteArray>{lengthPrefixedSlice});
}

void GstSourceFactoryTest::_testSourceFactoryUdpMpegTs()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "tsdemux"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("mpegts://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr src = GstTest::findByFactory(bin.get(), "udpsrc");
    QVERIFY2(src, "mpegts:// must build a udpsrc");
    GstCaps* rawCaps = nullptr;
    g_object_get(src.get(), "caps", &rawCaps, nullptr);
    QVERIFY2(!GStreamer::adoptCaps(rawCaps), "raw MPEG-TS over UDP must not force RTP caps on udpsrc");

    QVERIFY2(GstTest::findByFactory(bin.get(), "tsdemux"), "mpegts:// must insert tsdemux");
    QVERIFY2(!GstTest::findByFactory(bin.get(), "rtpjitterbuffer"), "raw MPEG-TS is not RTP; no jitterbuffer");
}

void GstSourceFactoryTest::_testSourceFactorySchemeCaseInsensitive()
{
    if (!GstTest::firstMissingFactory({"udpsrc"}).isEmpty()) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    QVERIFY2(GstTest::createSource(QStringLiteral("UDP://127.0.0.1:5600"), config),
             "uppercase scheme must be accepted (scheme is lower-cased before matching)");
}

void GstSourceFactoryTest::_testSourceFactoryNegativeLatencyClamped()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "rtpjitterbuffer"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    config.latencyMs = -100;
    const GStreamer::GstElementPtr bin = GstTest::createSource(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr jb = GstTest::findByFactory(bin.get(), "rtpjitterbuffer");
    QVERIFY(jb);
    guint latency = 99999;
    g_object_get(jb.get(), "latency", &latency, nullptr);
    QCOMPARE(latency, 0u);
}

void GstSourceFactoryTest::_testSourceFactoryDynamicRtpLinkFailureCleansJitterBuffer()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "tsdemux", "rtpjitterbuffer"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    const MpegTsSource source(config);
    QVERIFY(source.isValid());

    // A running bin syncs the temporary jitterbuffer out of NULL, so removing it without a NULL
    // transition raises GStreamer's "disposing element in READY/PLAYING" critical. The source stays
    // locked in NULL so the test doesn't bind the UDP port.
    const GStreamer::GstElementPtr udpsrc = GstTest::findByFactory(source.bin.get(), "udpsrc");
    QVERIFY(udpsrc);
    (void) gst_element_set_locked_state(udpsrc.get(), TRUE);
    QCOMPARE(gst_element_set_state(source.bin.get(), GST_STATE_READY), GST_STATE_CHANGE_SUCCESS);

    // Not added to tsdemux, so linking it to the jitterbuffer fails.
    const GStreamer::GstPadPtr strayPad = GStreamer::adoptPad(
        GST_PAD(gst_object_ref_sink(gst_pad_new_from_static_template(&sH264RtpPadTemplate, "stray_rtp"))));
    QVERIFY(strayPad);

    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("gst_element_link_pads\\(\\) failed")));
    g_signal_emit_by_name(source.demux.get(), "pad-added", strayPad.get());

    QVERIFY2(!gst_pad_is_linked(source.parserSink.get()),
             "a failed dynamic RTP pad link must leave parsebin available for the next valid pad");
    QVERIFY2(!GstTest::findByFactory(source.bin.get(), "rtpjitterbuffer"),
             "a failed dynamic RTP pad link must remove its temporary jitterbuffer");
}

void GstSourceFactoryTest::_testSourceFactoryDynamicLinkSkipsNonVideoRtp()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "tsdemux", "rtpjitterbuffer"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    const MpegTsSource source(config);
    QVERIFY(source.isValid());

    // Mirrors an RTSP SDP listing audio before video: the first RTP pad must not claim the parser.
    static GstStaticPadTemplate sAudioTemplate = GST_STATIC_PAD_TEMPLATE(
        "audio_rtp", GST_PAD_SRC, GST_PAD_SOMETIMES,
        GST_STATIC_CAPS("application/x-rtp, media=(string)audio, clock-rate=(int)48000, encoding-name=(string)OPUS"));

    // gst_element_add_pad emits pad-added and consumes the floating pad.
    QVERIFY(gst_element_add_pad(source.demux.get(), gst_pad_new_from_static_template(&sAudioTemplate, "audio_rtp")));
    QVERIFY2(!gst_pad_is_linked(source.parserSink.get()), "an audio RTP pad must not be linked to the video parser");
    QVERIFY2(!GstTest::findByFactory(source.bin.get(), "rtpjitterbuffer"),
             "an audio RTP pad must not get a jitterbuffer");

    QVERIFY(
        gst_element_add_pad(source.demux.get(), gst_pad_new_from_static_template(&sH264RtpPadTemplate, "video_rtp")));
    QVERIFY2(gst_pad_is_linked(source.parserSink.get()), "the video RTP pad that follows must still reach the parser");
}

void GstSourceFactoryTest::_testSourceFactoryRtspDepayloaderRequestsKeyframes_data()
{
    QTest::addColumn<QString>("uri");
    QTest::addColumn<QString>("sourceFactory");
    QTest::addColumn<bool>("expectedRequestKeyframe");

    // RTSP has RTCP to carry the request back to the camera; a plain UDP source has nothing to send it over.
    QTest::newRow("rtsp") << QStringLiteral("rtsp://127.0.0.1:8554/test") << QStringLiteral("rtspsrc") << true;
    QTest::newRow("mpegts-rtp") << QStringLiteral("mpegts://127.0.0.1:5600") << QStringLiteral("tsdemux") << false;
}

void GstSourceFactoryTest::_testSourceFactoryRtspDepayloaderRequestsKeyframes()
{
    QFETCH(QString, uri);
    QFETCH(QString, sourceFactory);
    QFETCH(bool, expectedRequestKeyframe);

    const QString missing = GstTest::firstMissingFactory({"rtspsrc", "udpsrc", "tsdemux", "parsebin", "rtph264depay"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::None;
    const GStreamer::GstElementPtr bin = GstTest::createSource(uri, config);
    QVERIFY(bin);
    const GStreamer::GstElementPtr source = GstTest::findByFactory(bin.get(), qPrintable(sourceFactory));
    const GStreamer::GstElementPtr parser = GstTest::findByFactory(bin.get(), "parsebin");
    QVERIFY(source && parser);

    // The source stays in NULL, so nothing connects: its RTP pad is added by hand, as it adds one after SETUP, and
    // the stream caps make parsebin plug the depayloader.
    GstPad* const pad = gst_pad_new_from_static_template(&sH264RtpPadTemplate, "recv_rtp_src_0");
    QVERIFY(gst_element_add_pad(source.get(), pad));
    QVERIFY(gst_pad_is_linked(pad));
    const auto stop = qScopeGuard([&parser, pad] {
        (void) gst_element_set_state(parser.get(), GST_STATE_NULL);
        (void) gst_pad_set_active(pad, FALSE);
    });
    QVERIFY(gst_element_set_state(parser.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);
    QVERIFY(gst_pad_set_active(pad, TRUE));
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_from_string(
        "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96"));
    GstSegment segment;
    gst_segment_init(&segment, GST_FORMAT_TIME);
    QVERIFY(gst_pad_push_event(pad, gst_event_new_stream_start("qgc-rtp")));
    QVERIFY(gst_pad_push_event(pad, gst_event_new_caps(caps.get())));
    QVERIFY(gst_pad_push_event(pad, gst_event_new_segment(&segment)));

    const GStreamer::GstElementPtr depayloader = GstTest::findByFactory(bin.get(), "rtph264depay");
    QVERIFY2(depayloader, "parsebin did not plug rtph264depay for H.264 RTP caps");
    gboolean requestKeyframe = !expectedRequestKeyframe;
    g_object_get(depayloader.get(), "request-keyframe", &requestKeyframe, nullptr);
    QCOMPARE(bool(requestKeyframe), expectedRequestKeyframe);
}

#endif

UT_REGISTER_TEST(GstSourceFactoryTest, TestLabel::Unit)
