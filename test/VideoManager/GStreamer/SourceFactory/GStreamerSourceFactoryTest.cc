#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtGui/QImage>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <atomic>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <mutex>
#include <utility>

#ifdef QGC_HAS_WEBSOCKET_VIDEO
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#endif

#include "GstSourceFactory.h"
#ifdef QGC_HAS_WEBSOCKET_VIDEO
#include "QGCWebSocketVideoSource.h"
#endif

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

struct BufferProbeContext
{
    std::mutex mutex;
    QByteArray data;
    std::atomic_bool observed{false};
};

GstPadProbeReturn captureBufferProbe(GstPad*, GstPadProbeInfo* info, gpointer userData)
{
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer) {
        return GST_PAD_PROBE_OK;
    }

    GstMapInfo map = GST_MAP_INFO_INIT;
    if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
        auto* context = static_cast<BufferProbeContext*>(userData);
        {
            const std::lock_guard<std::mutex> lock(context->mutex);
            context->data = QByteArray(reinterpret_cast<const char*>(map.data), static_cast<qsizetype>(map.size));
        }
        context->observed.store(true, std::memory_order_release);
        gst_buffer_unmap(buffer, &map);
    }
    return GST_PAD_PROBE_OK;
}

QByteArray createTestJpeg()
{
    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return {};
    }

    QImage image(4, 4, QImage::Format_RGB32);
    image.fill(Qt::red);
    if (!image.save(&buffer, "JPEG")) {
        return {};
    }
    return jpeg;
}

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
    gboolean doLost = FALSE, doRtx = FALSE, dropOnLatency = FALSE;
    gint rtxDelay = 0, rtxMaxRetries = 0;
    g_object_get(jb, "latency", &latency, "do-lost", &doLost, "do-retransmission", &doRtx, "drop-on-latency",
                 &dropOnLatency, "rtx-delay", &rtxDelay, "rtx-max-retries", &rtxMaxRetries, nullptr);
    QCOMPARE(latency, 80u);
    QCOMPARE(doLost, TRUE);
    QCOMPARE(doRtx, TRUE);
    QCOMPARE(dropOnLatency, TRUE);
    QCOMPARE(rtxDelay, 25);
    QCOMPARE(rtxMaxRetries, 1);
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
    if (!gst_element_factory_find("udpsrc") || !gst_element_factory_find("rtpjitterbuffer")) {
        QSKIP("udpsrc/rtpjitterbuffer plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::Buffered;
    config.doRetransmission = false;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("udp://127.0.0.1:5600"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    GstElement* jb = findChildByFactoryName(bin, "rtpjitterbuffer");
    QVERIFY(jb);

    gboolean doRtx = TRUE, dropOnLatency = TRUE;
    g_object_get(jb, "do-retransmission", &doRtx, "drop-on-latency", &dropOnLatency, nullptr);
    QCOMPARE(doRtx, FALSE);
    QCOMPARE(dropOnLatency, FALSE);
}

void GStreamerTest::_testSourceFactoryRtspExcludesStaticJitterBuffer()
{
    if (!gst_element_factory_find("rtspsrc")) {
        QSKIP("rtspsrc plugin unavailable");
    }

    GStreamer::SourceFactory::Config config;
    config.jitterBuffer = GStreamer::SourceFactory::JitterBuffer::DropOnLatency;

    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("rtsp://127.0.0.1:8554/test"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"),
             "rtspsrc owns its internal jitterbuffer; the factory must not add a second one");
}

void GStreamerTest::_testSourceFactoryRejectsBadUri()
{
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtCriticalMsg,
                     QRegularExpression(QStringLiteral("URI is not specified|Invalid UDP port")));
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Unsupported URI scheme")));

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
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtCriticalMsg,
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
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

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

void GStreamerTest::_testSourceFactoryHttpMjpeg()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse")) {
        QSKIP("HTTP MJPEG GStreamer plugins unavailable");
    }

    GStreamer::SourceFactory::Config config;
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("http://127.0.0.1:5077/video_feed"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    QVERIFY2(findChildByFactoryName(bin, "souphttpsrc"), "http:// must build souphttpsrc");
    QVERIFY2(findChildByFactoryName(bin, "multipartdemux"), "HTTP MJPEG must demux multipart/x-mixed-replace");
    QVERIFY2(findChildByFactoryName(bin, "jpegparse"), "HTTP MJPEG must expose parsed JPEG frames");
    QVERIFY2(!findChildByFactoryName(bin, "rtpjitterbuffer"), "HTTP MJPEG is not RTP; no jitterbuffer");
}

void GStreamerTest::_testSourceFactoryHttpMjpegAuthRequiresHttps()
{
    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Bearer;
    config.networkSourceConfig.secret = QByteArrayLiteral("secret-token");
    const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });

    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Authenticated HTTP MJPEG video requires HTTPS")));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("http://127.0.0.1:5077/video_feed"), config));
}

