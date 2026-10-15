#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <gst/gst.h>

#include <QtCore/QHash>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "GstScoped.h"
#include "GstSourceFactory.h"

namespace {

// Borrowed (bin-owned) first child whose element-factory name matches, or nullptr.
GstElement* findChildByFactoryName(GstElement* bin, const char* factoryName)
{
    GstIterator* it = gst_bin_iterate_elements(GST_BIN(bin));
    GValue item = G_VALUE_INIT;
    GstElement* match = nullptr;
    bool done = false;
    while (!done) {
        switch (gst_iterator_next(it, &item)) {
            case GST_ITERATOR_OK: {
                GstElement* child = GST_ELEMENT(g_value_get_object(&item));
                GstElementFactory* f = gst_element_get_factory(child);
                if (f && (g_strcmp0(GST_OBJECT_NAME(f), factoryName) == 0)) {
                    match = child;
                    done = true;
                }
                g_value_reset(&item);
                break;
            }
            case GST_ITERATOR_RESYNC:
                gst_iterator_resync(it);
                break;
            default:
                done = true;
                break;
        }
    }
    g_value_unset(&item);
    gst_iterator_free(it);
    return match;
}

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
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    config.latencyMs = 80;
    config.doRetransmission = true;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* jb = findChildByFactoryName(bin, "rtpjitterbuffer");
    QVERIFY2(jb, "static UDP-RTP path must insert an rtpjitterbuffer at NULL state");

    guint latency = 0;
    gboolean doLost = FALSE, doRtx = TRUE, dropOnLatency = FALSE;
    g_object_get(jb, "latency", &latency, "do-lost", &doLost, "do-retransmission", &doRtx, "drop-on-latency",
                 &dropOnLatency, nullptr);
    QCOMPARE(latency, 80u);
    QCOMPARE(doLost, TRUE);
    QVERIFY2(!doRtx, "plain UDP has no RTCP back-channel, so retransmission requests could never reach the sender");
    QCOMPARE(dropOnLatency, TRUE);
}

void GStreamerTest::_testSourceFactoryJitterBufferNone()
{
    if (!gst_element_factory_find("udpsrc")) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::None;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"),
             "JitterBuffer::None must link the source straight to the parser");
}

void GStreamerTest::_testSourceFactoryNoRetransmission()
{
    if (!gst_element_factory_find("rtspsrc")) {
        QSKIP("rtspsrc plugin unavailable");
    }

    // RTSP is the only source with a back-channel for retransmission requests, so it's the one the option reaches.
    GStreamer::SourceFactory::Config config;
    config.doRetransmission = false;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* source = findChildByFactoryName(bin, "rtspsrc");
    QVERIFY(source);

    gboolean doRtx = TRUE;
    g_object_get(source, "do-retransmission", &doRtx, nullptr);
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
    if (!gst_element_factory_find("rtspsrc")) {
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

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* source = findChildByFactoryName(bin, "rtspsrc");
    QVERIFY(source);

    guint latencyMs = 0;
    gboolean doRetransmission = FALSE;
    gboolean dropOnLatency = FALSE;
    g_object_get(source, "latency", &latencyMs, "do-retransmission", &doRetransmission, "drop-on-latency",
                 &dropOnLatency, nullptr);
    QCOMPARE(latencyMs, expectedLatencyMs);
    QCOMPARE(doRetransmission, expectedRetransmission ? TRUE : FALSE);
    QCOMPARE(dropOnLatency, expectedDropOnLatency ? TRUE : FALSE);

    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"),
             "rtspsrc owns its internal jitterbuffer; the factory must not add a second one");

    // rtspsrc only creates its rtpbin during SETUP; drive the same signals it emits so the
    // per-stream internal jitterbuffer policy can be checked without a server.
    GStreamer::GstElementPtr manager = GStreamer::adoptElement(gst_element_factory_make("rtpbin", nullptr));
    GStreamer::GstElementPtr internalBuffer =
        GStreamer::adoptElement(gst_element_factory_make("rtpjitterbuffer", nullptr));
    QVERIFY(manager);
    QVERIFY(internalBuffer);
    g_signal_emit_by_name(source, "new-manager", manager.get());
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
    if (!gst_element_factory_find("rtspsrc")) {
        QSKIP("rtspsrc plugin unavailable");
    }

    QFETCH(QString, capsName);
    QFETCH(QString, media);
    QFETCH(bool, expectedSelected);

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* source = findChildByFactoryName(bin, "rtspsrc");
    QVERIFY(source);

    const QByteArray capsNameUtf8 = capsName.toUtf8();
    const QByteArray mediaUtf8 = media.toUtf8();
    const GStreamer::GstCapsPtr caps = GStreamer::adoptCaps(gst_caps_new_simple(
        capsNameUtf8.constData(), "media", G_TYPE_STRING, mediaUtf8.constData(), "payload", G_TYPE_INT, 96,
        "clock-rate", G_TYPE_INT, 90000, "encoding-name", G_TYPE_STRING, "H264", nullptr));
    QVERIFY(caps);

    gboolean selected = expectedSelected ? FALSE : TRUE;
    g_signal_emit_by_name(source, "select-stream", 0u, caps.get(), &selected);
    QCOMPARE(selected, expectedSelected ? TRUE : FALSE);
}

