#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <atomic>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

#include "GStreamerTestHelpers.h"
#include "GstScoped.h"
#include "GstSourceFactory.h"

namespace {

/// SourceFactory::create() hands out a floating bin; this sinks it.
GStreamer::GstElementPtr createSource(const QString& uri, const GStreamer::SourceFactory::Config& config)
{
    GstElement* const bin = GStreamer::SourceFactory::create(uri, config);
    return GStreamer::adoptElement(bin ? GST_ELEMENT(gst_object_ref_sink(bin)) : nullptr);
}

/// The first element inside @p bin made by @p factoryName, or null.
GStreamer::GstElementPtr child(GstElement* bin, const char* factoryName)
{
    return GstTest::findByFactory(GST_BIN(bin), factoryName);
}

/// RTP packets into rtph264depay and the units that reach h264parse after it.
struct DepayloaderOutput
{
    std::atomic<int> packetsIn{0};
    std::atomic<int> unitsOut{0};
    std::atomic<bool> firstUnitHasSps{false};
};

bool containsH264Sps(const guint8* data, gsize size)
{
    for (gsize i = 0; (i + 3) < size; ++i) {
        if ((data[i] == 0) && (data[i + 1] == 0) && (data[i + 2] == 1) && ((data[i + 3] & 0x1FU) == 7U)) {
            return true;
        }
    }
    return false;
}

GstPadProbeReturn recordDepayloaderUnit([[maybe_unused]] GstPad* pad, GstPadProbeInfo* info, gpointer data)
{
    auto* output = static_cast<DepayloaderOutput*>(data);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    GstMapInfo map;
    if ((output->unitsOut.fetch_add(1) == 0) && buffer && gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        output->firstUnitHasSps = containsH264Sps(map.data, map.size);
        gst_buffer_unmap(buffer, &map);
    }
    return GST_PAD_PROBE_OK;
}

// Units are recorded at h264parse's sink rather than the depayloader's src: the source bin sees
// deep-element-added before parsebin's own handlers, so a probe added here would run before the factory's.
void watchH264Depayloading([[maybe_unused]] GstBin* bin, [[maybe_unused]] GstBin* subBin, GstElement* element,
                           gpointer data)
{
    GstElementFactory* factory = gst_element_get_factory(element);
    const char* factoryName = factory ? gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)) : nullptr;
    auto* output = static_cast<DepayloaderOutput*>(data);
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(element, "sink"));
    if (g_strcmp0(factoryName, "rtph264depay") == 0) {
        (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &output->packetsIn,
                                 nullptr);
    } else if (g_strcmp0(factoryName, "h264parse") == 0) {
        (void) gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BUFFER, recordDepayloaderUnit, output, nullptr);
    }
}

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

void linkToSink([[maybe_unused]] GstElement* element, GstPad* pad, gpointer data)
{
    const GStreamer::GstPadPtr sinkPad = GStreamer::adoptPad(gst_element_get_static_pad(GST_ELEMENT(data), "sink"));
    if (sinkPad && !gst_pad_is_linked(sinkPad.get())) {
        (void) gst_pad_link(pad, sinkPad.get());
    }
}

GstStaticPadTemplate sH264RtpPadTemplate = GST_STATIC_PAD_TEMPLATE(
    "video_rtp", GST_PAD_SRC, GST_PAD_SOMETIMES,
    GST_STATIC_CAPS("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264"));

/// An mpegts:// source bin, whose tsdemux the dynamic-pad tests hand RTP pads to.
struct MpegTsSource
{
    explicit MpegTsSource(const GStreamer::SourceFactory::Config& config)
        : bin(GStreamer::SourceFactory::create(QStringLiteral("mpegts://127.0.0.1:5600"), config))
        , demux(bin.get() ? child(bin.get(), "tsdemux") : nullptr)
        , parserSink(GStreamer::adoptPad(bin.get() ? parserSinkPad(bin.get()) : nullptr))
    {}

    /// Built, and no pad reaches the parser yet.
    bool isValid() const { return demux && parserSink && !gst_pad_is_linked(parserSink.get()); }

    GstTest::TestPipeline bin;
    GStreamer::GstElementPtr demux;
    GStreamer::GstPadPtr parserSink;

private:
    static GstPad* parserSinkPad(GstElement* bin)
    {
        const GStreamer::GstElementPtr parser = child(bin, "parsebin");
        return parser ? gst_element_get_static_pad(parser.get(), "sink") : nullptr;
    }
};