void GStreamerTest::_testSourceFactoryHttpMjpegSecurityProperties()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse")) {
        QSKIP("HTTP MJPEG GStreamer plugins unavailable");
    }

    {
        GStreamer::SourceFactory::Config config;
        config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Basic;
        config.networkSourceConfig.username = QStringLiteral("viewer");
        config.networkSourceConfig.secret = QByteArrayLiteral("test-password");
        const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });

        GstElement* bin =
            GStreamer::SourceFactory::create(QStringLiteral("https://camera.example/mjpeg"), config);
        QVERIFY(bin);
        const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

        GstElement* source = findChildByFactoryName(bin, "souphttpsrc");
        QVERIFY(source);
        gboolean automaticRedirect = TRUE;
        gboolean strictTls = FALSE;
        gchar* userId = nullptr;
        gchar* userPassword = nullptr;
        g_object_get(source, "automatic-redirect", &automaticRedirect, "ssl-strict", &strictTls, "user-id", &userId,
                     "user-pw", &userPassword, nullptr);
        const auto credentialsCleanup = qScopeGuard([&] {
            g_free(userId);
            g_free(userPassword);
        });
        QCOMPARE(automaticRedirect, FALSE);
        QCOMPARE(strictTls, TRUE);
        QCOMPARE(QByteArray(userId), QByteArrayLiteral("viewer"));
        QCOMPARE(QByteArray(userPassword), QByteArrayLiteral("test-password"));
    }

    {
        GStreamer::SourceFactory::Config config;
        config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Bearer;
        config.networkSourceConfig.secret = QByteArrayLiteral("test-token");
        config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
        const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });

        GstElement* bin =
            GStreamer::SourceFactory::create(QStringLiteral("https://camera.example/mjpeg"), config);
        QVERIFY(bin);
        const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

        GstElement* source = findChildByFactoryName(bin, "souphttpsrc");
        QVERIFY(source);
        gboolean automaticRedirect = TRUE;
        GstStructure* headers = nullptr;
        g_object_get(source, "automatic-redirect", &automaticRedirect, "extra-headers", &headers, nullptr);
        QVERIFY(headers);
        const auto headersCleanup = qScopeGuard([&] { gst_structure_free(headers); });
        QCOMPARE(automaticRedirect, FALSE);
        QCOMPARE(QByteArray(gst_structure_get_string(headers, "Authorization")),
                 QByteArrayLiteral("Bearer test-token"));
        QCOMPARE(QByteArray(gst_structure_get_string(headers, "Origin")),
                 QByteArrayLiteral("https://operator.example.test"));
    }
}