void GStreamerTest::_testSourceFactoryRtspSetsUpOnlyVideoStream()
{
    if (!gst_element_factory_find("rtspsrc")) {
        QSKIP("rtspsrc plugin unavailable");
    }

    // libproxy warns about host proxy bypass entries it cannot parse (e.g. "169.254/16" on macOS runners).
    ignoreLogMessage("Video.GStreamer.GStreamerLogging", QtWarningMsg,
                     QRegularExpression(QStringLiteral("\"pxbackend\" \"Could not parse ignore mask")));

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

    GstElement* pipeline = gst_pipeline_new("rtsp-select-stream-test");
    QVERIFY(pipeline);
    const auto cleanup = qScopeGuard([&] {
        (void) gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
    });

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(server.url(), config);
    QVERIFY(bin);
    QVERIFY(gst_bin_add(GST_BIN(pipeline), bin));
    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PAUSED) != GST_STATE_CHANGE_FAILURE);

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
    QVERIFY(!GStreamer::SourceFactory::create(QString(), config));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("ftp://127.0.0.1/x"), config));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:0"), config));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:99999"), config));
}

void GStreamerTest::_testSourceFactoryTcpMpegTs()
{
    if (!gst_element_factory_find("tcpclientsrc") || !gst_element_factory_find("tsdemux")) {
        QSKIP("tcpclientsrc/tsdemux plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("tcp://192.168.1.50:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* src = findChildByFactoryName(bin, "tcpclientsrc");
    QVERIFY2(src, "tcp:// must build a tcpclientsrc");
    gchar* host = nullptr;
    gint port = 0;
    g_object_get(src, "host", &host, "port", &port, nullptr);
    const auto hostCleanup = qScopeGuard([&] { g_free(host); });
    QCOMPARE(QString::fromUtf8(host), QStringLiteral("192.168.1.50"));
    QCOMPARE(port, 5600);

    QVERIFY2(findChildByFactoryName(bin, "tsdemux"), "MPEG-TS over TCP must insert tsdemux explicitly");
    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"), "raw MPEG-TS is not RTP; no jitterbuffer");
}

void GStreamerTest::_testSourceFactoryRejectsBadTcpUri()
{
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Invalid TCP port|Missing host in TCP URI")));

    GStreamer::SourceFactory::Config config;
    QVERIFY2(!GStreamer::SourceFactory::create(QStringLiteral("tcp://192.168.1.50"), config),
             "tcp:// without a port must be rejected");
    QVERIFY2(!GStreamer::SourceFactory::create(QStringLiteral("tcp://:5600"), config),
             "tcp:// without a host must be rejected");
    QVERIFY2(!GStreamer::SourceFactory::create(QStringLiteral("tcp://192.168.1.50:0"), config),
             "tcp:// with port 0 must be rejected");
}

void GStreamerTest::_testSourceFactoryUdp265Caps()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp265://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* src = findChildByFactoryName(bin, "udpsrc");
    QVERIFY2(src, "udp265:// must build a udpsrc");

    GstCaps* caps = nullptr;
    g_object_get(src, "caps", &caps, nullptr);
    QVERIFY2(caps, "udp265:// must set H265 RTP caps on udpsrc");
    const GstStructure* st = gst_caps_get_structure(caps, 0);
    const gchar* encName = gst_structure_get_string(st, "encoding-name");
    QCOMPARE(QString::fromUtf8(encName), QStringLiteral("H265"));
    gst_caps_unref(caps);

    QVERIFY2(findChildByFactoryName(bin, "rtpjitterbuffer"), "udp265 RTP path must insert a jitterbuffer");
    QVERIFY2(!findChildByFactoryName(bin, "tsdemux"), "udp265 is RTP, not MPEG-TS; no tsdemux");
}

void GStreamerTest::_testSourceFactoryUdp265UsesExplicitDepayAndParser()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtph265depay") ||
        !gst_element_factory_find("h265parse")) {
        QSKIP("udp265 RTP/H265 plugins unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp265://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    QVERIFY2(findChildByFactoryName(bin, "rtph265depay"), "udp265:// must depayload H265 RTP explicitly");

    GstElement* parser = findChildByFactoryName(bin, "h265parse");
    QVERIFY2(parser, "udp265:// must parse H265 explicitly");

    gint configInterval = 0;
    g_object_get(parser, "config-interval", &configInterval, nullptr);
    QCOMPARE(configInterval, -1);
}

void GStreamerTest::_testSourceFactoryUdpH264Caps()
{
    if (!gst_element_factory_find("udpsrc")) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* src = findChildByFactoryName(bin, "udpsrc");
    QVERIFY(src);

    GstCaps* caps = nullptr;
    g_object_get(src, "caps", &caps, nullptr);
    QVERIFY2(caps, "udp:// must set H264 RTP caps");
    const GstStructure* st = gst_caps_get_structure(caps, 0);
    const gchar* media = gst_structure_get_string(st, "media");
    const gchar* encName = gst_structure_get_string(st, "encoding-name");
    QCOMPARE(QString::fromUtf8(media), QStringLiteral("video"));
    QCOMPARE(QString::fromUtf8(encName), QStringLiteral("H264"));
    gst_caps_unref(caps);
}

void GStreamerTest::_testSourceFactoryUdpMpegTs()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("tsdemux")) {
        QSKIP("udpsrc/tsdemux plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("mpegts://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* src = findChildByFactoryName(bin, "udpsrc");
    QVERIFY2(src, "mpegts:// must build a udpsrc");

    GstCaps* caps = nullptr;
    g_object_get(src, "caps", &caps, nullptr);
    if (caps) {
        gst_caps_unref(caps);
    }
    QVERIFY2(!caps, "raw MPEG-TS over UDP must not force RTP caps on udpsrc");

    QVERIFY2(findChildByFactoryName(bin, "tsdemux"), "mpegts:// must insert tsdemux");
    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"), "raw MPEG-TS is not RTP; no jitterbuffer");
}

void GStreamerTest::_testSourceFactorySchemeCaseInsensitive()
{
    if (!gst_element_factory_find("udpsrc")) {
        QSKIP("udpsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("UDP://127.0.0.1:5600"), config);
    QVERIFY2(bin, "uppercase scheme must be accepted (scheme is lower-cased before matching)");
    gst_object_unref(bin);
}

void GStreamerTest::_testSourceFactoryNegativeLatencyClamped()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.latencyMs = -100;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* jb = findChildByFactoryName(bin, "rtpjitterbuffer");
    QVERIFY(jb);
    guint latency = 99999;
    g_object_get(jb, "latency", &latency, nullptr);
    QCOMPARE(latency, 0u);
}

void GStreamerTest::_testSourceFactoryDynamicRtpLinkFailureCleansJitterBuffer()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("tsdemux") ||
        !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/tsdemux/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("mpegts://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] {
        (void) gst_element_set_state(bin, GST_STATE_NULL);
        gst_object_unref(bin);
    });

    // A running bin syncs the temporary jitterbuffer out of NULL, so removing it without a NULL
    // transition raises GStreamer's "disposing element in READY/PLAYING" critical. The source stays
    // locked in NULL so the test doesn't bind the UDP port.
    GstElement* udpsrc = findChildByFactoryName(bin, "udpsrc");
    QVERIFY(udpsrc);
    (void) gst_element_set_locked_state(udpsrc, TRUE);
    QCOMPARE(gst_element_set_state(bin, GST_STATE_READY), GST_STATE_CHANGE_SUCCESS);

    GstElement* tsdemux = findChildByFactoryName(bin, "tsdemux");
    QVERIFY(tsdemux);
    GstElement* parser = findChildByFactoryName(bin, "parsebin");
    QVERIFY(parser);

    GstPad* parserSink = gst_element_get_static_pad(parser, "sink");
    QVERIFY(parserSink);
    const auto parserSinkCleanup = qScopeGuard([&] { gst_object_unref(parserSink); });
    QVERIFY(!gst_pad_is_linked(parserSink));

    static GstStaticPadTemplate sRtpPadTemplate = GST_STATIC_PAD_TEMPLATE(
        "stray_rtp", GST_PAD_SRC, GST_PAD_SOMETIMES,
        GST_STATIC_CAPS("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264"));

    GstPad* strayPad = gst_pad_new_from_static_template(&sRtpPadTemplate, "stray_rtp");
    QVERIFY(strayPad);
    const auto padCleanup = qScopeGuard([&] { gst_object_unref(strayPad); });

    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("gst_element_link_pads\\(\\) failed")));
    g_signal_emit_by_name(tsdemux, "pad-added", strayPad);

    QVERIFY2(!gst_pad_is_linked(parserSink),
             "a failed dynamic RTP pad link must leave parsebin available for the next valid pad");
    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"),
             "a failed dynamic RTP pad link must remove its temporary jitterbuffer");
}

