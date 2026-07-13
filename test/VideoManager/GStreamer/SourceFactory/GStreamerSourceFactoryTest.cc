#include "GStreamerTest.h"

#ifdef QGC_GST_STREAMING

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QScopeGuard>
#include <QtCore/QTemporaryDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QUrl>
#include <QtGui/QImage>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslServer>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtTest/QSignalSpy>
#include <atomic>
#include <functional>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <mutex>
#include <utility>

#ifdef QGC_HAS_WEBSOCKET_VIDEO
#include <QtWebSockets/QWebSocket>
#include <QtWebSockets/QWebSocketServer>
#endif

#include "GstSourceFactory.h"
#include "GstVideoReceiver.h"
#include "NetworkVideoTlsFixture.h"
#include "QGCJpegStreamGuard.h"
#ifdef QGC_HAS_WEBSOCKET_VIDEO
#include "QGCWebSocketVideoSource.h"
#endif

namespace {

class RawConnectionObservingSslServer final : public QSslServer
{
public:
    int rawConnectionCount() const { return _rawConnectionCount; }

    void setRawConnectionObserver(std::function<void()> observer) { _rawConnectionObserver = std::move(observer); }

protected:
    void incomingConnection(qintptr socketDescriptor) override
    {
        ++_rawConnectionCount;
        if (_rawConnectionObserver) {
            _rawConnectionObserver();
        }
        QSslServer::incomingConnection(socketDescriptor);
    }

private:
    int _rawConnectionCount = 0;
    std::function<void()> _rawConnectionObserver;
};

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

QByteArray createTestJpeg(int size = 4)
{
    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return {};
    }

    QImage image(size, size, QImage::Format_RGB32);
    image.fill(Qt::red);
    if (!image.save(&buffer, "JPEG")) {
        return {};
    }
    return jpeg;
}

GstMessage* waitForBusMessage(GstBus* bus, GstMessageType types, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        if (GstMessage* message = gst_bus_pop_filtered(bus, types)) {
            return message;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(5);
    }
    return gst_bus_pop_filtered(bus, types);
}

QSslConfiguration testServerSslConfiguration()
{
    QSslConfiguration configuration = QSslConfiguration::defaultConfiguration();
    configuration.setLocalCertificateChain(
        QSslCertificate::fromData(QByteArray(NetworkVideoTlsFixture::kServerCertificatePem), QSsl::Pem));
    configuration.setPrivateKey(
        QSslKey(QByteArray::fromBase64(NetworkVideoTlsFixture::kServerPrivateKeyDerBase64), QSsl::Rsa, QSsl::Der));
    configuration.setPeerVerifyMode(QSslSocket::VerifyNone);
    return configuration;
}

bool writeTestCaFile(QFile& file)
{
    return file.open(QIODevice::WriteOnly) && file.write(NetworkVideoTlsFixture::kCaCertificatePem) > 0 && file.flush();
}

bool replaceJpegDimensions(QByteArray& jpeg, quint16 width, quint16 height)
{
    for (qsizetype offset = 0; (offset + 8) < jpeg.size(); ++offset) {
        if (static_cast<quint8>(jpeg.at(offset)) != 0xFF) {
            continue;
        }
        const quint8 marker = static_cast<quint8>(jpeg.at(offset + 1));
        const bool isStartOfFrame = ((marker >= 0xC0) && (marker <= 0xC3)) || ((marker >= 0xC5) && (marker <= 0xC7)) ||
                                    ((marker >= 0xC9) && (marker <= 0xCB)) || ((marker >= 0xCD) && (marker <= 0xCF));
        if (!isStartOfFrame) {
            continue;
        }
        jpeg[offset + 5] = static_cast<char>((height >> 8) & 0xFF);
        jpeg[offset + 6] = static_cast<char>(height & 0xFF);
        jpeg[offset + 7] = static_cast<char>((width >> 8) & 0xFF);
        jpeg[offset + 8] = static_cast<char>(width & 0xFF);
        return true;
    }
    return false;
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

        GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("https://camera.example/mjpeg"), config);
        QVERIFY(bin);
        const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

        GstElement* source = findChildByFactoryName(bin, "souphttpsrc");
        QVERIFY(source);
        gboolean automaticRedirect = TRUE;
        gboolean strictTls = FALSE;
        GObject* tlsDatabase = nullptr;
        gchar* userId = nullptr;
        gchar* userPassword = nullptr;
        g_object_get(source, "automatic-redirect", &automaticRedirect, "ssl-strict", &strictTls, "tls-database",
                     &tlsDatabase, "user-id", &userId, "user-pw", &userPassword, nullptr);
        const auto credentialsCleanup = qScopeGuard([&] {
            g_clear_object(&tlsDatabase);
            g_free(userId);
            g_free(userPassword);
        });
        QCOMPARE(automaticRedirect, FALSE);
        QCOMPARE(strictTls, TRUE);
        QVERIFY(!tlsDatabase);
        QCOMPARE(QByteArray(userId), QByteArrayLiteral("viewer"));
        QCOMPARE(QByteArray(userPassword), QByteArrayLiteral("test-password"));
    }

    {
        GStreamer::SourceFactory::Config config;
        config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Bearer;
        config.networkSourceConfig.secret = QByteArrayLiteral("test-token");
        config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
        const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });

        GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("https://camera.example/mjpeg"), config);
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

    {
        QTemporaryDir caDirectory;
        QVERIFY(caDirectory.isValid());
        const QString caPath = QDir(caDirectory.path()).filePath(QStringLiteral("qgc-\u6d4b\u8bd5-ca.pem"));
        QFile caFile(caPath);
        QVERIFY(writeTestCaFile(caFile));
        caFile.close();

        GStreamer::SourceFactory::Config config;
        config.networkSourceConfig.caCertificateFile = caPath;
        GstElement* bin = GStreamer::SourceFactory::create(QStringLiteral("https://camera.example/mjpeg"), config);
        QVERIFY(bin);
        const auto cleanup = qScopeGuard([&] { gst_object_unref(bin); });

        GstElement* source = findChildByFactoryName(bin, "souphttpsrc");
        QVERIFY(source);
        gboolean automaticRedirect = TRUE;
        GObject* tlsDatabase = nullptr;
        g_object_get(source, "automatic-redirect", &automaticRedirect, "tls-database", &tlsDatabase, nullptr);
        const auto databaseCleanup = qScopeGuard([&] { g_clear_object(&tlsDatabase); });
        QCOMPARE(automaticRedirect, FALSE);
        QVERIFY(tlsDatabase);
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
    connect(&server, &QTcpServer::pendingConnectionAvailable, this, [&]() {
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

void GStreamerTest::_testPipelineDiagnosticRedactionUsesGeneration()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse")) {
        QSKIP("HTTP MJPEG GStreamer plugins unavailable");
    }

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QTcpSocket* serverSocket = nullptr;
    QByteArray requestBytes;
    connect(&server, &QTcpServer::pendingConnectionAvailable, this, [&]() {
        serverSocket = server.nextPendingConnection();
        if (!serverSocket) {
            return;
        }
        connect(serverSocket, &QIODevice::readyRead, this, [&]() {
            requestBytes += serverSocket->readAll();
            if (!requestBytes.endsWith("\r\n\r\n")) {
                return;
            }
            serverSocket->write(
                QByteArrayLiteral("HTTP/1.1 200 OK\r\n"
                                  "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                                  "Cache-Control: no-store\r\n"
                                  "Connection: keep-alive\r\n\r\n"));
            serverSocket->flush();
        });
    });

    GstVideoReceiver receiver;
    receiver.setAutoReconnect(false);
    const QString sourceUrl =
        QStringLiteral("http://127.0.0.1:%1/mjpeg?token=pipeline-secret").arg(server.serverPort());
    receiver.setUri(sourceUrl);
    QSignalSpy startSpy(&receiver, &VideoReceiver::onStartComplete);
    receiver.start(3);
    QVERIFY_SIGNAL_WAIT(startSpy, TestTimeout::mediumMs());

    receiver.setUri(QString());
    constexpr const char* sentinel = "pipeline-secret-must-not-be-logged";
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("details redacted")));
    GstElement* pipeline = receiver._acquirePipelineRef();
    QVERIFY(pipeline);
    GError* warning = g_error_new_literal(GST_STREAM_ERROR, GST_STREAM_ERROR_FAILED, sentinel);
    GstMessage* message = gst_message_new_warning(GST_OBJECT(pipeline), warning, g_strdup(sentinel));
    QVERIFY(gst_element_post_message(pipeline, message));
    gst_object_unref(pipeline);
    verifyExpectedLogMessage();

    QSignalSpy stopSpy(&receiver, &VideoReceiver::onStopComplete);
    receiver.stop();
    QVERIFY_SIGNAL_WAIT(stopSpy, TestTimeout::mediumMs());
}