void GStreamerTest::_testHttpMjpegDelivery()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse") || !gst_element_factory_find("fakesink")) {
        QSKIP("HTTP MJPEG GStreamer plugins unavailable");
    }

    const QByteArray jpeg = createTestJpeg();
    QVERIFY(!jpeg.isEmpty());

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QTcpSocket* serverSocket = nullptr;
    QByteArray requestBytes;
    bool responseSent = false;
    bool responseWriteAccepted = false;
    connect(&server, &QTcpServer::newConnection, this, [&]() {
        serverSocket = server.nextPendingConnection();
        if (!serverSocket) {
            return;
        }
        connect(serverSocket, &QTcpSocket::readyRead, this, [&]() {
            requestBytes += serverSocket->readAll();
            if (responseSent || !requestBytes.contains("\r\n\r\n")) {
                return;
            }

            QByteArray response = QByteArrayLiteral(
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                "Cache-Control: no-store\r\n"
                "Connection: close\r\n\r\n"
                "--frame\r\n"
                "Content-Type: image/jpeg\r\n");
            response +=
                QByteArrayLiteral("Content-Length: ") + QByteArray::number(jpeg.size()) + QByteArrayLiteral("\r\n\r\n");
            response += jpeg;
            response += QByteArrayLiteral("\r\n--frame\r\n");
            responseWriteAccepted = serverSocket->write(response) == static_cast<qint64>(response.size());
            serverSocket->flush();
            responseSent = true;
        });
    });

    GStreamer::SourceFactory::Config config;
    config.timeoutS = 2;
    config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
    const QString url = QStringLiteral("http://127.0.0.1:%1/mjpeg").arg(server.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });

    GstElement* httpSource = findChildByFactoryName(sourceBin, "souphttpsrc");
    QVERIFY(httpSource);
    gboolean automaticRedirect = TRUE;
    gboolean strictTls = FALSE;
    g_object_get(httpSource, "automatic-redirect", &automaticRedirect, "ssl-strict", &strictTls, nullptr);
    QCOMPARE(automaticRedirect, FALSE);
    QCOMPARE(strictTls, TRUE);

    GstElement* parser = findChildByFactoryName(sourceBin, "jpegparse");
    QVERIFY(parser);
    GstPad* parserPad = gst_element_get_static_pad(parser, "src");
    QVERIFY(parserPad);
    BufferProbeContext probeContext;
    const gulong bufferProbeId =
        gst_pad_add_probe(parserPad, GST_PAD_PROBE_TYPE_BUFFER, captureBufferProbe, &probeContext, nullptr);
    QVERIFY(bufferProbeId != 0);
    const auto probeCleanup = qScopeGuard([&] {
        gst_pad_remove_probe(parserPad, bufferProbeId);
        gst_object_unref(parserPad);
    });

    GstElement* pipeline = gst_pipeline_new("http-mjpeg-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, nullptr);
    GstElement* sourceElement = sourceBin;
    gst_bin_add_many(GST_BIN(pipeline), sourceBin, sink, nullptr);
    sourceCleanup.dismiss();
    sourceBin = nullptr;
    const auto pipelineCleanup = qScopeGuard([&] {
        (void) gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_clear_object(&pipeline);
    });
    QVERIFY(gst_element_link(sourceElement, sink));

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    QVERIFY_TRUE_WAIT(probeContext.observed.load(std::memory_order_acquire), TestTimeout::mediumMs());
    QVERIFY(responseSent);
    QVERIFY(responseWriteAccepted);
    QVERIFY(requestBytes.startsWith("GET /mjpeg HTTP/1.1\r\n"));
    const QByteArray lowerRequest = requestBytes.toLower();
    QVERIFY(lowerRequest.contains("origin: https://operator.example.test\r\n"));
    QVERIFY(!lowerRequest.contains("authorization:"));

    QByteArray observedJpeg;
    {
        const std::lock_guard<std::mutex> lock(probeContext.mutex);
        observedJpeg = probeContext.data;
    }
    QCOMPARE(observedJpeg, jpeg);
    if (serverSocket) {
        serverSocket->disconnectFromHost();
    }
}

void GStreamerTest::_testSourceFactoryWebSocketJpeg()
{
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    GStreamer::SourceFactory::Config config;
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("ws://127.0.0.1:5077/ws/video_feed"), config);
    QVERIFY(bin);
    const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

    QVERIFY2(findChildByFactoryName(bin, "appsrc"), "ws:// must build an appsrc bridge");
    QVERIFY2(findChildByFactoryName(bin, "jpegparse"), "WebSocket JPEG must expose parsed JPEG frames");
#else
    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("WebSocket JPEG support is unavailable")));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("ws://127.0.0.1:5077/ws/video_feed"), config));
#endif
}