void GStreamerTest::_testSourceFactoryDynamicLinkSkipsNonVideoRtp()
{
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("tsdemux") ||
        !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/tsdemux/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("mpegts://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* demux = findChildByFactoryName(bin, "tsdemux");
    QVERIFY(demux);
    GstElement* parser = findChildByFactoryName(bin, "parsebin");
    QVERIFY(parser);
    const GStreamer::GstPadPtr parserSink = GStreamer::adoptPad(gst_element_get_static_pad(parser, "sink"));
    QVERIFY(parserSink);

    // Mirrors an RTSP SDP listing audio before video: the first RTP pad must not claim the parser.
    static GstStaticPadTemplate sAudioTemplate = GST_STATIC_PAD_TEMPLATE(
        "audio_rtp", GST_PAD_SRC, GST_PAD_SOMETIMES,
        GST_STATIC_CAPS("application/x-rtp, media=(string)audio, clock-rate=(int)48000, encoding-name=(string)OPUS"));
    static GstStaticPadTemplate sVideoTemplate = GST_STATIC_PAD_TEMPLATE(
        "video_rtp", GST_PAD_SRC, GST_PAD_SOMETIMES,
        GST_STATIC_CAPS("application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264"));

    // gst_element_add_pad emits pad-added and consumes the floating pad.
    QVERIFY(gst_element_add_pad(demux, gst_pad_new_from_static_template(&sAudioTemplate, "audio_rtp")));
    QVERIFY2(!gst_pad_is_linked(parserSink.get()), "an audio RTP pad must not be linked to the video parser");
    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"), "an audio RTP pad must not get a jitterbuffer");

    QVERIFY(gst_element_add_pad(demux, gst_pad_new_from_static_template(&sVideoTemplate, "video_rtp")));
    QVERIFY2(gst_pad_is_linked(parserSink.get()), "the video RTP pad that follows must still reach the parser");
}

#endif