// Minimal RTSP responder for driving a real rtspsrc without gst-rtsp-server: answers OPTIONS/DESCRIBE with a
// fixed SDP and records each SETUP URL without answering it, so the client stops right after picking streams.
class FakeRtspServer
{
public:
    explicit FakeRtspServer(const QByteArray& sdp)
        : _sdp(sdp)
    {
        QObject::connect(&_server, &QTcpServer::newConnection, &_server, [this] {
            while (QTcpSocket* socket = _server.nextPendingConnection()) {
                QObject::connect(socket, &QTcpSocket::readyRead, socket, [this, socket] { _serve(socket); });
            }
        });
    }

    bool listen() { return _server.listen(QHostAddress::LocalHost, 0); }

    QString url() const { return QStringLiteral("rtsp://127.0.0.1:%1/test").arg(_server.serverPort()); }

    const QList<QByteArray>& setupUrls() const { return _setupUrls; }

private:
    void _serve(QTcpSocket* socket)
    {
        QByteArray& pending = _pending[socket];
        pending += socket->readAll();
        for (qsizetype end = pending.indexOf("\r\n\r\n"); end >= 0; end = pending.indexOf("\r\n\r\n")) {
            const QList<QByteArray> lines = pending.left(end).split('\n');
            pending.remove(0, end + 4);

            const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
            const QByteArray method = requestLine.value(0);
            QByteArray cseq;
            for (const QByteArray& line : lines) {
                if (line.toLower().startsWith("cseq:")) {
                    cseq = line.mid(5).trimmed();
                }
            }

            if (method == "SETUP") {
                _setupUrls.append(requestLine.value(1));
                continue;
            }

            QByteArray reply = "RTSP/1.0 200 OK\r\nCSeq: " + cseq + "\r\n";
            if (method == "OPTIONS") {
                reply += "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n\r\n";
            } else if (method == "DESCRIBE") {
                reply += "Content-Base: " + url().toUtf8() +
                         "/\r\nContent-Type: application/sdp\r\nContent-Length: " + QByteArray::number(_sdp.size()) +
                         "\r\n\r\n" + _sdp;
            } else {
                reply += "\r\n";
            }
            (void) socket->write(reply);
        }
    }

    QTcpServer _server;
    QByteArray _sdp;
    QHash<QTcpSocket*, QByteArray> _pending;
    QList<QByteArray> _setupUrls;
};

}  // namespace

void GStreamerTest::_testSourceFactoryUdpRtpJitterBuffer()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "rtpjitterbuffer"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    config.latencyMs = 80;
    config.doRetransmission = true;

    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr jb = child(bin.get(), "rtpjitterbuffer");
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

void GStreamerTest::_testSourceFactoryJitterBufferNone()
{
    if (!GstTest::firstMissingFactory({"udpsrc"}).isEmpty()) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::None;

    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);

    QVERIFY2(!child(bin.get(), "rtpjitterbuffer"), "JitterBuffer::None must link the source straight to the parser");
}

void GStreamerTest::_testSourceFactoryNoRetransmission()
{
    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }

    // RTSP is the only source with a back-channel for retransmission requests, so it's the one the option reaches.
    GStreamer::SourceFactory::Config config;
    config.doRetransmission = false;

    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr source = child(bin.get(), "rtspsrc");
    QVERIFY(source);

    gboolean doRtx = TRUE;
    g_object_get(source.get(), "do-retransmission", &doRtx, nullptr);
    QCOMPARE(doRtx, FALSE);
}

void GStreamerTest::_testSourceFactoryRtspJitterBufferPolicy_data()
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