void GStreamerTest::_testHttpsMjpegTlsAuth()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse") || !gst_element_factory_find("fakesink")) {
        QSKIP("HTTP MJPEG GStreamer plugins unavailable");
    }

    QTemporaryFile caFile;
    QVERIFY(writeTestCaFile(caFile));
    QSslServer server;
    const QSslConfiguration sslConfiguration = testServerSslConfiguration();
    QVERIFY(!sslConfiguration.localCertificate().isNull());
    QVERIFY(!sslConfiguration.privateKey().isNull());
    server.setSslConfiguration(sslConfiguration);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    const QByteArray jpeg = createTestJpeg();
    QByteArray requestBytes;
    QSslSocket* serverSocket = nullptr;
    bool challengeSent = false;
    bool responseWriteAccepted = false;
    connect(&server, &QTcpServer::pendingConnectionAvailable, this, [&]() {
        if (serverSocket) {
            return;
        }
        serverSocket = qobject_cast<QSslSocket*>(server.nextPendingConnection());
        if (!serverSocket) {
            return;
        }
        connect(serverSocket, &QIODevice::readyRead, this, [&]() {
            requestBytes += serverSocket->readAll();
            if (!challengeSent && requestBytes.contains("\r\n\r\n")) {
                challengeSent = true;
                const QByteArray challenge = QByteArrayLiteral(
                    "HTTP/1.1 401 Unauthorized\r\n"
                    "WWW-Authenticate: Basic realm=\"QGC Video Test\"\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: keep-alive\r\n\r\n");
                serverSocket->write(challenge);
                serverSocket->flush();
                return;
            }
            if (responseWriteAccepted || !requestBytes.toLower().contains("authorization: basic ") ||
                !requestBytes.endsWith("\r\n\r\n")) {
                return;
            }
            const QByteArray response = QByteArrayLiteral(
                                            "HTTP/1.1 200 OK\r\n"
                                            "Cache-Control: no-store\r\n"
                                            "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                                            "Connection: close\r\n\r\n"
                                            "--frame\r\n"
                                            "Content-Type: image/jpeg\r\n") +
                                        QByteArrayLiteral("Content-Length: ") + QByteArray::number(jpeg.size()) +
                                        QByteArrayLiteral("\r\n\r\n") + jpeg + QByteArrayLiteral("\r\n--frame--\r\n");
            responseWriteAccepted = serverSocket->write(response) == response.size();
            serverSocket->flush();
        });
    });

    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Basic;
    config.networkSourceConfig.username = QStringLiteral("viewer");
    config.networkSourceConfig.secret = QByteArrayLiteral("test-password");
    config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
    config.networkSourceConfig.caCertificateFile = caFile.fileName();
    const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });
    const QString url = QStringLiteral("https://127.0.0.1:%1/mjpeg").arg(server.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });

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

    GstElement* pipeline = gst_pipeline_new("https-mjpeg-auth-test");
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
    QVERIFY(challengeSent);
    QVERIFY(responseWriteAccepted);

    const QByteArray lowerRequest = requestBytes.toLower();
    QCOMPARE(lowerRequest.count("authorization:"), 1);
    QVERIFY(lowerRequest.contains("authorization: basic dmlld2vyonrlc3qtcgfzc3dvcmq=\r\n"));
    QVERIFY(lowerRequest.contains("origin: https://operator.example.test\r\n"));
}

void GStreamerTest::_testHttpsMjpegRejectsUntrustedCa()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse") || !gst_element_factory_find("fakesink")) {
        QSKIP("HTTP MJPEG GStreamer plugins unavailable");
    }

    QSslServer server;
    server.setSslConfiguration(testServerSslConfiguration());
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QSignalSpy acceptedConnectionSpy(&server, &QTcpServer::pendingConnectionAvailable);

    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
    const QString url = QStringLiteral("https://127.0.0.1:%1/mjpeg").arg(server.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });
    GstElement* pipeline = gst_pipeline_new("https-mjpeg-untrusted-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
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

    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);
    GstMessage* message = waitForBusMessage(bus, GST_MESSAGE_ERROR, TestTimeout::mediumMs());
    gst_object_unref(bus);
    QVERIFY2(message, "Untrusted HTTPS MJPEG connection did not fail closed");
    gst_message_unref(message);
    QCOMPARE(acceptedConnectionSpy.count(), 0);
}