void GStreamerTest::_testSourceFactoryWebSocketAuthRequiresWss()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Bearer;
    config.networkSourceConfig.secret = QByteArrayLiteral("test-token");
    const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });

    ignoreLogMessage("Video.GStreamer.GstSourceFactory", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Authenticated WebSocket JPEG video requires WSS")));
    QVERIFY(!GStreamer::SourceFactory::create(QStringLiteral("ws://127.0.0.1:5078/video"), config));
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testWebSocketJpegValidation()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    QVERIFY(QGCWebSocketVideoSource::isCompleteJpeg(QByteArray::fromHex("ffd800ffd9")));
    QVERIFY(!QGCWebSocketVideoSource::isCompleteJpeg(QByteArray::fromHex("ffd800")));
    QVERIFY(!QGCWebSocketVideoSource::isCompleteJpeg(QByteArray::fromHex("000000ffd9")));
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testWebSocketJpegDelivery()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse") ||
        !gst_element_factory_find("fakesink")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    QWebSocketServer server(QStringLiteral("QGC video delivery test"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QWebSocket* serverSocket = nullptr;
    connect(&server, &QWebSocketServer::newConnection, this,
            [&server, &serverSocket]() { serverSocket = server.nextPendingConnection(); });

    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
    const QString url = QStringLiteral("ws://127.0.0.1:%1/video").arg(server.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });

    GstElement* appsrc = findChildByFactoryName(sourceBin, "appsrc");
    QVERIFY(appsrc);
    guint64 maximumBuffers = 0;
    guint64 maximumBytes = 0;
    GstAppLeakyType leakyType = GST_APP_LEAKY_TYPE_NONE;
    g_object_get(appsrc, "max-buffers", &maximumBuffers, "max-bytes", &maximumBytes, "leaky-type", &leakyType, nullptr);
    QCOMPARE(maximumBuffers, static_cast<guint64>(4));
    QCOMPARE(maximumBytes, static_cast<guint64>(64U * 1024U * 1024U));
    QCOMPARE(leakyType, GST_APP_LEAKY_TYPE_DOWNSTREAM);
    GstPad* appsrcPad = gst_element_get_static_pad(appsrc, "src");
    QVERIFY(appsrcPad);

    BufferProbeContext probeContext;
    const gulong bufferProbeId =
        gst_pad_add_probe(appsrcPad, GST_PAD_PROBE_TYPE_BUFFER, captureBufferProbe, &probeContext, nullptr);
    QVERIFY(bufferProbeId != 0);
    const auto probeCleanup = qScopeGuard([&] {
        gst_pad_remove_probe(appsrcPad, bufferProbeId);
        gst_object_unref(appsrcPad);
    });

    GstElement* pipeline = gst_pipeline_new("websocket-jpeg-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
    g_object_set(sink, "sync", FALSE, nullptr);
    GstElement* sourceElement = sourceBin;
    gst_bin_add_many(GST_BIN(pipeline), sourceBin, sink, nullptr);
    sourceCleanup.dismiss();
    sourceBin = nullptr;
    const auto pipelineCleanup = qScopeGuard([&] {
        (void) gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_clear_object(&pipeline);
    });
    QVERIFY(gst_element_link(sourceElement, sink));

    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    QVERIFY_TRUE_WAIT(serverSocket != nullptr, TestTimeout::mediumMs());
    QCOMPARE(serverSocket->origin(), config.networkSourceConfig.origin);
    QVERIFY(!serverSocket->request().rawHeader("User-Agent").isEmpty());

    const QByteArray jpeg = createTestJpeg();
    QVERIFY(!jpeg.isEmpty());
    QVERIFY(QGCWebSocketVideoSource::isCompleteJpeg(jpeg));
    QCOMPARE(serverSocket->sendBinaryMessage(jpeg), static_cast<qint64>(jpeg.size()));
    serverSocket->flush();

    QVERIFY_TRUE_WAIT(probeContext.observed.load(std::memory_order_acquire), TestTimeout::mediumMs());
    QByteArray observedJpeg;
    {
        const std::lock_guard<std::mutex> lock(probeContext.mutex);
        observedJpeg = probeContext.data;
    }
    QCOMPARE(observedJpeg, jpeg);

    serverSocket->close();
    serverSocket->deleteLater();
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testWebSocketThreadTeardown()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    QWebSocketServer server(QStringLiteral("QGC video teardown test"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QList<QWebSocket*> serverSockets;
    connect(&server, &QWebSocketServer::newConnection, this, [&server, &serverSockets]() {
        while (server.hasPendingConnections()) {
            serverSockets.append(server.nextPendingConnection());
        }
    });

    for (int iteration = 0; iteration < 3; ++iteration) {
        GStreamer::SourceFactory::Config config;
        const QString url = QStringLiteral("ws://127.0.0.1:%1/video").arg(server.serverPort());
        GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
        QVERIFY(sourceBin);
        QCOMPARE_TRUE_WAIT(serverSockets.size(), iteration + 1, TestTimeout::mediumMs());

        QWebSocket* socket = serverSockets.at(iteration);
        QSignalSpy disconnectedSpy(socket, &QWebSocket::disconnected);
        QElapsedTimer elapsed;
        elapsed.start();
        gst_object_unref(sourceBin);
        QVERIFY2(elapsed.elapsed() < TestTimeout::mediumMs(), "WebSocket source teardown exceeded its normal bound");
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            QVERIFY_SIGNAL_WAIT(disconnectedSpy, TestTimeout::mediumMs());
        }
        QCOMPARE(socket->state(), QAbstractSocket::UnconnectedState);
    }

    for (QWebSocket* socket : std::as_const(serverSockets)) {
        delete socket;
    }
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

#endif