void GStreamerTest::_testSourceFactoryRtspJitterBufferPolicy()
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

    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr source = child(bin.get(), "rtspsrc");
    QVERIFY(source);

    guint latencyMs = 0;
    gboolean doRetransmission = FALSE;
    gboolean dropOnLatency = FALSE;
    g_object_get(source.get(), "latency", &latencyMs, "do-retransmission", &doRetransmission, "drop-on-latency",
                 &dropOnLatency, nullptr);
    QCOMPARE(latencyMs, expectedLatencyMs);
    QCOMPARE(doRetransmission, expectedRetransmission ? TRUE : FALSE);
    QCOMPARE(dropOnLatency, expectedDropOnLatency ? TRUE : FALSE);

    QVERIFY2(!child(bin.get(), "rtpjitterbuffer"),
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

void GStreamerTest::_testSourceFactoryRtspSelectsOnlyVideoStreams_data()
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

void GStreamerTest::_testSourceFactoryRtspSelectsOnlyVideoStreams()
{
    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }

    QFETCH(QString, capsName);
    QFETCH(QString, media);
    QFETCH(bool, expectedSelected);

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr source = child(bin.get(), "rtspsrc");
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

void GStreamerTest::_testSourceFactoryRtspSetsUpOnlyVideoStream()
{
    if (!GstTest::firstMissingFactory({"rtspsrc"}).isEmpty()) {
        QSKIP("rtspsrc plugin unavailable");
    }

    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg, GstTest::libproxyIgnoreMaskWarning());

    // Audio listed first, as on many IP cameras; rtspsrc sets streams up in SDP order.
    FakeRtspServer server(
        QByteArrayLiteral("v=0\r\n"
                          "o=- 0 0 IN IP4 127.0.0.1\r\n"
                          "s=QGC select-stream test\r\n"
                          "c=IN IP4 127.0.0.1\r\n"
                          "t=0 0\r\n"
                          "m=audio 0 RTP/AVP 97\r\n"
                          "a=rtpmap:97 OPUS/48000/2\r\n"
                          "a=control:trackID=0\r\n"
                          "m=video 0 RTP/AVP 96\r\n"
                          "a=rtpmap:96 H264/90000\r\n"
                          "a=control:trackID=1\r\n"));
    QVERIFY(server.listen());

    GstTest::TestPipeline pipeline(gst_pipeline_new("rtsp-select-stream-test"));
    QVERIFY(pipeline.get());

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = createSource(server.url(), config);
    QVERIFY(bin);
    QVERIFY(gst_bin_add(pipeline.bin(), bin.get()));
    QVERIFY(gst_element_set_state(pipeline.get(), GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);

    QVERIFY_TRUE_WAIT(!server.setupUrls().isEmpty(), TestTimeout::mediumMs());
    QVERIFY2(server.setupUrls().first().endsWith("/trackID=1"),
             qPrintable(QStringLiteral("first SETUP must target the video track, got %1")
                            .arg(QString::fromUtf8(server.setupUrls().first()))));
}

void GStreamerTest::_testSourceFactoryRejectsBadUri()
{
    ignoreLogMessage(
        "Video.GStreamer.GstSourceFactory", QtWarningMsg,
        QRegularExpression(QStringLiteral("URI is not specified|Unsupported URI scheme|Invalid UDP port")));

    GStreamer::SourceFactory::Config config;
    QVERIFY(!createSource(QString(), config));
    QVERIFY(!createSource(QStringLiteral("ftp://127.0.0.1/x"), config));
    QVERIFY(!createSource(QStringLiteral("udp://127.0.0.1:0"), config));
    QVERIFY(!createSource(QStringLiteral("udp://127.0.0.1:99999"), config));
}

void GStreamerTest::_testSourceFactoryTcpMpegTs()
{
    const QString missing = GstTest::firstMissingFactory({"tcpclientsrc", "tsdemux"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("tcp://192.168.1.50:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr src = child(bin.get(), "tcpclientsrc");
    QVERIFY2(src, "tcp:// must build a tcpclientsrc");
    gchar* rawHost = nullptr;
    gint port = 0;
    g_object_get(src.get(), "host", &rawHost, "port", &port, nullptr);
    const GStreamer::GCharPtr host = GStreamer::adoptGChar(rawHost);
    QCOMPARE(QString::fromUtf8(host.get()), QStringLiteral("192.168.1.50"));
    QCOMPARE(port, 5600);

    QVERIFY2(child(bin.get(), "tsdemux"), "MPEG-TS over TCP must insert tsdemux explicitly");
    QVERIFY2(!child(bin.get(), "rtpjitterbuffer"), "raw MPEG-TS is not RTP; no jitterbuffer");
}

void GStreamerTest::_testSourceFactoryRejectsBadTcpUri()
{
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid TCP port|Missing host in TCP URI")));

    GStreamer::SourceFactory::Config config;
    QVERIFY2(!createSource(QStringLiteral("tcp://192.168.1.50"), config), "tcp:// without a port must be rejected");
    QVERIFY2(!createSource(QStringLiteral("tcp://:5600"), config), "tcp:// without a host must be rejected");
    QVERIFY2(!createSource(QStringLiteral("tcp://192.168.1.50:0"), config), "tcp:// with port 0 must be rejected");
}

void GStreamerTest::_testSourceFactoryUdp265()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "rtpjitterbuffer", "rtph265depay", "h265parse"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("udp265://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr src = child(bin.get(), "udpsrc");
    QVERIFY2(src, "udp265:// must build a udpsrc");
    GstCaps* rawCaps = nullptr;
    g_object_get(src.get(), "caps", &rawCaps, nullptr);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(rawCaps);
    QVERIFY2(caps, "udp265:// must set H265 RTP caps on udpsrc");
    QCOMPARE(QString::fromUtf8(gst_structure_get_string(gst_caps_get_structure(caps.get(), 0), "encoding-name")),
             QStringLiteral("H265"));

    QVERIFY2(child(bin.get(), "rtpjitterbuffer"), "udp265 RTP path must insert a jitterbuffer");
    QVERIFY2(!child(bin.get(), "tsdemux"), "udp265 is RTP, not MPEG-TS; no tsdemux");
    QVERIFY2(child(bin.get(), "rtph265depay"), "udp265:// must depayload H265 RTP explicitly");

    const GStreamer::GstElementPtr parser = child(bin.get(), "h265parse");
    QVERIFY2(parser, "udp265:// must parse H265 explicitly");
    gint configInterval = 0;
    g_object_get(parser.get(), "config-interval", &configInterval, nullptr);
    QCOMPARE(configInterval, -1);
}

void GStreamerTest::_testSourceFactoryUdpH264Caps()
{
    if (!GstTest::firstMissingFactory({"udpsrc"}).isEmpty()) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr src = child(bin.get(), "udpsrc");
    QVERIFY(src);
    GstCaps* rawCaps = nullptr;
    g_object_get(src.get(), "caps", &rawCaps, nullptr);
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(rawCaps);
    QVERIFY2(caps, "udp:// must set H264 RTP caps");
    const GstStructure* st = gst_caps_get_structure(caps.get(), 0);
    QCOMPARE(QString::fromUtf8(gst_structure_get_string(st, "media")), QStringLiteral("video"));
    QCOMPARE(QString::fromUtf8(gst_structure_get_string(st, "encoding-name")), QStringLiteral("H264"));
}

void GStreamerTest::_testSourceFactoryDropsRtpUntilParameterSets()
{
    const QString missing = GstTest::firstMissingFactory(
        {"videotestsrc", "x264enc", "rtph264pay", "udpsink", "udpsrc", "parsebin", "rtph264depay", "fakesink"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    QUdpSocket portProbe;
    QVERIFY(portProbe.bind(QHostAddress::LocalHost, 0));
    const quint16 port = portProbe.localPort();
    portProbe.close();

    DepayloaderOutput output;
    std::atomic<int> sentFrames{0};

    // A GOP far longer than the test: the receiver joins after the only keyframe until the test forces one.
    GstTest::TestPipeline sender;
    QVERIFY2(
        sender.launch(QStringLiteral("videotestsrc is-live=true ! video/x-raw,width=160,height=120,framerate=30/1 ! "
                                     "x264enc name=encoder tune=zerolatency speed-preset=ultrafast "
                                     "key-int-max=100000 ! rtph264pay config-interval=-1 pt=96 ! "
                                     "udpsink host=127.0.0.1 port=%1 sync=false async=false")
                          .arg(port)),
        qPrintable(sender.error()));
    const GStreamer::GstElementPtr encoder = sender.element("encoder");
    QVERIFY(encoder);
    const GStreamer::GstPadPtr encoderSrc = GStreamer::adoptPad(gst_element_get_static_pad(encoder.get(), "src"));
    (void) gst_pad_add_probe(encoderSrc.get(), GST_PAD_PROBE_TYPE_BUFFER, GstTest::countBuffers, &sentFrames, nullptr);

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr source = createSource(QStringLiteral("udp://127.0.0.1:%1").arg(port), config);
    QVERIFY(source);
    (void) g_signal_connect(source.get(), "deep-element-added", G_CALLBACK(watchH264Depayloading), &output);

    GstTest::TestPipeline receiver(gst_pipeline_new("join-mid-gop"));
    GstElement* const sink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, "async", FALSE, nullptr);
    QVERIFY(gst_bin_add(receiver.bin(), source.get()));
    QVERIFY(gst_bin_add(receiver.bin(), sink));
    (void) g_signal_connect(source.get(), "pad-added", G_CALLBACK(linkToSink), sink);

    QVERIFY(sender.play());
    QVERIFY_TRUE_WAIT(sentFrames.load() >= 5, TestTimeout::mediumMs());
    QVERIFY(receiver.play());

    QVERIFY_TRUE_WAIT(output.packetsIn.load() >= 20, TestTimeout::mediumMs());
    QCOMPARE(output.unitsOut.load(), 0);

    QVERIFY(gst_pad_send_event(encoderSrc.get(),
                               gst_video_event_new_upstream_force_key_unit(GST_CLOCK_TIME_NONE, TRUE, 0)));
    QVERIFY_TRUE_WAIT(output.unitsOut.load() > 0, TestTimeout::mediumMs());
    QVERIFY2(output.firstUnitHasSps.load(), "the first unit reaching h264parse must carry the SPS");
}

void GStreamerTest::_testParameterSetGateHoldsUntilCompleteSet()
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

void GStreamerTest::_testParameterSetGatePassesLengthPrefixedOutput()
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

void GStreamerTest::_testSourceFactoryUdpMpegTs()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "tsdemux"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("mpegts://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr src = child(bin.get(), "udpsrc");
    QVERIFY2(src, "mpegts:// must build a udpsrc");
    GstCaps* rawCaps = nullptr;
    g_object_get(src.get(), "caps", &rawCaps, nullptr);
    QVERIFY2(!GStreamer::adoptCaps(rawCaps), "raw MPEG-TS over UDP must not force RTP caps on udpsrc");

    QVERIFY2(child(bin.get(), "tsdemux"), "mpegts:// must insert tsdemux");
    QVERIFY2(!child(bin.get(), "rtpjitterbuffer"), "raw MPEG-TS is not RTP; no jitterbuffer");
}

void GStreamerTest::_testSourceFactorySchemeCaseInsensitive()
{
    if (!GstTest::firstMissingFactory({"udpsrc"}).isEmpty()) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    QVERIFY2(createSource(QStringLiteral("UDP://127.0.0.1:5600"), config),
             "uppercase scheme must be accepted (scheme is lower-cased before matching)");
}

void GStreamerTest::_testSourceFactoryNegativeLatencyClamped()
{
    const QString missing = GstTest::firstMissingFactory({"udpsrc", "rtpjitterbuffer"});
    if (!missing.isEmpty()) {
        QSKIP(qPrintable(QStringLiteral("%1 plugin unavailable").arg(missing)));
    }

    GStreamer::SourceFactory::Config config;
    config.latencyMs = -100;
    const GStreamer::GstElementPtr bin = createSource(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);

    const GStreamer::GstElementPtr jb = child(bin.get(), "rtpjitterbuffer");
    QVERIFY(jb);
    guint latency = 99999;
    g_object_get(jb.get(), "latency", &latency, nullptr);
    QCOMPARE(latency, 0u);
}

void GStreamerTest::_testSourceFactoryDynamicRtpLinkFailureCleansJitterBuffer()
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
    const GStreamer::GstElementPtr udpsrc = child(source.bin.get(), "udpsrc");
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
    QVERIFY2(!child(source.bin.get(), "rtpjitterbuffer"),
             "a failed dynamic RTP pad link must remove its temporary jitterbuffer");
}

void GStreamerTest::_testSourceFactoryDynamicLinkSkipsNonVideoRtp()
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
    QVERIFY2(!child(source.bin.get(), "rtpjitterbuffer"), "an audio RTP pad must not get a jitterbuffer");

    QVERIFY(
        gst_element_add_pad(source.demux.get(), gst_pad_new_from_static_template(&sH264RtpPadTemplate, "video_rtp")));
    QVERIFY2(gst_pad_is_linked(source.parserSink.get()), "the video RTP pad that follows must still reach the parser");
}

#endif