void GStreamerTest::_testHttpsMjpegAuthRedirectNotFollowed()
{
    if (!gst_element_factory_find("souphttpsrc") || !gst_element_factory_find("multipartdemux") ||
        !gst_element_factory_find("jpegparse") || !gst_element_factory_find("fakesink")) {
        QSKIP("HTTP MJPEG GStreamer plugins unavailable");
    }

    QTemporaryFile caFile;
    QVERIFY(writeTestCaFile(caFile));
    const QSslConfiguration sslConfiguration = testServerSslConfiguration();
    RawConnectionObservingSslServer redirectServer;
    RawConnectionObservingSslServer targetServer;
    redirectServer.setSslConfiguration(sslConfiguration);
    targetServer.setSslConfiguration(sslConfiguration);
    QVERIFY(redirectServer.listen(QHostAddress::LocalHost, 0));
    QVERIFY(targetServer.listen(QHostAddress::LocalHost, 0));

    QByteArray requestBytes;
    QSslSocket* redirectSocket = nullptr;
    bool challengeSent = false;
    bool redirectSent = false;
    connect(&redirectServer, &QTcpServer::pendingConnectionAvailable, this, [&]() {
        redirectSocket = qobject_cast<QSslSocket*>(redirectServer.nextPendingConnection());
        if (!redirectSocket) {
            return;
        }
        connect(redirectSocket, &QIODevice::readyRead, this, [&]() {
            requestBytes += redirectSocket->readAll();
            if (!requestBytes.endsWith("\r\n\r\n")) {
                return;
            }
            if (!challengeSent) {
                challengeSent = true;
                redirectSocket->write(
                    QByteArrayLiteral("HTTP/1.1 401 Unauthorized\r\n"
                                      "WWW-Authenticate: Basic realm=\"QGC Redirect Test\"\r\n"
                                      "Content-Length: 0\r\n"
                                      "Connection: keep-alive\r\n\r\n"));
                redirectSocket->flush();
                return;
            }
            if (redirectSent || !requestBytes.toLower().contains("authorization: basic ")) {
                return;
            }
            redirectSent = true;
            const QByteArray location =
                QStringLiteral("https://127.0.0.1:%1/video").arg(targetServer.serverPort()).toUtf8();
            const QByteArray response = QByteArrayLiteral("HTTP/1.1 302 Found\r\nLocation: ") + location +
                                        QByteArrayLiteral("\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            redirectSocket->write(response);
            redirectSocket->disconnectFromHost();
        });
    });

    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Basic;
    config.networkSourceConfig.username = QStringLiteral("viewer");
    config.networkSourceConfig.secret = QByteArrayLiteral("redirect-password");
    config.networkSourceConfig.caCertificateFile = caFile.fileName();
    const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });
    const QString url = QStringLiteral("https://127.0.0.1:%1/mjpeg").arg(redirectServer.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });
    GstElement* pipeline = gst_pipeline_new("https-mjpeg-redirect-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
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
    QVERIFY_TRUE_WAIT(redirectSent, TestTimeout::mediumMs());
    QVERIFY(challengeSent);
    QVERIFY(requestBytes.toLower().contains("authorization: basic "));
    QCOMPARE(targetServer.rawConnectionCount(), 0);
}

void GStreamerTest::_testNetworkJpegValidation()
{
    const QByteArray validJpeg = createTestJpeg();
    QVERIFY(!validJpeg.isEmpty());
    QString error;
    QVERIFY(QGCJpegStreamGuard::validateJpeg(validJpeg, &error));
    QVERIFY(error.isEmpty());

    QByteArray truncated = validJpeg;
    truncated.chop(2);
    QVERIFY(!QGCJpegStreamGuard::validateJpeg(truncated, &error));
    QVERIFY(!error.isEmpty());

    QByteArray excessiveDimensions = validJpeg;
    QVERIFY(replaceJpegDimensions(excessiveDimensions, 8192, 8192));
    QVERIFY(!QGCJpegStreamGuard::validateJpeg(excessiveDimensions, &error));
    QVERIFY(error.contains(QStringLiteral("dimensions")));

    const QByteArray excessiveEncodedSize(QGCJpegStreamGuard::kMaximumEncodedBytes + 1, '\0');
    QVERIFY(!QGCJpegStreamGuard::validateJpeg(excessiveEncodedSize, &error));
    QVERIFY(error.contains(QStringLiteral("encoded-size")));
}

void GStreamerTest::_testMultipartJpegGuard()
{
    QString error;
    QGCJpegStreamGuard::MultipartGuard declaredGuard(128, 32);
    QVERIFY(declaredGuard.setBoundary(QByteArrayLiteral("frame"), &error));
    QVERIFY(declaredGuard.consume(QByteArrayLiteral("--fra"), &error));
    QVERIFY(declaredGuard.consume(QByteArrayLiteral("me\r\nContent-Type: image/jpeg\r\n\r\nabc\r\n--fr"), &error));
    QVERIFY(declaredGuard.consume(QByteArrayLiteral("ame\r\nContent-Type: image/jpeg\r\n\r\ndef\r\n--frame--\r\n"),
                                  &error));

    QGCJpegStreamGuard::MultipartGuard discoveredGuard(128, 64);
    QVERIFY(discoveredGuard.consume(QByteArrayLiteral("--camera\r"), &error));
    QVERIFY(
        discoveredGuard.consume(QByteArrayLiteral("\nContent-Type: image/jpeg\r\n\r\nabc\r\n--camera--\r\n"), &error));

    QGCJpegStreamGuard::MultipartGuard missingBoundary(128, 8);
    QVERIFY(missingBoundary.setBoundary(QByteArrayLiteral("frame"), &error));
    QVERIFY(!missingBoundary.consume(QByteArrayLiteral("123456789"), &error));
    QVERIFY(error.contains(QStringLiteral("declared boundary")));

    QGCJpegStreamGuard::MultipartGuard oversizedPart(24, 32);
    QVERIFY(oversizedPart.setBoundary(QByteArrayLiteral("frame"), &error));
    QVERIFY(oversizedPart.consume(QByteArrayLiteral("--frame\r\n"), &error));
    QVERIFY(!oversizedPart.consume(QByteArray(23, 'x'), &error));
    QVERIFY(error.contains(QStringLiteral("exceeds")));

    QGCJpegStreamGuard::MultipartGuard oversizedSingleBuffer(24, 32);
    QVERIFY(oversizedSingleBuffer.setBoundary(QByteArrayLiteral("frame"), &error));
    const QByteArray oversizedBody =
        QByteArrayLiteral("--frame\r\n") + QByteArray(30, 'x') + QByteArrayLiteral("\r\n--frame\r\n");
    QVERIFY(!oversizedSingleBuffer.consume(oversizedBody, &error));
    QVERIFY(error.contains(QStringLiteral("exceeds")));
}

void GStreamerTest::_testJpegRecordingContainers()
{
    const char* requiredFactories[] = {
        "appsrc", "jpegparse", "splitmuxsink", "qtmux", "matroskamux", "playbin", "fakesink",
    };
    for (const char* factory : requiredFactories) {
        GstElementFactory* elementFactory = gst_element_factory_find(factory);
        if (!elementFactory) {
            QSKIP(qPrintable(QStringLiteral("Required recording/playback plugin is unavailable: %1").arg(factory)));
        }
        gst_object_unref(elementFactory);
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QByteArray jpeg = createTestJpeg(32);
    QVERIFY(QGCJpegStreamGuard::validateJpeg(jpeg));

    const QList<QPair<QString, QString>> formats = {
        {QStringLiteral("qtmux"), QStringLiteral("mov")},
        {QStringLiteral("matroskamux"), QStringLiteral("mkv")},
    };
    for (const auto& [muxer, extension] : formats) {
        const QString outputPath = directory.filePath(QStringLiteral("jpeg-recording.%1").arg(extension));
        const QString pipelineDescription =
            QStringLiteral(
                "appsrc name=source caps=image/jpeg format=time is-live=false ! jpegparse ! "
                "splitmuxsink muxer-factory=%1 async-finalize=true message-forward=true location=\"%2\"")
                .arg(muxer, outputPath);
        GError* parseError = nullptr;
        GstElement* pipeline = gst_parse_launch(pipelineDescription.toUtf8().constData(), &parseError);
        const QString parseErrorText = parseError ? QString::fromUtf8(parseError->message) : QString();
        g_clear_error(&parseError);
        QVERIFY2(pipeline, qPrintable(parseErrorText));
        const auto pipelineCleanup = qScopeGuard([&] {
            (void) gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_clear_object(&pipeline);
        });

        GstElement* appsrc = gst_bin_get_by_name(GST_BIN(pipeline), "source");
        QVERIFY(appsrc);
        const auto appsrcCleanup = qScopeGuard([&] { gst_object_unref(appsrc); });
        QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
        for (int frame = 0; frame < 3; ++frame) {
            GstBuffer* frameBuffer = gst_buffer_new_allocate(nullptr, static_cast<gsize>(jpeg.size()), nullptr);
            QVERIFY(frameBuffer);
            gst_buffer_fill(frameBuffer, 0, jpeg.constData(), static_cast<gsize>(jpeg.size()));
            GST_BUFFER_PTS(frameBuffer) = static_cast<GstClockTime>(frame) * GST_SECOND / 10;
            GST_BUFFER_DURATION(frameBuffer) = GST_SECOND / 10;
            QCOMPARE(gst_app_src_push_buffer(GST_APP_SRC(appsrc), frameBuffer), GST_FLOW_OK);
        }
        QCOMPARE(gst_app_src_end_of_stream(GST_APP_SRC(appsrc)), GST_FLOW_OK);

        GstBus* bus = gst_element_get_bus(pipeline);
        QVERIFY(bus);
        GstMessage* message =
            gst_bus_timed_pop_filtered(bus, static_cast<GstClockTime>(TestTimeout::longMs()) * GST_MSECOND,
                                       static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        gst_object_unref(bus);
        QVERIFY2(message, "Timed out finalizing the JPEG recording");
        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
            GError* recordingError = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(message, &recordingError, &debug);
            const QString detail =
                recordingError ? QString::fromUtf8(recordingError->message) : QStringLiteral("unknown");
            g_clear_error(&recordingError);
            g_clear_pointer(&debug, g_free);
            gst_message_unref(message);
            QFAIL(qPrintable(QStringLiteral("JPEG recording failed: %1").arg(detail)));
        }
        gst_message_unref(message);
        (void) gst_element_set_state(pipeline, GST_STATE_NULL);

        const QFileInfo recording(outputPath);
        QVERIFY(recording.exists());
        QVERIFY(recording.size() > 0);

        GstElement* playback = gst_element_factory_make("playbin", nullptr);
        GstElement* videoSink = gst_element_factory_make("fakesink", nullptr);
        GstElement* audioSink = gst_element_factory_make("fakesink", nullptr);
        QVERIFY(playback);
        QVERIFY(videoSink);
        QVERIFY(audioSink);
        gst_object_ref_sink(videoSink);
        gst_object_ref_sink(audioSink);
        const QByteArray uri = QUrl::fromLocalFile(outputPath).toEncoded();
        g_object_set(playback, "uri", uri.constData(), "video-sink", videoSink, "audio-sink", audioSink, nullptr);
        gst_object_unref(videoSink);
        gst_object_unref(audioSink);
        const auto playbackCleanup = qScopeGuard([&] {
            (void) gst_element_set_state(playback, GST_STATE_NULL);
            gst_clear_object(&playback);
        });
        QVERIFY(gst_element_set_state(playback, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
        bus = gst_element_get_bus(playback);
        QVERIFY(bus);
        message = gst_bus_timed_pop_filtered(bus, static_cast<GstClockTime>(TestTimeout::longMs()) * GST_MSECOND,
                                             static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        gst_object_unref(bus);
        QVERIFY2(message, "Timed out playing the finalized JPEG recording");
        if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
            GError* playbackError = nullptr;
            gchar* debug = nullptr;
            gst_message_parse_error(message, &playbackError, &debug);
            const QString detail =
                playbackError ? QString::fromUtf8(playbackError->message) : QStringLiteral("unknown");
            g_clear_error(&playbackError);
            g_clear_pointer(&debug, g_free);
            gst_message_unref(message);
            QFAIL(qPrintable(QStringLiteral("JPEG recording playback failed: %1").arg(detail)));
        }
        gst_message_unref(message);
    }
}

void GStreamerTest::_testRecordingEosSeqnumClassification()
{
    GstVideoReceiver receiver;
    GstElement* messageSource = gst_pipeline_new("recording-eos-seqnum-test");
    GstElement* recordingBin = gst_bin_new("recording-sink-bin");
    GstElement* recordingSink = gst_element_factory_make("fakesink", nullptr);
    QVERIFY(messageSource);
    QVERIFY(recordingBin);
    QVERIFY(recordingSink);
    QVERIFY(gst_bin_add(GST_BIN(recordingBin), recordingSink));
    GstMessage* recordingMessage = gst_message_new_eos(GST_OBJECT(messageSource));
    GstMessage* sourceMessage = gst_message_new_eos(GST_OBJECT(messageSource));
    GstMessage* ancestryMessage = gst_message_new_eos(GST_OBJECT(recordingSink));
    QVERIFY(recordingMessage);
    QVERIFY(sourceMessage);
    QVERIFY(ancestryMessage);
    const auto cleanup = qScopeGuard([&] {
        gst_clear_message(&recordingMessage);
        gst_clear_message(&sourceMessage);
        gst_clear_message(&ancestryMessage);
        gst_clear_object(&recordingBin);
        gst_clear_object(&messageSource);
    });

    const guint32 recordingSeqnum = gst_util_seqnum_next();
    const guint32 sourceSeqnum = gst_util_seqnum_next();
    QVERIFY(recordingSeqnum != GST_SEQNUM_INVALID);
    QVERIFY(sourceSeqnum != GST_SEQNUM_INVALID);
    QVERIFY(recordingSeqnum != sourceSeqnum);
    gst_message_set_seqnum(recordingMessage, recordingSeqnum);
    gst_message_set_seqnum(sourceMessage, sourceSeqnum);

    receiver._recordingEosSeqnum.store(recordingSeqnum, std::memory_order_release);
    QVERIFY(receiver._isRecordingEOSMessage(recordingMessage));
    QVERIFY(receiver._isRecordingEOSMessage(ancestryMessage));
    QVERIFY(!receiver._isRecordingEOSMessage(sourceMessage));

    receiver._recordingEosSeqnum.store(GST_SEQNUM_INVALID, std::memory_order_release);
    QVERIFY(!receiver._isRecordingEOSMessage(recordingMessage));
}

void GStreamerTest::_testJpegReceiverRecording()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    const char* requiredFactories[] = {
        "appsrc", "jpegparse", "splitmuxsink", "qtmux", "matroskamux", "playbin", "fakesink",
    };
    for (const char* factory : requiredFactories) {
        GstElementFactory* elementFactory = gst_element_factory_find(factory);
        if (!elementFactory) {
            QSKIP(qPrintable(QStringLiteral("Required receiver-recording plugin is unavailable: %1").arg(factory)));
        }
        gst_object_unref(elementFactory);
    }

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QWebSocketServer server(QStringLiteral("QGC receiver recording test"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QWebSocket* serverSocket = nullptr;
    connect(&server, &QWebSocketServer::newConnection, this,
            [&server, &serverSocket]() { serverSocket = server.nextPendingConnection(); });

    GstVideoReceiver receiver;
    receiver.setAutoReconnect(false);
    receiver.setUri(QStringLiteral("ws://127.0.0.1:%1/video").arg(server.serverPort()));
    QSignalSpy receiverStartSpy(&receiver, &VideoReceiver::onStartComplete);
    receiver.start(30);
    QVERIFY_SIGNAL_WAIT(receiverStartSpy, TestTimeout::mediumMs());
    QVERIFY_TRUE_WAIT(serverSocket != nullptr, TestTimeout::mediumMs());
    QSignalSpy unexpectedReceiverStopSpy(&receiver, &VideoReceiver::onStopComplete);

    const QString rejectedMp4 = directory.filePath(QStringLiteral("jpeg-rejected.mp4"));
    // The configured URI can change before the old worker pipeline is retired. Recording
    // compatibility must remain tied to the source that is actually producing frames.
    receiver.setUri(QStringLiteral("rtsp://127.0.0.1/not-the-active-pipeline"));
    expectLogMessage("Video.GStreamer.GstVideoReceiver", QtWarningMsg,
                     QRegularExpression(QStringLiteral("MP4 recording is unavailable")));
    QSignalSpy rejectedRecordingSpy(&receiver, &VideoReceiver::onStartRecordingComplete);
    receiver.startRecording(rejectedMp4, VideoReceiver::FILE_FORMAT_MP4);
    QVERIFY_SIGNAL_WAIT(rejectedRecordingSpy, TestTimeout::mediumMs());
    QCOMPARE(qvariant_cast<VideoReceiver::STATUS>(rejectedRecordingSpy.takeFirst().at(0)),
             VideoReceiver::STATUS_NOT_IMPLEMENTED);
    verifyExpectedLogMessage();
    QVERIFY(!QFileInfo::exists(rejectedMp4));
    receiver.setUri(QStringLiteral("ws://127.0.0.1:%1/video").arg(server.serverPort()));

    const QByteArray jpeg = createTestJpeg(32);
    QVERIFY(!jpeg.isEmpty());
    const QList<QPair<VideoReceiver::FILE_FORMAT, QString>> formats = {
        {VideoReceiver::FILE_FORMAT_MOV, QStringLiteral("mov")},
        {VideoReceiver::FILE_FORMAT_MKV, QStringLiteral("mkv")},
    };
    for (const auto& [format, extension] : formats) {
        const QString outputPath = directory.filePath(QStringLiteral("receiver-recording.%1").arg(extension));
        QTimer frameTimer;
        connect(&frameTimer, &QTimer::timeout, this, [&]() {
            if (serverSocket && serverSocket->state() == QAbstractSocket::ConnectedState) {
                serverSocket->sendBinaryMessage(jpeg);
                serverSocket->flush();
            }
        });
        const quint64 preRecordingFrameCount = receiver._sourceFrameCount.load(std::memory_order_relaxed);
        frameTimer.start(1);
        QVERIFY_TRUE_WAIT(receiver._sourceFrameCount.load(std::memory_order_relaxed) > preRecordingFrameCount,
                          TestTimeout::mediumMs());

        QSignalSpy startRecordingSpy(&receiver, &VideoReceiver::onStartRecordingComplete);
        QSignalSpy recordingStartedSpy(&receiver, &VideoReceiver::recordingStarted);
        receiver.startRecording(outputPath, format);
        QVERIFY_SIGNAL_WAIT(startRecordingSpy, TestTimeout::mediumMs());
        QCOMPARE(qvariant_cast<VideoReceiver::STATUS>(startRecordingSpy.takeFirst().at(0)), VideoReceiver::STATUS_OK);
        QVERIFY_SIGNAL_WAIT(recordingStartedSpy, TestTimeout::mediumMs());
        QCOMPARE(recordingStartedSpy.takeFirst().at(0).toString(), outputPath);
        QCOMPARE(receiver._keyframeWatchId.load(std::memory_order_acquire), static_cast<gulong>(0));

        const quint64 initialFrameCount = receiver._sourceFrameCount.load(std::memory_order_relaxed);
        for (int frame = 0; frame < 12; ++frame) {
            QCOMPARE(serverSocket->sendBinaryMessage(jpeg), static_cast<qint64>(jpeg.size()));
            serverSocket->flush();
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        }
        QVERIFY_TRUE_WAIT(receiver._sourceFrameCount.load(std::memory_order_relaxed) >= initialFrameCount + 3,
                          TestTimeout::mediumMs());
        frameTimer.stop();

        QSignalSpy stopRecordingSpy(&receiver, &VideoReceiver::onStopRecordingComplete);
        receiver.stopRecording();
        QVERIFY_SIGNAL_WAIT(stopRecordingSpy, TestTimeout::longMs());
        QCOMPARE(qvariant_cast<VideoReceiver::STATUS>(stopRecordingSpy.takeFirst().at(0)), VideoReceiver::STATUS_OK);
        QCOMPARE(receiver._keyframeWatchId.load(std::memory_order_acquire), static_cast<gulong>(0));

        const quint64 postRecordingFrameCount = receiver._sourceFrameCount.load(std::memory_order_relaxed);
        frameTimer.start(1);
        QVERIFY_TRUE_WAIT(receiver._sourceFrameCount.load(std::memory_order_relaxed) >= postRecordingFrameCount + 12,
                          TestTimeout::mediumMs());
        frameTimer.stop();
        QCOMPARE(unexpectedReceiverStopSpy.count(), 0);

        const QFileInfo recording(outputPath);
        QVERIFY(recording.exists());
        QVERIFY(recording.size() > 0);

        GstElement* playback = gst_element_factory_make("playbin", nullptr);
        GstElement* videoSink = gst_element_factory_make("fakesink", nullptr);
        GstElement* audioSink = gst_element_factory_make("fakesink", nullptr);
        QVERIFY(playback);
        QVERIFY(videoSink);
        QVERIFY(audioSink);
        gst_object_ref_sink(videoSink);
        gst_object_ref_sink(audioSink);
        const auto playbackCleanup = qScopeGuard([&] {
            (void) gst_element_set_state(playback, GST_STATE_NULL);
            gst_clear_object(&playback);
        });
        const QByteArray uri = QUrl::fromLocalFile(outputPath).toEncoded();
        g_object_set(playback, "uri", uri.constData(), "video-sink", videoSink, "audio-sink", audioSink, nullptr);
        gst_object_unref(videoSink);
        gst_object_unref(audioSink);
        QVERIFY(gst_element_set_state(playback, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
        GstBus* playbackBus = gst_element_get_bus(playback);
        QVERIFY(playbackBus);
        GstMessage* playbackMessage =
            gst_bus_timed_pop_filtered(playbackBus, static_cast<GstClockTime>(TestTimeout::longMs()) * GST_MSECOND,
                                       static_cast<GstMessageType>(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));
        gst_object_unref(playbackBus);
        QVERIFY2(playbackMessage, "Timed out playing the receiver JPEG recording");
        QCOMPARE(GST_MESSAGE_TYPE(playbackMessage), GST_MESSAGE_EOS);
        gst_message_unref(playbackMessage);
    }

    QSignalSpy receiverStopSpy(&receiver, &VideoReceiver::onStopComplete);
    receiver.stop();
    QVERIFY_SIGNAL_WAIT(receiverStopSpy, TestTimeout::longMs());
    if (serverSocket) {
        serverSocket->close();
        serverSocket->deleteLater();
    }
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testSourceFactoryWebSocketJpeg()
{
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    GStreamer::SourceFactory::Config config;
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    QWebSocketServer server(QStringLiteral("QGC source factory test"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    const QString url = QStringLiteral("ws://127.0.0.1:%1/ws/video_feed").arg(server.serverPort());
    GstElement* bin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(bin);
    QWebSocket* serverSocket = nullptr;
    const auto cleanup = qScopeGuard([&] {
        gst_clear_object(&bin);
        if (serverSocket) {
            serverSocket->close();
            serverSocket->deleteLater();
        }
    });

    QVERIFY2(findChildByFactoryName(bin, "appsrc"), "ws:// must build an appsrc bridge");
    QVERIFY2(findChildByFactoryName(bin, "jpegparse"), "WebSocket JPEG must expose parsed JPEG frames");
    QVERIFY_TRUE_WAIT(server.hasPendingConnections(), TestTimeout::mediumMs());
    serverSocket = server.nextPendingConnection();
    QVERIFY(serverSocket);
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
    const QByteArray validJpeg = createTestJpeg();
    QVERIFY(!validJpeg.isEmpty());
    QVERIFY(QGCWebSocketVideoSource::isCompleteJpeg(validJpeg));
    QVERIFY(!QGCWebSocketVideoSource::isCompleteJpeg(QByteArray::fromHex("ffd800ffd9")));
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

void GStreamerTest::_testWebSocketRejectsInvalidFrame()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse") ||
        !gst_element_factory_find("fakesink")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    QWebSocketServer server(QStringLiteral("QGC invalid video test"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QWebSocket* serverSocket = nullptr;
    connect(&server, &QWebSocketServer::newConnection, this,
            [&server, &serverSocket]() { serverSocket = server.nextPendingConnection(); });

    GStreamer::SourceFactory::Config config;
    const QString url = QStringLiteral("ws://127.0.0.1:%1/video").arg(server.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });
    GstElement* pipeline = gst_pipeline_new("websocket-invalid-jpeg-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
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

    QSignalSpy disconnectedSpy(serverSocket, &QWebSocket::disconnected);
    expectLogMessage("Video.GStreamer.WebSocketVideoSource", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Rejecting WebSocket JPEG stream")));
    const QByteArray invalidFrame = QByteArray::fromHex("ffd800ffd9");
    for (int attempt = 0; attempt < 3; ++attempt) {
        serverSocket->sendBinaryMessage(invalidFrame);
    }
    serverSocket->flush();

    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);
    GstMessage* message = gst_bus_timed_pop_filtered(
        bus, static_cast<GstClockTime>(TestTimeout::mediumMs()) * GST_MSECOND, GST_MESSAGE_ERROR);
    gst_object_unref(bus);
    QVERIFY2(message, "Invalid WebSocket JPEG did not fail the source pipeline");
    gst_message_unref(message);
    verifyExpectedLogMessage();
    if (serverSocket->state() != QAbstractSocket::UnconnectedState) {
        QVERIFY_SIGNAL_WAIT(disconnectedSpy, TestTimeout::mediumMs());
    }
    QCOMPARE(serverSocket->state(), QAbstractSocket::UnconnectedState);
    serverSocket->deleteLater();
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testWebSocketEarlyTransportFailureAfterDelayedParenting()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse") ||
        !gst_element_factory_find("fakesink")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    QTcpServer portReservation;
    QVERIFY(portReservation.listen(QHostAddress::LocalHost, 0));
    const quint16 refusedPort = portReservation.serverPort();
    portReservation.close();

    expectLogMessage("Video.GStreamer.WebSocketVideoSource", QtWarningMsg,
                     QRegularExpression(QStringLiteral("WebSocket error code")));
    GStreamer::SourceFactory::Config config;
    GstElement* sourceBin =
        GStreamer::SourceFactory::create(QStringLiteral("ws://127.0.0.1:%1/video").arg(refusedPort), config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });

    // Keep the source unparented long enough for localhost refusal to complete.
    // The error must be retained even though there is no pipeline bus yet.
    QElapsedTimer preParentDelay;
    preParentDelay.start();
    while (preParentDelay.elapsed() < TestTimeout::shortMs()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(5);
    }
    verifyExpectedLogMessage();

    GstElement* pipeline = gst_pipeline_new("websocket-early-transport-error-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
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

    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);
    GstMessage* message = waitForBusMessage(bus, GST_MESSAGE_ERROR, TestTimeout::mediumMs());
    gst_object_unref(bus);
    QVERIFY2(message, "Pre-parent WebSocket refusal was not replayed to the pipeline bus");
    QCOMPARE(GST_MESSAGE_TYPE(message), GST_MESSAGE_ERROR);
    GError* error = nullptr;
    gchar* debug = nullptr;
    gst_message_parse_error(message, &error, &debug);
    QCOMPARE(error ? error->domain : 0, static_cast<GQuark>(GST_RESOURCE_ERROR));
    QCOMPARE(error ? error->code : -1, static_cast<int>(GST_RESOURCE_ERROR_OPEN_READ));
    g_clear_error(&error);
    g_clear_pointer(&debug, g_free);
    gst_message_unref(message);
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testWebSocketEarlyProtocolFailureAfterDelayedParenting()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse") ||
        !gst_element_factory_find("fakesink")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    QWebSocketServer server(QStringLiteral("QGC early invalid video test"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QWebSocket* serverSocket = nullptr;
    connect(&server, &QWebSocketServer::newConnection, this,
            [&server, &serverSocket]() { serverSocket = server.nextPendingConnection(); });

    GStreamer::SourceFactory::Config config;
    GstElement* sourceBin =
        GStreamer::SourceFactory::create(QStringLiteral("ws://127.0.0.1:%1/video").arg(server.serverPort()), config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });
    QVERIFY_TRUE_WAIT(serverSocket != nullptr, TestTimeout::mediumMs());

    QSignalSpy disconnectedSpy(serverSocket, &QWebSocket::disconnected);
    expectLogMessage("Video.GStreamer.WebSocketVideoSource", QtWarningMsg,
                     QRegularExpression(QStringLiteral("Rejecting WebSocket JPEG stream")));
    const QByteArray invalidFrame = QByteArray::fromHex("ffd800ffd9");
    QCOMPARE(serverSocket->sendBinaryMessage(invalidFrame), static_cast<qint64>(invalidFrame.size()));
    serverSocket->flush();
    QVERIFY_SIGNAL_WAIT(disconnectedSpy, TestTimeout::mediumMs());
    verifyExpectedLogMessage();

    GstElement* pipeline = gst_pipeline_new("websocket-early-protocol-error-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
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

    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);
    GstMessage* message = waitForBusMessage(bus, GST_MESSAGE_ERROR, TestTimeout::mediumMs());
    gst_object_unref(bus);
    QVERIFY2(message, "Pre-parent invalid WebSocket JPEG was not replayed to the pipeline bus");
    QCOMPARE(GST_MESSAGE_TYPE(message), GST_MESSAGE_ERROR);
    GError* error = nullptr;
    gchar* debug = nullptr;
    gst_message_parse_error(message, &error, &debug);
    QCOMPARE(error ? error->domain : 0, static_cast<GQuark>(GST_STREAM_ERROR));
    QCOMPARE(error ? error->code : -1, static_cast<int>(GST_STREAM_ERROR_DECODE));
    g_clear_error(&error);
    g_clear_pointer(&debug, g_free);
    gst_message_unref(message);
    serverSocket->deleteLater();
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testWebSocketJpegTlsAuth()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse") ||
        !gst_element_factory_find("fakesink")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    QTemporaryFile caFile;
    QVERIFY(writeTestCaFile(caFile));
    QWebSocketServer server(QStringLiteral("QGC secure video delivery test"), QWebSocketServer::SecureMode);
    const QSslConfiguration sslConfiguration = testServerSslConfiguration();
    QVERIFY(!sslConfiguration.localCertificate().isNull());
    QVERIFY(!sslConfiguration.privateKey().isNull());
    server.setSslConfiguration(sslConfiguration);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QWebSocket* serverSocket = nullptr;
    connect(&server, &QWebSocketServer::newConnection, this,
            [&server, &serverSocket]() { serverSocket = server.nextPendingConnection(); });

    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Bearer;
    config.networkSourceConfig.secret = QByteArrayLiteral("test-token");
    config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
    config.networkSourceConfig.caCertificateFile = caFile.fileName();
    const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });
    const QString url = QStringLiteral("wss://127.0.0.1:%1/video").arg(server.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });

    GstElement* appsrc = findChildByFactoryName(sourceBin, "appsrc");
    QVERIFY(appsrc);
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

    GstElement* pipeline = gst_pipeline_new("websocket-jpeg-tls-test");
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
    QCOMPARE(serverSocket->request().rawHeader("Authorization"), QByteArrayLiteral("Bearer test-token"));

    const QByteArray jpeg = createTestJpeg();
    QCOMPARE(serverSocket->sendBinaryMessage(jpeg), static_cast<qint64>(jpeg.size()));
    serverSocket->flush();
    QVERIFY_TRUE_WAIT(probeContext.observed.load(std::memory_order_acquire), TestTimeout::mediumMs());
    serverSocket->close();
    serverSocket->deleteLater();
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testWebSocketAuthRedirectNotFollowed()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse") ||
        !gst_element_factory_find("fakesink")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    QTemporaryFile caFile;
    QVERIFY(writeTestCaFile(caFile));
    const QSslConfiguration sslConfiguration = testServerSslConfiguration();
    RawConnectionObservingSslServer redirectServer;
    RawConnectionObservingSslServer targetServer;
    redirectServer.setSslConfiguration(sslConfiguration);
    targetServer.setSslConfiguration(sslConfiguration);
    QVERIFY(redirectServer.listen(QHostAddress::LocalHost, 0));
    QVERIFY(targetServer.listen(QHostAddress::LocalHost, 0));

    QByteArray requestBytes;
    QSslSocket* redirectSocket = nullptr;
    bool redirectSent = false;
    connect(&redirectServer, &QTcpServer::pendingConnectionAvailable, this, [&]() {
        redirectSocket = qobject_cast<QSslSocket*>(redirectServer.nextPendingConnection());
        if (!redirectSocket) {
            return;
        }
        connect(redirectSocket, &QIODevice::readyRead, this, [&]() {
            requestBytes += redirectSocket->readAll();
            if (redirectSent || !requestBytes.endsWith("\r\n\r\n")) {
                return;
            }
            redirectSent = true;
            const QByteArray location =
                QStringLiteral("wss://127.0.0.1:%1/video").arg(targetServer.serverPort()).toUtf8();
            redirectSocket->write(QByteArrayLiteral("HTTP/1.1 302 Found\r\nLocation: ") + location +
                                  QByteArrayLiteral("\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"));
            redirectSocket->disconnectFromHost();
        });
    });

    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.authentication = VideoReceiver::NetworkSourceConfig::Authentication::Bearer;
    config.networkSourceConfig.secret = QByteArrayLiteral("redirect-token");
    config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
    config.networkSourceConfig.caCertificateFile = caFile.fileName();
    const auto secretCleanup = qScopeGuard([&] { config.networkSourceConfig.clearSecret(); });
    const QString url = QStringLiteral("wss://127.0.0.1:%1/video").arg(redirectServer.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });
    GstElement* pipeline = gst_pipeline_new("websocket-auth-redirect-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
    GstElement* sourceElement = sourceBin;
    gst_bin_add_many(GST_BIN(pipeline), sourceBin, sink, nullptr);
    sourceCleanup.dismiss();
    sourceBin = nullptr;
    const auto pipelineCleanup = qScopeGuard([&] {
        (void) gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_clear_object(&pipeline);
    });
    QVERIFY(gst_element_link(sourceElement, sink));

    expectLogMessage("Video.GStreamer.WebSocketVideoSource", QtWarningMsg,
                     QRegularExpression(QStringLiteral("WebSocket error code")));
    QVERIFY(gst_element_set_state(pipeline, GST_STATE_PLAYING) != GST_STATE_CHANGE_FAILURE);
    QVERIFY_TRUE_WAIT(redirectSent, TestTimeout::mediumMs());
    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);
    GstMessage* message = waitForBusMessage(bus, GST_MESSAGE_ERROR, TestTimeout::mediumMs());
    gst_object_unref(bus);
    QVERIFY2(message, "Redirected WSS handshake did not fail the source pipeline");
    gst_message_unref(message);
    verifyExpectedLogMessage();

    const QByteArray lowerRequest = requestBytes.toLower();
    QVERIFY(lowerRequest.contains("authorization: bearer redirect-token\r\n"));
    QVERIFY(lowerRequest.contains("origin: https://operator.example.test\r\n"));

    // Keep the source alive long enough to detect a queued or delayed attempt to
    // follow the redirect. The raw observer fires before any TLS handshake result.
    QEventLoop noTargetConnectionObservation;
    QTimer observationTimeout;
    observationTimeout.setSingleShot(true);
    QObject::connect(&observationTimeout, &QTimer::timeout, &noTargetConnectionObservation, &QEventLoop::quit);
    targetServer.setRawConnectionObserver([&noTargetConnectionObservation] { noTargetConnectionObservation.exit(1); });
    observationTimeout.start(TestTimeout::shortMs());
    QCOMPARE(noTargetConnectionObservation.exec(), 0);
    targetServer.setRawConnectionObserver({});
    QCOMPARE(targetServer.rawConnectionCount(), 0);
#else
    QSKIP("Qt WebSockets unavailable");
#endif
}

void GStreamerTest::_testWebSocketJpegRejectsUntrustedCa()
{
#ifdef QGC_HAS_WEBSOCKET_VIDEO
    if (!gst_element_factory_find("appsrc") || !gst_element_factory_find("jpegparse") ||
        !gst_element_factory_find("fakesink")) {
        QSKIP("WebSocket JPEG GStreamer plugins unavailable");
    }

    RawConnectionObservingSslServer server;
    server.setSslConfiguration(testServerSslConfiguration());
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    expectLogMessage("Video.GStreamer.WebSocketVideoSource", QtWarningMsg,
                     QRegularExpression(QStringLiteral("TLS verification failed")));
    expectLogMessage("Video.GStreamer.WebSocketVideoSource", QtWarningMsg,
                     QRegularExpression(QStringLiteral("WebSocket error code")));
    GStreamer::SourceFactory::Config config;
    config.networkSourceConfig.origin = QStringLiteral("https://operator.example.test");
    const QString url = QStringLiteral("wss://127.0.0.1:%1/video").arg(server.serverPort());
    GstElement* sourceBin = GStreamer::SourceFactory::create(url, config);
    QVERIFY(sourceBin);
    auto sourceCleanup = qScopeGuard([&] { gst_clear_object(&sourceBin); });
    GstElement* pipeline = gst_pipeline_new("websocket-untrusted-ca-test");
    GstElement* sink = gst_element_factory_make("fakesink", "sink");
    QVERIFY(pipeline);
    QVERIFY(sink);
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
    GstBus* bus = gst_element_get_bus(pipeline);
    QVERIFY(bus);
    GstMessage* message = waitForBusMessage(bus, GST_MESSAGE_ERROR, TestTimeout::mediumMs());
    gst_object_unref(bus);
    QVERIFY2(message, "Untrusted WSS connection did not fail the source pipeline");
    gst_message_unref(message);
    verifyExpectedLogMessage();
    verifyExpectedLogMessage();
    QVERIFY(server.rawConnectionCount() > 0);
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
